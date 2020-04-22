import json
import os

# from https://stackoverflow.com/questions/2301789/read-a-file-in-reverse-order-using-python
def reverse_readline(filename, buf_size=8192):
    """A generator that returns the lines of a file in reverse order"""
    with open(filename) as fh:
        segment = None
        offset = 0
        fh.seek(0, os.SEEK_END)
        file_size = remaining_size = fh.tell()
        while remaining_size > 0:
            offset = min(file_size, offset + buf_size)
            fh.seek(file_size - offset)
            buffer = fh.read(min(remaining_size, buf_size))
            remaining_size -= buf_size
            lines = buffer.split('\n')
            # The first line of the buffer is probably not a complete line so
            # we'll save it and append it to the last line of the next buffer
            # we read
            if segment is not None:
                # If the previous chunk starts right from the beginning of line
                # do not concat the segment to the last line of new chunk.
                # Instead, yield the segment first 
                if buffer[-1] != '\n':
                    lines[-1] += segment
                else:
                    yield segment
            segment = lines[0]
            for index in range(len(lines) - 1, 0, -1):
                if lines[index]:
                    yield lines[index]
        # Don't yield None if the file was empty
        if segment is not None:
            yield segment

def parse_stats(filename):
  lines = []
  started = False
  for line in reverse_readline(filename):
    line = line.strip()
    if line == "=========================================":
      started = True
    elif line == "================= Stats =================":
      break
    elif started:
      lines.append(line)
  lines = lines[::-1]

  d = {}

  for l in lines:
    if (l.startswith("z3 [") or l.startswith("cvc4 [") or
        l.startswith("z3 TOTAL") or l.startswith("cvc4 TOTAL")):
      stuff = l.split()
      key = ' '.join(stuff[:-10])
      assert stuff[-10] == "total"
      ms_time = stuff[-9] + " ms"
      assert stuff[-8] == "ms"
      assert stuff[-7] == "over"
      ops = stuff[-6]
      assert stuff[-5] == "ops,"

      d[key + " time"] = ms_time
      d[key + " ops"] = ops
    elif ":" in l:
      t = l.split(':')
      key = t[0].strip()
      value = t[1].strip()
      d[key] = value
    else:
      assert False
  return d

def pad(k, n):
  while len(k) < 50:
    k += " "
  return k

def log_stats(f, stats, name):
  log(f, "")
  log(f, name)
  log(f, "-------------------------------------------------")
  for key in sorted(stats.keys()):
    if key != "progress":
      log(f, pad(key, 50) + " ---> " + stats[key])
  log(f, "-------------------------------------------------")

def add_stats(s1, s2):
  t1 = s1.split()
  t2 = s2.split()
  if len(t1) == 1:
    assert len(t2) == 1
    return str(int(t1[0]) + int(t2[0]))
  else:
    assert len(t1) == 2
    assert len(t2) == 2
    assert t1[1] == t2[1]
    return str(int(t1[0]) + int(t2[0])) + " " + t1[1]

def aggregate_stats(stats_list):
  d = {}
  for stats in stats_list:
    for key in stats:
      value = stats[key]
      if key in d:
        d[key] = add_stats(stats[key], d[key])
      else:
        d[key] = stats[key]
  return d

def log(f, *args):
  print(*args, file=f)

class Stats(object):
  def __init__(self, num_threads, all_args, ivy_filename, json_filename, logfile):
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

    self.logfile_base = logfile

  def add_inc_log(self, iternum, log, seconds):
    while len(self.inc_logs) <= iternum:
      self.inc_logs.append([])
      self.inc_individual_times.append([])
    self.inc_logs[-1].append(log)
    self.inc_individual_times[-1].append(seconds)

  def add_inc_result(self, iternum, log, seconds):
    #print("inc_result_filenames", self.inc_result_filenames)
    #print("iternum", iternum)
    assert len(self.inc_result_filenames) == iternum
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
      if res["success"]:
        return True
    if self.finisher_result and self.finisher_result["success"]:
      return True
    return False

  def total_number_of_invariants_incremental(self):
    if len(self.inc_results) > 0:
      return len(self.inc_results[-1]["formulas"])
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

  def get_breadth_time(self, i):
    return self.inc_times[i]

  def get_breadth_cpu_time(self, i):
    return sum(self.inc_individual_times[i])

  def get_breadth_individual_times_string(self, i):
    return " ".join(str(x) for x in self.inc_individual_times[i])

  def get_finisher_time(self):
    return self.finisher_time

  def get_finisher_cpu_time(self):
    return sum(self.finisher_individual_times)

  def get_finisher_individual_times_string(self):
    return " ".join(str(x) for x in self.finisher_individual_times)

  def get_total_time(self):
    t = 0
    for i in range(len(self.inc_results)):
      t += self.get_breadth_time(i)
    t += self.get_finisher_time()
    return t

  def get_total_cpu_time(self):
    t = 0
    for i in range(len(self.inc_results)):
      t += self.get_breadth_cpu_time(i)
    t += self.get_finisher_cpu_time()
    return t

  def stats_finisher(self, f):
    stats_list = []
    for log in self.finisher_logs:
      stats_list.append(parse_stats(log))
    total = aggregate_stats(stats_list)
    log_stats(f, total, "finisher")
    return total

  def stats_inc_one(self, f, i):
    stats_list = []
    for log in self.inc_logs[i]:
      stats_list.append(parse_stats(log))
    total = aggregate_stats(stats_list)
    log_stats(f, total, "breadth (" + str(i) + ")")
    return total

  def stats_inc_total(self, f):
    stats_list = []
    for i in range(len(self.inc_logs)):
      stats_list.append(self.stats_inc_one(f, i))
    total = aggregate_stats(stats_list)
    log_stats(f, total, "breadth (total)")
    return total

  def stats_total(self, f):
    stats_list = []
    stats_list.append(self.stats_inc_total(f))
    stats_list.append(self.stats_finisher(f))
    total = aggregate_stats(stats_list)
    log_stats(f, total, "total")
    return total

  def print_stats(self, filename):
    self.read_result_files()
    with open(filename, "w") as f:
      log(f, "logs", self.logfile_base)
      log(f, "")
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
            "cpu time:", self.get_breadth_cpu_time(i), "seconds;",
            "thread times: ", self.get_breadth_individual_times_string(i))
      if self.finisher_result:
        log(f, "FINISHER",
            "time:", self.get_finisher_time(), "seconds;",
            "cpu time:", self.get_finisher_cpu_time(), "seconds",
            "thread times:", self.get_finisher_individual_times_string())
      log(f, "total time:", self.get_total_time(), "seconds;",
          "total cpu time:", self.get_total_cpu_time(), "seconds")

      self.stats_total(f)
