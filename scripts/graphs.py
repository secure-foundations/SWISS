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

def get_files_with_prefix(input_directory, prefix):
  files = []
  for filename in os.listdir(input_directory):
    if filename.startswith(prefix):
      files.append(filename)
  return files

def make_nonacc_cmp_graph(ax, input_directory):
  a = get_files_with_prefix(input_directory, "paxos_breadth_seed_")
  b = get_files_with_prefix(input_directory, "nonacc_paxos_breadth_seed_")
  columns = (
    [(a[i], i+1) for i in range(len(a))] +
    [(b[i], len(a) + 2 + i) for i in range(len(b))]
  )
  ax.set_title("accumulation (paxos)")
  make_segmented_graph(ax, input_directory, "", "", columns=columns)

def make_parallel_graph(ax, input_directory, name, include_smt_times, graph_cex_count=False, graph_inv_count=False):
  ax.set_title("parallel " + name)
  make_segmented_graph(ax, input_directory, name, "_t", True, include_smt_times, 
      graph_cex_count=graph_cex_count, graph_inv_count=graph_inv_count)

def make_seed_graph(ax, input_directory, name, include_smt_times):
  ax.set_title("seed " + name)
  make_segmented_graph(ax, input_directory, name, "_seed_", True, include_smt_times)

red = '#ff4040'
lightred = '#ff8080'

blue = '#4040ff'
lightblue = '#8080ff'

orange = '#ffa500'
lightorange = '#ffd8b1'

green = '#00ff00'

def slightly_darken(color):
  if color == red:
    return '#b02020'
  if color == lightred:
    return '#b06060'
  if color == blue:
    return '#6060b0'
  if color == lightblue:
    return '#6060b0'
  if color == orange:
    return '#ff8000'
  if color == lightorange:
    return '#ffa060'
  assert False

def group_by_thread(s):
  res = []
  a = 0
  while a < len(s):
    b = a + 1
    while b < len(s) and s[b][0] == s[a][0]:
      b += 1
    res.append((s[a][0], sorted([s[i][2] for i in range(a, b)])))
    a = b
  return res

def make_segmented_graph(ax, input_directory, name, suffix, include_threads=False, include_smt_times=False, columns=None, graph_cex_count=False, graph_inv_count=False):
  if graph_cex_count or graph_inv_count:
    include_smt_times = True

  if columns is None:
    columns = []
    for filename in os.listdir(input_directory):
      if filename.startswith(name + suffix):
        idx = int(filename[len(name + suffix) : ])
        columns.append((filename, idx))

  for (filename, idx) in columns:
      with open(os.path.join(input_directory, filename)) as f:
        logpath = None
        for line in f:
          if line.startswith("logs "):
            logpath = line.split()[1]
            break
      assert logpath != None

      times = []
      thread_times = []
      colors = []
      smt_times = []

      odd2 = True
      with open(os.path.join(logpath)) as f:
        stuff = []
        for line in f:
          if line.startswith("complete iteration."):
            lsplit = line.split()
            task_name = lsplit[1].split('.')
            secs = lsplit[2]
            assert secs[0] == '('
            secs = int(secs[1:])

            iternum = int(task_name[1])
            if task_name[2] == 'size':
              size = int(task_name[3])
              if task_name[4] == 'thread':
                thr = int(task_name[5])
              else:
                continue
            elif task_name[2] == 'thread':
              size = None
              thr = int(task_name[3])
            else:
              continue

            odd1 = iternum % 2 == 0
            inner_odd = False if size == None else (size % 2 == 0)

            stuff.append((('breadth', iternum, size), thr, secs))
          elif line.startswith("complete finisher.thread."):
            lsplit = line.split()
            task_name = lsplit[1].split('.')
            secs = lsplit[2]
            assert secs[0] == '('
            secs = int(secs[1:])
            thr = int(task_name[2])
            stuff.append((('finisher', None, None), thr, secs))
        stuff = group_by_thread(stuff)
        for (info, secs) in stuff:
          times.append(max(secs))
          thread_times.append(secs)
          if info[0] == 'breadth':
            iternum = info[1]
            size = info[2]
            odd1 = iternum % 2 == 0
            inner_odd = False if size == None else size % 2 == 0
            if odd1:
              colors.append(red if inner_odd else lightred)
            else:
              colors.append(orange if inner_odd else lightorange)
            if include_smt_times:
              log_name_for_thread = "iteration." + str(iternum) if size == None else "iteration." + str(iternum) + ".size." + str(size)
              smt_times.append(get_smt_time(input_directory, filename, log_name_for_thread))
          else:
            colors.append(blue)
            if include_smt_times:
              smt_times.append(get_smt_time(input_directory, filename, "finisher"))

      #if include_threads:
      #  smt_times = get_smt_times(input_directory, filename)

      bottom = 0
      for i in range(len(times)):
        if graph_cex_count or graph_inv_count:
          (smt_sec, filtering_sec, cex_sec, cc, ic) = smt_times[i]
          c = (cc if graph_cex_count else ic)
          ax.bar(idx, c, bottom=bottom, color=colors[i])
          bottom += c
        else:
          t = times[i]

          if include_smt_times:
            ax.bar(idx - 0.2, t, bottom=bottom, width=0.4, color=colors[i])
          else:
            ax.bar(idx, t, bottom=bottom, color=colors[i])

          if include_smt_times:
            (smt_sec, filtering_sec, cex_sec, cex_count, inv_count) = smt_times[i]
            a = bottom
            b = bottom + smt_sec
            c = bottom + smt_sec + filtering_sec
            d = bottom + smt_sec + filtering_sec + cex_sec
            e = bottom + t
            ax.bar(idx + 0.2, b-a, bottom=a, width=0.4, color='black')
            ax.bar(idx + 0.2, c-b, bottom=b, width=0.4, color='green')
            ax.bar(idx + 0.2, d-c, bottom=c, width=0.4, color='red')
            ax.bar(idx + 0.2, e-d, bottom=d, width=0.4, color='gray')

          if include_threads:
            for j in range(len(thread_times[i])):
              thread_time = thread_times[i][j]
              m = len(thread_times[i])
              if include_smt_times:
                ax.bar(idx - 0.4 + 0.4 * (j / float(m)) + 0.2 / float(m),
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
  filtering_ms = None
  cex_ms = None
  inv_count = None
  cex_count_t = None
  cex_count_f = None
  cex_count_i = None
  new_inv_count = 0
  with open(logpath + "." + iteration_key + ".thread." + str(thread_to_use)) as f:
    for line in f:
      if line.startswith("SMT result ("):
        t = int(line.split()[-2])
        ms += t
      elif line.startswith("total time filtering: "):
        filtering_ms = int(line.split()[3])
      elif line.startswith("total time addCounterexample: "):
        cex_ms = int(line.split()[3])
      elif line.startswith("Counterexamples of type TRUE:"):
        cex_count_t = int(line.split()[-1])
      elif line.startswith("Counterexamples of type FALSE:"):
        cex_count_f = int(line.split()[-1])
      elif line.startswith("Counterexamples of type TRANSITION:"):
        cex_count_i = int(line.split()[-1])
      elif line.startswith("number of redundant invariants found:"):
        inv_count = int(line.split()[-1])
      elif line.startswith("found new invariant! all so far:"):
        new_inv_count += 1
  assert filtering_ms != None
  assert cex_ms != None
  assert inv_count != None
  assert cex_count_t != None
  assert cex_count_f != None
  assert cex_count_i != None

  print(new_inv_count)

  cex_count = cex_count_t + cex_count_f + cex_count_i
        
  return (ms / 1000, filtering_ms / 1000, cex_ms / 1000, cex_count, inv_count + new_inv_count)
  
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

  fig, ax = plt.subplots(nrows=3, ncols=5, figsize=[6, 8])

  make_parallel_graph(ax.flat[0], input_directory, "paxos_breadth", False)
  make_parallel_graph(ax.flat[1], input_directory, "paxos_implshape_finisher", False)
  #make_seed_graph(ax.flat[2], input_directory, "learning_switch", False)
  #make_seed_graph(ax.flat[3], input_directory, "paxos_breadth", False)

  make_parallel_graph(ax.flat[5], input_directory, "paxos_breadth", False)
  make_parallel_graph(ax.flat[6], input_directory, "paxos_implshape_finisher", True)
  #make_seed_graph(ax.flat[7], input_directory, "learning_switch", True)
  #make_seed_graph(ax.flat[8], input_directory, "paxos_breadth", True)

  make_opt_comparison_graph(ax.flat[10], input_directory, False)
  make_opt_comparison_graph(ax.flat[11], input_directory, True)

  #make_nonacc_cmp_graph(ax.flat[12], input_directory)

  make_seed_graph(ax.flat[2], input_directory, "paxos_breadth", True)
  make_parallel_graph(ax.flat[7], input_directory, "paxos_breadth", True)
  make_parallel_graph(ax.flat[12], input_directory, "nonacc_paxos_breadth", False)

  make_parallel_graph(ax.flat[8], input_directory, "paxos_breadth", True, graph_cex_count=True)
  make_parallel_graph(ax.flat[13], input_directory, "nonacc_paxos_breadth", False, graph_cex_count=True)

  make_parallel_graph(ax.flat[9], input_directory, "paxos_breadth", True, graph_inv_count=True)
  make_parallel_graph(ax.flat[14], input_directory, "nonacc_paxos_breadth", False, graph_inv_count=True)

  #plt.savefig(os.path.join(output_directory, 'graphs.png'))
  plt.show()

if __name__ == '__main__':
  #directory = sys.argv[1]
  #input_directory = os.path.join("paperlogs", directory)
  #make_table(input_directory)
  main()
