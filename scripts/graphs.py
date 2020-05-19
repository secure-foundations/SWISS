import matplotlib.pyplot as plt
import matplotlib.patches as patches
import numpy as np
from pathlib import Path
import sys
import os

class ThreadStats(object):
  def __init__(self, input_directory, filename):
    times = {}
    stats = {}
    with open(os.path.join(input_directory, filename)) as f:
      cur_name = None
      cur_stats = None
      state = 0
      for line in f:
        if line.startswith("time for process "):
          l = line.split()
          name = self.get_name_from_log(l[3])
          secs = int(l[-2])
          times[name] = secs
        elif state == 0 and line.startswith("./logs/log."):
          cur_name = self.get_name_from_log(line.strip())
          state = 1
        elif state == 1:
          assert line == "-------------------------------------------------\n"
          state = 2
          cur_stats = {}
        elif state == 2 and line == "-------------------------------------------------\n":
          state = 0
          stats[cur_name] = cur_stats
        elif state == 2:
          l = line.split('--->')
          assert len(l) == 2
          key = l[0].strip()
          val = int(l[1].split()[0])
          cur_stats[key] = val
    for k in times:
      stats[k]["total_time"] = times[k]

    self.uses_sizes = False
    for k in times:
      if ".size." in k:
        self.uses_sizes = True

    self.stats = stats

  def get_finisher_stats(self):
    res = []
    for k in self.stats:
      if k.startswith("finisher.thread."):
        res.append(self.stats[k])
    return res

  def get_breadth_stats(self):
    res = []
    for k in self.stats:
      if k.startswith("iteration."):
        task_name = k.split('.')
        iternum = int(task_name[1])
        if task_name[2] == 'size':
          size = int(task_name[3])
          if task_name[4] == 'thread':
            thr = int(task_name[5])
          else:
            assert False
        elif task_name[2] == 'thread':
          size = 1
          thr = int(task_name[3])
        else:
          assert False

        while len(res) < iternum:
          res.append([])
        while len(res[iternum-1]) < size:
          res[iternum-1].append([])
        res[iternum-1][size-1].append(self.stats[k])
    return res

  def get_name_from_log(self, log):
    assert log.startswith("./logs/log.")
    return '.'.join(log.split('.')[5:])

class Breakdown(object):
  def __init__(self, stats, skip=False):
    if skip:
      return

    self.stats = stats
    self.total_smt_ms = 0
    for k in stats:
      if "TOTAL sat time" in k or "TOTAL unsat time" in k:
        self.total_smt_ms += stats[k]
    self.total_smt_secs = self.total_smt_ms / 1000

    self.filter_secs = stats["total time filtering"] / 1000
    self.cex_secs = stats["total time processing counterexamples"] / 1000
    self.nonredundant_secs = stats["total time processing nonredundant"] / 1000
    self.redundant_secs = stats["total time processing redundant"] / 1000

    self.total_cex = (
      stats["Counterexamples of type FALSE"] +
      stats["Counterexamples of type TRUE"] +
      stats["Counterexamples of type TRANSITION"]
    )

    self.total_invs = (
      stats["number of non-redundant invariants found"] +
      stats["number of redundant invariants found"]
    )

  def add(self, other):
    b = Breakdown(None, skip=True)
    for attr in (
        "total_smt_ms",
        "filter_secs",
        "cex_secs",
        "nonredundant_secs",
        "redundant_secs",
        "total_cex"):
      setattr(b, attr, getattr(self, attr) + getattr(other, attr))
    b.stats = {"total_time": self.stats["total_time"] + other.stats["total_time"]}
    return b

def sum_breakdowns(l):
  t = l[0]
  for i in range(1, len(l)):
    t = t.add(l[i])
  return t

def get_longest(stat_list):
  assert len(stat_list) > 0
  t = stat_list[0]
  for l in stat_list:
    if l["total_time"] > t["total_time"]:
      t = l
  return t

class BasicStats(object):
  def __init__(self, input_directory, name, filename, I4=None):
    self.I4_time = I4
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
      self.total_time_filtering_ms = 0
      self.total_inductivity_check_time_ms = 0
      self.num_inductivity_checks = 0
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
          elif line.startswith("total time filtering"):
            self.total_time_filtering_ms = int(line.split()[-2])

          if "[init-check] sat ops" in line:
            self.num_inductivity_checks += int(line.split()[-1])
          elif "[init-check] unsat ops" in line:
            self.num_inductivity_checks += int(line.split()[-1])


          if ("[inductivity-check: " in line or "[init-check]" in line):
            s = line.split()
            if s[-1] == "ms" and s[-4] == "time":
              self.total_inductivity_check_time_ms += int(s[-2])

    self.smt_time_sec = (self.z3_sat_time + self.z3_unsat_time + self.cvc4_sat_time + self.cvc4_unsat_time) // 1000
    self.smt_ops = self.z3_sat_ops + self.z3_unsat_ops + self.cvc4_sat_ops + self.cvc4_unsat_ops

    self.total_inductivity_check_time_sec = (
        self.total_inductivity_check_time_ms / 1000)

    self.total_time_filtering_sec = (
        self.total_time_filtering_ms / 1000)

    if filename == 'mm_leader_election_breadth':
      self.b_size = 4490
      self.f_size = -1
    elif filename == 'mm_leader_election_fin':
      self.b_size = -1
      self.f_size = 56915730
    elif filename == "mm_2pc":
      self.b_size = 3739
      self.f_size = -1
    elif filename == "mm_lock_server":
      self.b_size = -1
      self.f_size = 11
    elif filename == "mm_learning_switch":
      self.b_size = 2259197
      self.f_size = -1
    elif filename == "mm_paxos":
      self.b_size = 3435314 + 47972
      self.f_size = 232460599446
    elif filename == "paxos_depth2_finisher_t1":
      self.b_size = 0
      self.f_size = 232460599446
    elif filename == "mm_flexible_paxos":
      self.b_size = 3435314 + 47972
      self.f_size = 232460599446
    elif filename == "mm_multi_paxos":
      self.b_size = 8439183 + 26290
      self.f_size = 33589418704
    elif filename == "mm_sdl":
      self.b_size = -1
      self.f_size = 87858803
    else:
      assert False, "don't have space-size numbers for " + filename

    self.f_b_size = self.f_size + self.b_size

    self.num_valid_finisher_candidates = -1

def make_table(input_directory, which):
  s = [
    BasicStats(input_directory, "Simple decentralized lock", "mm_sdl"),
    BasicStats(input_directory, "Leader election (1)", "mm_leader_election_breadth", I4='6.1'),
    BasicStats(input_directory, "Leader election (2)", "mm_leader_election_fin", I4='6.1'),
    BasicStats(input_directory, "Two-phase commit", "mm_2pc", I4='4.3'),
    BasicStats(input_directory, "Lock server", "mm_lock_server", I4='0.8'),
    BasicStats(input_directory, "Learning switch", "mm_learning_switch", I4='10.7'),
    BasicStats(input_directory, "Paxos", "mm_paxos"),
    BasicStats(input_directory, "Flexible Paxos", "mm_flexible_paxos"),
    BasicStats(input_directory, "Multi-Paxos", "mm_multi_paxos"),
  ]
  if which == 0:
    columns = [
      ('Benchmark', 'name'),
      ('Threads', 'num_threads'),
      ('$B_i$', 'num_breadth_iters'),
      ('$F_i$', 'num_finisher_iters', 'checkmark'),
      ('$|\\B|$', 'b_size', 'b_only'),
      ('$|\\F|$', 'f_size', 'f_only'),
      ('$t$', 'total_time_sec'),
      #('Breadth time (sec)', 'breadth_total_time_sec'),
      #('Finisher time (sec)', 'finisher_time_sec'),
      ('$B_t$', 'breadth_total_time_sec', 'b_only'),
      ('$F_t$', 'finisher_time_sec', 'f_only'),
      ('I4', 'I4_time'),
    ]
  else:
    columns = [
      ('Benchmark', 'name'),
      ('\\cextrue', 'cex_true'),
      ('\\cexfalse', 'cex_false'),
      ('\\cexind', 'cex_trans'),
      ('$r$', 'num_redundant', 'b_only'),
      ('SMT calls', 'smt_ops'),
      ('SMT time (sec)', 'smt_time_sec'),
      ('$m$', 'num_inv'),
    ]
  print("\\begin{tabular}{" + ('|l' * (len(columns)-1)) + "||l|}")
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
      if col[1] == 'I4_time' and prop == 'None':
        prop = ''
      print(prop, "\\\\" if i == len(columns) - 1 else "&", end=" ")
    print("")
    print("\\hline")
  print("\\end{tabular}")

def make_smt_stats_table(input_directory):
  s = [
    #BasicStats(input_directory, "Simple decentralized lock", "mm_sdl"),
    BasicStats(input_directory, "Paxos (Finisher)", 
        "paxos_depth2_finisher_t1")
  ]
  columns = [
    ('Benchmark', 'name'),
    #('smt ops', 'smt_ops'),
    ('smt', 'smt_time_sec'),
    ('smti', 'total_inductivity_check_time_sec'),
    ('I', 'num_inductivity_checks'),
    ('f', 'total_time_filtering_sec'),
    ('size', 'f_b_size'),
    ('I / sec', 'num_inductivity_checks', 'total_inductivity_check_time_sec'),
    ('F / sec', 'f_b_size', 'total_time_filtering_sec'),
    ('T / sec', 'f_b_size', 'total_time_sec'),
  ]
  print("\\begin{tabular}{" + ('|l' * (len(columns)-1)) + "|l|}")
  print("\\hline")
  for i in range(len(columns)):
    print(columns[i][0], "\\\\" if i == len(columns) - 1 else "&", end=" ")
  print("")
  print("\\hline")
  for bench in s:
    print(bench.total_time_filtering_sec)
    print(bench.total_time_sec)
    for i in range(len(columns)):
      col = columns[i]
      prop = str(getattr(bench, col[1]))
      if len(col) == 3:
        prop2 = str(getattr(bench, col[2]))
        prop = str(float(prop) / float(prop2))
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
    [(a[i], i+1, i+1) for i in range(len(a))] +
    [(b[i], len(a) + 2 + i, i+1) for i in range(len(b))]
  )
  ax.set_title("accumulation (paxos)")
  make_segmented_graph(ax, input_directory, "", "", columns=columns)

def make_parallel_graph(ax, input_directory, name, include_threads=False, include_breakdown=False, graph_cex_count=False, graph_inv_count=False, collapse_size_split=False, collapsed_breakdown=False):
  suffix = ''
  if graph_cex_count:
    suffix = ' (cex)'
  if graph_inv_count:
    suffix = ' (invs)'
  
  if name == "paxos_depth2_finisher":
    ax.set_title("Paxos Finisher")
  elif name == "paxos_breadth":
    ax.set_title("Paxos BreadthAccumulative")
  elif name == "nonacc_paxos_breadth":
    ax.set_title("Paxos Breadth")
  else:
    ax.set_title("parallel " + name + suffix)

  ax.set_ylabel("seconds")
  ax.set_xlabel("threads")
  make_segmented_graph(ax, input_directory, name, "_t", include_threads=include_threads, include_breakdown=include_breakdown, 
      collapsed_breakdown=collapsed_breakdown,
      graph_cex_count=graph_cex_count, graph_inv_count=graph_inv_count, collapse_size_split=collapse_size_split)

def make_seed_graph(ax, input_directory, name, title=None, include_threads=False, include_breakdown=False, skip_b=False, skip_f=False):
  if title is None:
    ax.set_title("seed " + name)
  else:
    ax.set_title(title)
    
  ax.set_ylabel("seconds")
  make_segmented_graph(ax, input_directory, name, "_seed_",
      include_threads=include_threads,
      include_breakdown=include_breakdown,
      skip_b=skip_b,
      skip_f=skip_f)
  ax.set_xticks([])

red = '#ff4040'
lightred = '#ff8080'

blue = '#4040ff'
lightblue = '#8080ff'

orange = '#ffa500'
lightorange = '#ffd8b1'

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

def make_segmented_graph(ax, input_directory, name, suffix,
        include_threads=False,
        include_breakdown=False,
        columns=None,
        graph_cex_count=False,
        graph_inv_count=False,
        skip_b=False,
        skip_f=False,
        collapse_size_split=False,
        collapsed_breakdown=False):
  if graph_cex_count or graph_inv_count:
    include_breakdown = True

  if columns is None:
    columns = []
    for filename in os.listdir(input_directory):
      if filename.startswith(name + suffix):
        idx = int(filename[len(name + suffix) : ])
        x_label = idx
        columns.append((filename, idx, x_label))

  ax.set_xticks([a[1] for a in columns])
  ax.set_xticklabels([a[2] for a in columns]) 

  for (filename, idx, label) in columns:
      ts = ThreadStats(input_directory, filename)

      times = []
      thread_times = []
      colors = []
      breakdowns = []

      stuff = []

      if collapse_size_split:
        assert not include_breakdown
        assert not include_threads

      if not skip_b:
        bs = ts.get_breadth_stats()
        for i in range(len(bs)):
          this_size_time = 0
          odd1 = i % 2 == 1
          for j in range(len(bs[i])):
            inner_odd = j % 2 == 1
            iternum = i+1
            size = (j+1 if ts.uses_sizes else None)
            time_per_thread = sorted([s["total_time"] for s in bs[i][j]])
            longest = get_longest(bs[i][j])

            if not collapse_size_split:
              thread_times.append(time_per_thread)
              breakdowns.append(Breakdown(longest))
              times.append(longest["total_time"])

              if odd1:
                colors.append(red if inner_odd else lightred)
              else:
                colors.append(orange if inner_odd else lightorange)

            this_size_time += longest["total_time"]

          if collapse_size_split:
            times.append(this_size_time)
            if odd1:
              colors.append(lightred)
            else:
              colors.append(lightorange)

      if not skip_f:
        fs = ts.get_finisher_stats()
        if len(fs) > 0:
          time_per_thread = sorted([s["total_time"] for s in fs])
          thread_times.append(time_per_thread)
          longest = get_longest(fs)
          breakdowns.append(Breakdown(longest))
          times.append(longest["total_time"])

          colors.append(blue)

      if collapsed_breakdown:
        assert not include_threads
        times = [sum(times)]
        breakdowns = [sum_breakdowns(breakdowns)]

      bottom = 0
      for i in range(len(times)):
        if graph_cex_count or graph_inv_count:
          breakdown = breakdowns[i]
          c = (breakdown.total_cex if graph_cex_count else breakdown.total_invs)
          ax.bar(idx, c, bottom=bottom, color=colors[i])
          bottom += c
        else:
          t = times[i]

          if not collapsed_breakdown:
            if include_breakdown:
              ax.bar(idx - 0.2, t, bottom=bottom, width=0.4, color=colors[i])
            else:
              ax.bar(idx, t, bottom=bottom, color=colors[i])

          if include_breakdown or collapsed_breakdown:
            breakdown = breakdowns[i]
            a = bottom
            b = a + breakdown.filter_secs
            c = b + breakdown.cex_secs
            d = c + breakdown.nonredundant_secs
            e = d + breakdown.redundant_secs
            top = bottom + breakdown.stats["total_time"]

            if collapsed_breakdown:
              center = idx
              width = 0.8
            else:
              center = idx + 0.2
              width = 0.4

            ax.bar(center, b-a, bottom=a, width=width, color='#b0ffb0')
            ax.bar(center, c-b, bottom=b, width=width, color='#ff2020')
            ax.bar(center, d-c, bottom=c, width=width, color='blue')
            ax.bar(center, e-d, bottom=d, width=width, color='#8080ff')
            ax.bar(center, top-e, bottom=e, width=width, color='gray')

          if not collapsed_breakdown:
            if include_threads:
              for j in range(len(thread_times[i])):
                thread_time = thread_times[i][j]
                m = len(thread_times[i])
                if include_breakdown:
                  ax.bar(idx - 0.4 + 0.4 * (j / float(m)) + 0.2 / float(m),
                      thread_time, bottom=bottom, width = 0.4 / float(m),
                      color=slightly_darken(colors[i]))
                else:
                  ax.bar(idx - 0.4 + 0.8 * (j / float(m)) + 0.4 / float(m),
                      thread_time, bottom=bottom, width = 0.8 / float(m),
                      color=slightly_darken(colors[i]))

          bottom += t

def get_total_time(input_directory, filename):
  with open(os.path.join(input_directory, filename)) as f:
    for line in f:
      if line.startswith("total time: "):
        t = int(line.split()[2])
        return t

def make_opt_comparison_graph(ax, input_directory, large_ones):
  ax.set_yscale('log')
  #opts = ['mm_', 'postbmc_mm_', 'prebmc_mm_', 'postbmc_prebmc_mm_']
  opts = ['', 'mm_', 'prebmc_', 'prebmc_mm_'] #, 'postbmc_']
  #if large_ones:
  #  probs = ['flexible_paxos', 'learning_switch', 'paxos']
  #else:
  #  probs = ['2pc', 'leader_election_breadth', 'leader_election_fin', 'lock_server']

  colors = ['black', '#ff8080', '#8080ff', '#80ff80']

  prob_data = [
      ('Leader election (1)',  'leader_election_breadth'),
      ('Leader election (2)',  'leader_election_fin'),
      ('Two-phase commit', '2pc'),
      #('Lock server', 'lock_server'),
      ('Learning switch', 'learning_switch'),
      ('Paxos', 'paxos'),
      ('Flexible Paxos', 'flexible_paxos'),
      ('Multi Paxos', 'multi_paxos'),
      #('Multi-Paxos', 'multi_paxos'),
  ]

  failures = ['prebmc_multi_paxos', 'multi_paxos']

  ax.set_xticks([i for i in range(1, len(prob_data) + 1)])
  ax.set_xticklabels([name for (name, prob) in prob_data])
  probs = [prob for (name, prob) in prob_data]

  for tick in ax.get_xticklabels():
      tick.set_rotation(90)

  patterns = [ None, "\\" , "/" , "+" ]

  idx = 0
  for prob in probs:
    idx += 1
    opt_idx = -1
    for (opt, color, pattern) in zip(opts, colors, patterns):
      opt_idx += 1

      name = opt + prob
      if name not in failures:
        t = get_total_time(input_directory, name)
        ax.bar(idx - 0.4 + 0.4/len(opts) + 0.8/len(opts) * opt_idx, t,
            bottom=0, width=0.8/len(opts), color=color, edgecolor='black') #hatch=pattern)

  p = []
  for (opt, color, pattern) in zip(opts, colors, patterns):
    opt_name = {
      '': "Baseline",
      'mm_': "Minimal models",
      'prebmc_': "BMC",
      'prebmc_mm_': "Minimal models + BMC",
    }[opt]
    p.append(patches.Patch(color=color, label=opt_name))
  ax.legend(handles=p)
  
def make_parallel_graphs(input_directory, save=False):
  output_directory = "graphs"
  Path(output_directory).mkdir(parents=True, exist_ok=True)

  fig, ax = plt.subplots(nrows=2, ncols=3, figsize=[15, 6])

  plt.gcf().subplots_adjust(bottom=0.20)

  ax.flat[1].set_ylim(bottom=0, top=500)
  ax.flat[2].set_ylim(bottom=0, top=500)
  ax.flat[4].set_ylim(bottom=0, top=500)
  ax.flat[5].set_ylim(bottom=0, top=500)

  make_parallel_graph(ax.flat[0], input_directory, "paxos_depth2_finisher")
  make_parallel_graph(ax.flat[1], input_directory, "paxos_breadth")
  make_parallel_graph(ax.flat[2], input_directory, "nonacc_paxos_breadth", collapse_size_split=True)

  make_parallel_graph(ax.flat[3], input_directory, "paxos_depth2_finisher", collapsed_breakdown=True)
  make_parallel_graph(ax.flat[4], input_directory, "paxos_breadth", collapsed_breakdown=True)
  make_parallel_graph(ax.flat[5], input_directory, "nonacc_paxos_breadth", collapsed_breakdown=True)

  plt.tight_layout()

  if save:
    plt.savefig(os.path.join(output_directory, 'paxos-parallel.png'))
  else:
    plt.show()

def make_opt_graphs_main(input_directory, save=False):
  output_directory = "graphs"
  Path(output_directory).mkdir(parents=True, exist_ok=True)

  fig, ax = plt.subplots(nrows=1, ncols=1, figsize=[6.5, 4.5])
  plt.gcf().subplots_adjust(bottom=0.45)
  ax.set_ylabel("seconds")

  make_opt_comparison_graph(ax, input_directory, True)

  if save:
    plt.savefig(os.path.join(output_directory, 'opt-comparison.png'))
  else:
    plt.show()

def make_seed_graphs_main(input_directory, save=False):
  output_directory = "graphs"
  Path(output_directory).mkdir(parents=True, exist_ok=True)

  fig, ax = plt.subplots(nrows=1, ncols=4, figsize=[12, 3])
  plt.gcf().subplots_adjust(bottom=0.20)

  make_seed_graph(ax.flat[0], input_directory, "learning_switch", title="Learning switch (BreadthAccumulative)")
  make_seed_graph(ax.flat[1], input_directory, "paxos", title="Paxos (BreadthAccumulative)", skip_f=True)
  make_seed_graph(ax.flat[2], input_directory, "paxos", title="Paxos (Finisher)", skip_b=True)
  make_seed_graph(ax.flat[3], input_directory, "wholespace_finisher_paxos", title="Paxos (Finisher, entire space)")

  fig.tight_layout()

  if save:
    plt.savefig(os.path.join(output_directory, 'seed.png'))
  else:
    plt.show()


def main():
  directory = sys.argv[1]
  input_directory = os.path.join("paperlogs", directory)
  output_directory = os.path.join("graphs", directory)

  Path(output_directory).mkdir(parents=True, exist_ok=True)

  fig, ax = plt.subplots(nrows=3, ncols=5, figsize=[6, 8])

  #make_parallel_graph(ax.flat[0], input_directory, "paxos_breadth", False)
  #make_parallel_graph(ax.flat[1], input_directory, "paxos_implshape_finisher", False)
  #make_seed_graph(ax.flat[2], input_directory, "learning_switch", False)
  #make_seed_graph(ax.flat[3], input_directory, "paxos_breadth", False)

  #make_parallel_graph(ax.flat[5], input_directory, "paxos_breadth", False)
  #make_parallel_graph(ax.flat[6], input_directory, "paxos_implshape_finisher", True)
  #make_seed_graph(ax.flat[7], input_directory, "learning_switch", True)
  #make_seed_graph(ax.flat[8], input_directory, "paxos_breadth", True)

  make_opt_comparison_graph(ax.flat[10], input_directory, False)
  make_opt_comparison_graph(ax.flat[11], input_directory, True)

  #make_nonacc_cmp_graph(ax.flat[12], input_directory)

  #make_seed_graph(ax.flat[2], input_directory, "paxos_breadth", True)
  #make_parallel_graph(ax.flat[7], input_directory, "paxos_breadth", True)
  #make_parallel_graph(ax.flat[12], input_directory, "nonacc_paxos_breadth", True)

  #make_parallel_graph(ax.flat[8], input_directory, "paxos_breadth", True, graph_cex_count=True)
  #make_parallel_graph(ax.flat[13], input_directory, "nonacc_paxos_breadth", False, graph_cex_count=True)

  #make_parallel_graph(ax.flat[9], input_directory, "paxos_breadth", True, graph_inv_count=True)
  #make_parallel_graph(ax.flat[14], input_directory, "nonacc_paxos_breadth", False, graph_inv_count=True)

  #plt.savefig(os.path.join(output_directory, 'graphs.png'))
  plt.show()

if __name__ == '__main__':
  directory = sys.argv[1]
  input_directory = os.path.join("paperlogs", directory)
  #make_table(input_directory)
  #main()
  #make_parallel_graphs(input_directory)
  #make_seed_graphs_main(input_directory)
  #make_smt_stats_table(input_directory)
  make_opt_graphs_main(input_directory)
