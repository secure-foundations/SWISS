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

    self.inc_results = []
    self.finisher_result = None

  def add_inc_log(self, iternum, log):
    while len(self.inc_logs) <= iternum:
      self.inc_logs.append([])
    self.inc_logs[-1].append(log)

  def add_inc_result(self, iternum, log):
    assert len(self.inc_results) == iternum
    self.inc_result_filenames.append(log)

  def add_finisher_log(self, filename):
    self.finisher_logs.append(filename)

  def add_finisher_result(self, filename):
    self.finisher_result_filename = filename

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
