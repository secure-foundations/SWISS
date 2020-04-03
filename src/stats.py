import json

def log(f, *args):
  print(*args, file=f)

class Stats(object):
  def __init__(self, num_threads, all_args, ivy_filename, json_filename):
    self.num_threads = num_threads
    self.all_args = all_args
    self.inc_logs = []
    self.finisher_logs = []
    self.inc_result_filenames = []
    self.finisher_result_filename = None
    self.ivy_filename = ivy_filename
    self.json_filename = json_filename

    self.inc_individual_times = []
    self.inc_times = []
    self.finisher_individual_times = []
    self.finisher_time = 0

    self.inc_results = []
    self.finisher_result = None

  def add_inc_log(self, iternum, log, seconds):
    while len(self.inc_logs) <= iternum:
      self.inc_logs.append([])
      self.inc_times.append([])
    self.inc_logs[-1].append(log)
    self.inc_times[-1].append(seconds)

  def add_inc_result(self, iternum, log, seconds):
    assert len(self.inc_results) == iternum
    self.inc_result_filenames.append(log)
    self.inc_times.append(seconds)

  def add_finisher_log(self, filename, seconds):
    self.finisher_logs.append(filename)
    self.finisher_individual_times.append(seconds)

  def add_finisher_result(self, filename, seconds):
    self.finisher_result_filename = filename
    self.finisher_time = seconds

  def num_incremental(self):
    return len(self.inc_results)

  def num_finisher(self):
    if self.finisher_result == None:
      return 0
    else:
      return 1

  def read_result_file(self, filename):
    with open(filename, "r") as f:
      return json.loads(f.read())

  def read_result_files(self):
    for filename in self.inc_result_filenames:
      self.inc_results.append(self.read_result_file(filename))
    if self.finisher_result_filename != None:
      self.finisher_result = self.read_result_file(self.finisher_result_filename)

  def was_success(self):
    for res in self.inc_results:
      if self.inc_results["success"]:
        return True
    if self.finisher_result and self.finisher_result["success"]:
      return True
    return False

  def total_number_of_invariants_incremental(self):
    if len(self.inc_results) > 0:
      return len(self.inc_results[-1].formulas)
    else:
      return 0

  def total_number_of_invariants_finisher(self):
    if self.finisher_result and self.finisher_result["success"]:
      return 1
    else:
      return 0

  def total_number_of_invariants(self):
    return (self.total_number_of_invariants_incremental()
        + self.total_number_of_invariants_finisher())

  def get_inc_time(self, i):
    return self.inc_times[i]

  def get_inc_cpu_time(self, i):
    return sum(self.inc_individual_times[i])

  def get_finisher_time(self):
    return self.finisher_time

  def get_finisher_cpu_time(self):
    return sum(self.finisher_individual_times)

  def get_total_time(self):
    t = 0
    for i in range(len(self.inc_results)):
      t += self.get_inc_time(i)
    t += self.get_finisher_time()
    return t

  def get_total_cpu_time(self):
    t = 0
    for i in range(len(self.inc_results)):
      t += self.get_inc_cpu_time(i)
    t += self.get_finisher_cpu_time()
    return t

  def print_stats(self, filename):
    self.read_result_files()
    with open(filename, "w") as f:
      log(f, "Protocol:", self.ivy_filename)
      log(f, "Args:", " ".join(self.all_args))
      log(f, "Number of threads:", self.num_threads)
      log(f, "")
      log(f, "Number of iterations of BREADTH:", self.num_incremental())
      log(f, "Number of iterations of FINISHER:", self.num_finisher())
      log(f, "Success:", self.was_success())
      log(f, "Number of invariants synthesized:", self.total_number_of_invariants())
      log(f, "")

      for i in range(len(self.inc_results)):
        log(f, "BREADTH iteration", i,
            "time:", self.get_breadth_time(i), "seconds;",
            "cpu time:", self.get_breadth_cpu_time(i), "seconds")
      if self.finisher_result:
        log(f, "FINISHER",
            "time:", self.get_finisher_time(), "seconds;",
            "cpu time:", self.get_finisher_cpu_time(), "seconds")
      log(f, "total time:", self.get_total_time(), "seconds;",
          "total cpu time:", self.get_total_cpu_time(), "seconds")
