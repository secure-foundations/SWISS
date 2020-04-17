import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path
import sys
import os

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
  main()
