import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path
import sys
import os

class BasicStats(object):
  def __init__(self, input_directory, name, filename):
    self.name = name
    with open(os.path.join(input_directory, filename)) as f:
      doing_total = False
      self.z3_sat_ops = 0
      self.z3_sat_time = 0
      self.z3_unsat_ops = 0
      self.z3_unsat_time = 0
      self.cvc4_sat_ops = 0
      self.cvc4_sat_time = 0
      self.cvc4_unsat_ops = 0
      self.cvc4_unsat_time = 0
      self.breadth_total_time_sec = 0
      self.finisher_time_sec = 0
      for line in f:
        if line.strip() == "total":
          doing_total = True
        if line.startswith("total time: "):
          self.total_time_sec = int(line.split()[2])
        elif line.startswith("Number of threads: "):
          self.num_threads = int(line.split()[3])
        elif line.startswith("Number of invariants synthesized: "):
          self.num_inv = int(line.split()[4])
        elif line.startswith("Number of iterations of BREADTH: "):
          self.num_breadth_iters = int(line.split()[5])
        elif line.startswith("Number of iterations of FINISHER: "):
          self.num_finisher_iters = int(line.split()[5])
        elif line.startswith("FINISHER time: "):
          self.finisher_time_sec= int(line.split()[2])
        elif line.startswith("BREADTH iteration "):
          self.breadth_total_time_sec += int(line.split()[4])
        elif doing_total:
          if line.startswith("Counterexamples of type FALSE"):
            self.cex_false = int(line.split()[-1])
          elif line.startswith("Counterexamples of type TRUE"):
            self.cex_true = int(line.split()[-1])
          elif line.startswith("Counterexamples of type TRANSITION"):
            self.cex_trans = int(line.split()[-1])
          elif line.startswith("number of redundant invariants found"):
            self.num_redundant = int(line.split()[-1])
          elif line.startswith("z3 TOTAL sat ops"):
            self.z3_sat_ops = int(line.split()[-1])
          elif line.startswith("z3 TOTAL sat time"):
            self.z3_sat_time = int(line.split()[-2])
          elif line.startswith("z3 TOTAL unsat ops"):
            self.z3_unsat_ops = int(line.split()[-1])
          elif line.startswith("z3 TOTAL unsat time"):
            self.z3_unsat_time = int(line.split()[-2])
          elif line.startswith("cvc4 TOTAL sat ops"):
            self.cvc4_sat_ops = int(line.split()[-1])
          elif line.startswith("cvc4 TOTAL sat time"):
            self.cvc4_sat_time = int(line.split()[-2])
          elif line.startswith("cvc4 TOTAL unsat ops"):
            self.cvc4_unsat_ops = int(line.split()[-1])
          elif line.startswith("cvc4 TOTAL unsat time"):
            self.cvc4_unsat_time = int(line.split()[-2])

    self.smt_time_sec = (self.z3_sat_time + self.z3_unsat_time + self.cvc4_sat_time + self.cvc4_unsat_time) // 1000
    self.smt_ops = self.z3_sat_ops + self.z3_unsat_ops + self.cvc4_sat_ops + self.cvc4_unsat_ops

    self.b_size = -1
    self.f_size = -1
    self.num_valid_finisher_candidates = -1

def make_table(input_directory):
  s = [
    BasicStats(input_directory, "Leader election (1)", "mm_leader_election_breadth"),
    BasicStats(input_directory, "Leader election (2)", "mm_leader_election_fin"),
    BasicStats(input_directory, "Two-phase commit", "mm_2pc"),
    BasicStats(input_directory, "Lock server", "mm_lock_server"),
    BasicStats(input_directory, "Learning switch", "mm_learning_switch"),
    BasicStats(input_directory, "Paxos", "mm_paxos"),
    BasicStats(input_directory, "Flexible Paxos", "mm_flexible_paxos"),
  ]
  columns = [
    ('Benchmark', 'name'),
    ('$t$', 'total_time_sec'),
    ('$B_i$', 'num_breadth_iters'),
    ('$F_i$', 'num_finisher_iters', 'checkmark'),
    ('$|\\B|$', 'b_size', 'b_only'),
    ('$|\\F|$', 'f_size', 'f_only'),
    #('Breadth time (sec)', 'breadth_total_time_sec'),
    #('Finisher time (sec)', 'finisher_time_sec'),
    ('$B_t$', 'breadth_total_time_sec', 'b_only'),
    ('$F_t$', 'finisher_time_sec', 'f_only'),
    ('\\cextrue', 'cex_true'),
    ('\\cexfalse', 'cex_false'),
    ('\\cexind', 'cex_trans'),
    ('$r$', 'num_redundant', 'b_only'),
    ('SMT calls', 'smt_ops'),
    ('SMT time (sec)', 'smt_time_sec'),
    ('$m$', 'num_inv'),
  ]
  print("\\begin{tabular}{" + ('|l' * len(columns)) + "|}")
  print("\\hline")
  for i in range(len(columns)):
    print(columns[i][0], "\\\\" if i == len(columns) - 1 else "&", end=" ")
  print("")
  print("\\hline")
  for bench in s:
    for i in range(len(columns)):
      col = columns[i]
      prop = str(getattr(bench, col[1]))
      if len(col) >= 3 and col[2] == 'checkmark':
        prop = ("$\\checkmark$" if prop == "1" else "")
      if len(col) >= 3 and col[2] == 'b_only' and bench.num_breadth_iters == 0:
        prop = ""
      if len(col) >= 3 and col[2] == 'f_only' and bench.num_finisher_iters == 0:
        prop = ""
      print(prop, "\\\\" if i == len(columns) - 1 else "&", end=" ")
    print("")
    print("\\hline")
  print("\\end{tabular}")

def make_parallel_graph(ax, input_directory, name, include_smt_times):
  ax.set_title("parallel " + name)
  make_segmented_graph(ax, input_directory, name, "_t", True, include_smt_times)

def make_seed_graph(ax, input_directory, name, include_smt_times):
  ax.set_title("seed " + name)
  make_segmented_graph(ax, input_directory, name, "_seed_", True, include_smt_times)

red = '#ff4040'
lightred = '#ff8080'

blue = '#4040ff'
lightblue = '#8080ff'

def slightly_darken(color):
  if color == red:
    return '#b02020'
  if color == lightred:
    return '#b06060'
  if color == blue:
    return '#6060b0'
  if color == lightblue:
    return '#6060b0'
  assert False

def make_segmented_graph(ax, input_directory, name, suffix, include_threads=False, include_smt_times=False):
  for filename in os.listdir(input_directory):
    if filename.startswith(name + suffix):
      idx = int(filename[len(name + suffix) : ])
      times = []
      thread_times = []
      colors = []
      smt_times = []

      odd1 = True
      odd2 = True
      iternum = 1
      with open(os.path.join(input_directory, filename)) as f:
        for line in f:
          if line.startswith("BREADTH iteration "):
            times.append(int(line.split()[4]))
            thread_times.append([int(x) for x in line.split()[12:]])
            colors.append(red if odd1 else lightred)
            odd1 = not odd1
            if include_smt_times:
              smt_times.append(get_smt_time(input_directory, filename, "iteration." + str(iternum)))
            iternum += 1
          elif line.startswith("FINISHER time: "):
            times.append(int(line.split()[2]))
            thread_times.append([int(x) for x in line.split()[10:]])
            colors.append(blue if odd2 else lightblue)
            odd2 = not odd2
            if include_smt_times:
              smt_times.append(get_smt_time(input_directory, filename, "finisher"))

      #if include_threads:
      #  smt_times = get_smt_times(input_directory, filename)

      bottom = 0
      for i in range(len(times)):
        t = times[i]
        color = 'red' if i % 2 == 0 else 'black'

        if include_smt_times:
          ax.bar(idx - 0.2, t, bottom=bottom, width=0.4, color=colors[i])
        else:
          ax.bar(idx, t, bottom=bottom, color=colors[i])

        if include_smt_times:
          ax.bar(idx + 0.2, smt_times[i], bottom=bottom, width=0.4, color='black')
          ax.bar(idx + 0.2, t-smt_times[i], bottom=bottom+smt_times[i], width=0.4, color='gray')

        if include_threads:
          for j in range(len(thread_times[i])):
            thread_time = thread_times[i][j]
            m = len(thread_times[i])
            if include_smt_times:
              ax.bar(idx - 0.4 + 0.4 * (j / float(m)) + 0.8 / float(m),
                  thread_time, bottom=bottom, width = 0.4 / float(m),
                  color=slightly_darken(colors[i]))
            else:
              ax.bar(idx - 0.4 + 0.8 * (j / float(m)) + 0.4 / float(m),
                  thread_time, bottom=bottom, width = 0.8 / float(m),
                  color=slightly_darken(colors[i]))

        bottom += t

def get_smt_time(input_directory, filename, iteration_key):
  with open(os.path.join(input_directory, filename)) as f:
    logpath = None
    threads = None
    for line in f:
      if line.startswith("logs "):
        logpath = line.split()[1]
      if line.startswith("Number of threads: "):
        threads = int(line.split()[3])
      if logpath != None and threads != None:
        break
    assert logpath != None
    assert threads != None

  thread_to_use = None
  with open(logpath) as f:
    for line in f:
      if line.startswith("complete " + iteration_key + ".thread."):
        l = line.split()[1]
        thread_to_use = int(l.split('.')[-1])

  ms = 0
  with open(logpath + "." + iteration_key + ".thread." + str(thread_to_use)) as f:
    for line in f:
      if line.startswith("SMT result ("):
        t = int(line.split()[-2])
        ms += t
  return ms / 1000
  
#def get_smt_times(input_directory, filename):
#  with open(os.path.join(input_directory, filename)) as f:
#    times = []
#    cur = None
#    for line in f:
#      line = line.strip()
#      if line == "-------------------------------------------------":
#        if cur == None:
#          cur = 0
#        else:
#          times.append(cur)
#          cur = None
#      elif cur != None:
#        t = line.split('--->')
#        key = t[0].strip()
#        value = t[1].strip()
#        if key in (
#          'z3 TOTAL sat time',
#          'z3 TOTAL unsat time',
#          'cvc4 TOTAL sat time',
#          'cvc4 TOTAL unsat time'):
#          cur += int(value.split()[0])
#  return [t/1000 for t in times]

def get_total_time(input_directory, filename):
  with open(os.path.join(input_directory, filename)) as f:
    for line in f:
      if line.startswith("total time: "):
        t = int(line.split()[2])
        return t

def make_opt_comparison_graph(ax, input_directory, large_ones):
  opts = ['mm_', 'postbmc_mm_', 'prebmc_mm_', 'postbmc_prebmc_mm_']
  if large_ones:
    probs = ['flexible_paxos', 'learning_switch', 'paxos']
  else:
    probs = ['2pc', 'leader_election_breadth', 'leader_election_fin', 'lock_server']
  idx = 0
  for prob in probs:
    idx += 1
    opt_idx = -1
    for opt in opts:
      opt_idx += 1

      name = opt + prob
      t = get_total_time(input_directory, name)
      ax.bar(idx - 0.4 + 0.4/len(opts) + 0.8/len(opts) * opt_idx, t,
          bottom=0, width=0.8/len(opts), color='black')
  

def main():
  directory = sys.argv[1]
  input_directory = os.path.join("paperlogs", directory)
  output_directory = os.path.join("graphs", directory)

  Path(output_directory).mkdir(parents=True, exist_ok=True)

  fig, ax = plt.subplots(nrows=3, ncols=4, figsize=[6, 8])

  #make_parallel_graph(ax.flat[0], input_directory, "paxos_breadth", False)
  #make_parallel_graph(ax.flat[1], input_directory, "paxos_implshape_finisher", False)
  #make_seed_graph(ax.flat[2], input_directory, "learning_switch", False)
  #make_seed_graph(ax.flat[3], input_directory, "paxos", False)

  #make_parallel_graph(ax.flat[4], input_directory, "paxos_breadth", True)
  #make_parallel_graph(ax.flat[5], input_directory, "paxos_implshape_finisher", True)
  #make_seed_graph(ax.flat[6], input_directory, "learning_switch", True)
  #make_seed_graph(ax.flat[7], input_directory, "paxos", True)

  make_opt_comparison_graph(ax.flat[8], input_directory, False)
  make_opt_comparison_graph(ax.flat[9], input_directory, True)

  #plt.savefig(os.path.join(output_directory, 'graphs.png'))
  plt.show()

if __name__ == '__main__':
  directory = sys.argv[1]
  input_directory = os.path.join("paperlogs", directory)
  make_table(input_directory)
  #main()
