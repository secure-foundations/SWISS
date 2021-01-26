import matplotlib.pyplot as plt
import matplotlib.patches as patches
import numpy as np
from pathlib import Path
import matplotlib.transforms as mtrans
import sys
import os
import json
import paper_benchmarks

import get_counts
import templates

use_old_names = False

def map_filename_maybe(filename):
  if not use_old_names:
    return filename
  if "learning-switch-quad_pyv" in filename:
    return filename.replace("learning-switch-quad_pyv", "learning_switch_pyv")
  if "learning-switch-ternary" in filename:
    return filename.replace("learning-switch-ternary", "learning-switch")
  if "lock-server-sync" in filename:
    return filename.replace("lock-server-sync", "lock_server")
  if "lock-server-async" in filename:
    return filename.replace("lock-server-async", "lockserv")
  return filename

class ThreadStats(object):
  def __init__(self, input_directory, filename):
    times = {}
    stats = {}
    with open(os.path.join(input_directory, map_filename_maybe(filename), "summary")) as f:
      cur_name = None
      cur_stats = None
      state = 0
      for line in f:
        if line.startswith("time for process "):
          l = line.split()
          name = self.get_name_from_log(l[3])
          if l[-1] == "seconds":
            secs = float(l[-2])
          elif l[-2] == "seconds":
            secs = float(l[-3])
          else:
            assert False
          times[name] = secs
        elif state == 0 and (line.startswith("./logs/log.") or line.startswith("/pylon5/ms5pijp/tjhance/log.")):
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
    if use_old_names:
      if log.startswith("./logs/log."):
        name = '.'.join(log.split('.')[5:])
      elif log.startswith("/pylon5/ms5pijp/tjhance/log."):
        name = '.'.join(log.split('.')[4:])
      else:
        assert False
      return name
    else:
      if log.startswith("./logs/log."):
        name = log.split('/')[-1]
      elif log.startswith("/pylon5/ms5pijp/tjhance/log."):
        name = log.split('/')[-1]
      else:
        assert False
      return name

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

def get_longest_that_succeed(stat_list):
  assert len(stat_list) > 0
  t = stat_list[0]
  for l in stat_list:
    if l["total_time"] > t["total_time"] and l["number of finisher invariants found"] > 0:
      t = l
  return t


class BasicStats(object):

  def __init__(self, input_directory, name, filename, I4=None):
    self.I4_time = I4
    self.name = name
    self.filename = filename
    with open(os.path.join(input_directory, map_filename_maybe(filename), "summary")) as f:
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
      self.timed_out_6_hours = False
      self.global_stats = {}
      self.total_long_smtAllQueries_ops = 0
      self.total_long_smtAllQueries_ms = 0
      for line in f:
        if line.strip() == "total":
          doing_total = True
        if line.startswith("total time: "):
          self.total_time_sec = float(line.split()[2])
          self.total_cpu_time_sec = float(line.split()[7])
        elif line.startswith("Number of threads: "):
          self.num_threads = int(line.split()[3])
        elif line.startswith("Number of invariants synthesized: "):
          self.num_inv = int(line.split()[4])
        elif line.startswith("Number of iterations of BREADTH: "):
          self.num_breadth_iters = int(line.split()[5])
        elif line.startswith("Number of iterations of FINISHER: "):
          self.num_finisher_iters = int(line.split()[5])
        elif line.startswith("Success: True"):
          self.success = True
        elif line.startswith("Success: False"):
          self.success = False
        elif line.startswith("FINISHER time: "):
          self.finisher_time_sec= float(line.split()[2])
        elif line.startswith("BREADTH iteration "):
          self.breadth_total_time_sec += float(line.split()[4])
        elif line.strip() == "TIMED OUT 21600 seconds":
          self.timed_out_6_hours = True
          self.total_time_sec = 21600
        elif line.startswith("global_stats "):
          s = line.split()
          name = s[1]
          key = s[2]
          rest = s[3:]
          if name not in self.global_stats:
            self.global_stats[name] = {}
          self.global_stats[name][key] = rest
        elif doing_total:
          if line.startswith("Counterexamples of type FALSE"):
            self.cex_false = int(line.split()[-1])
          elif line.startswith("Counterexamples of type TRUE"):
            self.cex_true = int(line.split()[-1])
          elif line.startswith("Counterexamples of type TRANSITION"):
            self.cex_trans = int(line.split()[-1])
          elif line.startswith("number of candidates could not determine inductiveness"):
            self.inv_indef = int(line.split()[-1])
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
          elif line.startswith("long smtAllQueries ops"):
            self.total_long_smtAllQueries_ops = int(line.split()[-1])
          elif line.startswith("long smtAllQueries time"):
            self.total_long_smtAllQueries_ms = int(line.split()[-2])

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

  def get_global_stat_avg(self, name):
    return float(self.global_stats[name]["avg"][0])

  def get_global_stat_percentile(self, name, percentile):
    assert len(self.global_stats[name]["percentiles"]) == 101
    return int(self.global_stats[name]["percentiles"][percentile])

class Hatch(object):
  def __init__(self):
    pass

class Table(object):
  def __init__(self, column_names, rows, calc_fn, source_column=False, borderless=False):
    self.source_column = source_column
    self.column_names = []
    self.column_alignments = []
    self.column_double = []
    self.borderless = borderless
    for stuff in column_names:
      if stuff != '||':
        alignment, c = stuff
        self.column_names.append(c)
        self.column_double.append(False)
        self.column_alignments.append(alignment)
      else:
        self.column_double[-1] = True

    self.rows = []
    self.rows.append(self.column_names)
    self.row_double = []
    hatch_ctr = 0
    self.hatch_lines = []
    for r in rows:
      if r == '||':
        self.row_double[-1] = True
        continue
      self.row_double.append(False)

      new_r = []
      for column_double, c in zip(self.column_double, self.column_names):
        x = calc_fn(r, c)
        if isinstance(x, Hatch):
          hatch_ctr += 1
          if column_double:
            new_r.append("\\hatchdouble{" + str(hatch_ctr) + "}{1}")
          else:
            new_r.append("\\hatch{" + str(hatch_ctr) + "}{1}")
          self.hatch_lines.append("\\HatchedCell{start"+str(hatch_ctr)+"}{end"+str(hatch_ctr)+"}{pattern color=black!70,pattern=north east lines}")
        else:
          assert x != None
          s = str(x)
          new_r.append(s)
      self.rows.append(new_r)
  def dump(self):
    colspec = "|"
    if self.source_column:
      colspec = "|c|"
    for (al, d) in zip(self.column_alignments, self.column_double):
      if d:
        colspec += al+"||"
      else:
        colspec += al+"|"

    source_col = [
      "\\multirow{1}{*}{\\S\\ref{sec-sdl}}",
      "\\multirow{8}{*}{\\cite{I4}}",
      "\\multirow{13}{*}{\\cite{fol-sep}}",
      "\\multirow{7}{*}{\\cite{paxos-made-epr}}",
    ]
    source_col_idx = 0

    if self.borderless:
      colspec = colspec[1 : -1]

    s = "\\begin{tabular}{" + colspec + "}\n"
    if not self.borderless:
      s += "\\hline\n"
    column_widths = [max(len(self.rows[r][c]) for r in range(len(self.rows))) + 1 for c in range(len(self.column_names))]
    for i in range(len(self.rows)):
      if self.source_column:
        if i == 0:
          s += "Source &"
        else:
          s += "       &"

      for j in range(len(self.column_names)):
        s += (" " * (column_widths[j] - len(self.rows[i][j]))) + self.rows[i][j]
        if j == len(self.column_names) - 1:
          if i == 0 or self.row_double[i-1]:
            if self.borderless:
              s += " \\\\ \\hline"
            else:
              s += " \\\\ \\hline \\hline"
            if self.source_column:
              s += "\n" + source_col[source_col_idx]
              source_col_idx += 1
          else:
            if self.source_column and i != len(self.rows) - 1:
              s += " \\\\ \\cline{2-"+str(len(self.column_names)+1)+"}"
            else:
              if self.borderless and (i == len(self.rows) - 1):
                s += " \\\\"
              else:
                s += " \\\\ \\hline"
          s += "\n"
        else:
          s += " &"
    s += "\\end{tabular}\n"
    for h in self.hatch_lines:
      s += h + "\n"
    print(s, end="")

def read_I4_data(input_directory):
  def real_line_parse_secs(l):
    # e.g., real	0m0.285s
    k = l.split('\t')[1]
    t = k.split('m')
    minutes = int(t[0])
    assert t[1][-1] == "s"
    seconds = float(t[1][:-1])
    return minutes*60.0 + seconds

  i4_filename = os.path.join(input_directory, "I4-output.txt")

  if not os.path.exists(i4_filename):
    return None

  d = {}
  with open(i4_filename) as f:
    last_real_line = None
    for line in f:
      line = line.strip()
      if line.startswith("real\t"):
        last_real_line = line
      elif line.endswith(" done!"):
        secs = real_line_parse_secs(last_real_line)
        last_real_line = None

        name = line[:-6]

        d[name] = secs
  return d

def commaify(n):
  n = str(n)
  s = ""
  for i in range(0, len(n))[::-1]:
    if len(n) - i > 1 and (len(n) - i) % 3 == 1:
      s = "," + s
    s = n[i] + s
  return s

def commaify1(n):
  if n == 820878178:
    return '$820 \cdot 10^6$~'
  elif n == 3461137:
    return '$3 \cdot 10^6$'
  elif n == 98828918711712:
    return '$99 \cdot 10^{12}$'
  elif n == 232460599445:
    return '$232 \cdot 10^{9}$'
  else:
    return commaify(n)

def I4_get_res(d, r):
  if d == None:
    return None

  name = get_bench_name(r)
  if name in ("ticket", "learning-switch-quad", "sdl"):
    return None

  if get_bench_existential(r):
    return None

  I4_name = {
      "toy-consensus-forall": "toy_consensus_forall",
      "consensus-forall": "consensus_forall",
      "consensus-wo-decide": "consensus_wo_decide",
      "lock-server-async": "lockserv",
      "sharded-kv": "sharded_kv",
      "ticket": "ticket",

      'sdl': "simple-de-lock",
      'ring-election': "leader election",
      'learning-switch-ternary': "learning switch",
      'lock-server-sync': "lock server",
      'two-phase-commit': "two phase commit",
      'chain': "database chain replication",
      'chord': "chord ring",
      'distributed-lock': "distributed lock",
  }[name]

  return d[I4_name]
  

def read_folsep_json(input_directory):
  json_file = os.path.join(input_directory, "folsep.json")
  if os.path.exists(json_file):
    with open(os.path.join(input_directory, "folsep.json")) as f:
      j = f.read()
    return json.loads(j)
  else:
    return None

def folsep_json_get_res(j, r):
  if j == None:
    return None

  name = get_bench_name(r)
  fol_name = {
      "client-server-ae": "client_server_ae",
      "client-server-db-ae": "client_server_db_ae",
      "toy-consensus-epr": "toy_consensus_epr",
      "toy-consensus-forall": "toy_consensus_forall",
      "consensus-epr": "consensus_epr",
      "consensus-forall": "consensus_forall",
      "consensus-wo-decide": "consensus_wo_decide",
      "hybrid-reliable-broadcast": "hybrid_reliable_broadcast_cisa",
      "learning-switch-quad": "learning_switch",
      "lock-server-async": "lockserv",
      "sharded-kv": "sharded_kv",
      "sharded-kv-no-lost-keys": "sharded_kv_no_lost_keys",
      "ticket": "ticket",

      'sdl': "simple-de-lock",
      'ring-election': "ring_id",
      'learning-switch-ternary': "learning_switch_ternary",
      'lock-server-sync': "lock_server",
      'two-phase-commit': "2PC",
      'multi-paxos': "multi_paxos",
      'flexible-paxos': "flexible_paxos",
      'fast-paxos': "fast_paxos",
      'vertical-paxos': "vertical_paxos",
      'stoppable-paxos': "stoppable_paxos",
      'chain': "chain",
      'chord': "chord",
      'distributed-lock': "distributed_lock",
      'paxos': "paxos",
  }[name]

  for entry in j:
    if entry["name"] == fol_name:
      if entry["success"]:
        return entry["elapsed"]
      else:
        return None
  assert False, fol_name

def get_bench_name(name):
  return get_bench_info(name)[1]

def get_bench_existential(name):
  return get_bench_info(name)[2]

#def get_bench_num_handwritten_invs(name):
#  return get_bench_info(name)[3]
#
#def get_bench_num_handwritten_terms(name):
#  return get_bench_info(name)[4]

def get_bench_info(name):
  name = "_"+name

  if "pyv" in name:
    stuff = [
      ("__client_server_ae_pyv__", "client-server-ae", True),
      ("__client_server_db_ae_pyv__", "client-server-db-ae", True),
      ("__toy_consensus_epr_pyv__", "toy-consensus-epr", True),
      ("__toy_consensus_forall_pyv__", "toy-consensus-forall", False),
      ("__consensus_epr_pyv__", "consensus-epr", True),
      ("__consensus_forall_pyv__", "consensus-forall", False),
      ("__consensus_wo_decide_pyv__", "consensus-wo-decide", False),
      ("__firewall_pyv__", "firewall", True),
      ("__hybrid_reliable_broadcast_cisa_pyv__", "hybrid-reliable-broadcast", True),
      ("__learning-switch-quad_pyv__", "learning-switch-quad", False),
      ("__lock-server-async_pyv__", "lock-server-async", False),
      #("__ring_id_pyv__", "ring-election-mypyvy"),
      ("__ring_id_not_dead_pyv__", "ring-election-not-dead", True),
      ("__sharded_kv_pyv__", "sharded-kv", False),
      ("__sharded_kv_no_lost_keys_pyv__", "sharded-kv-no-lost-keys", True),
      ("__ticket_pyv__", "ticket", False),
    ]
  else:
    stuff = [
      ("__simple-de-lock__", 'sdl', False),
      ("__leader-election__", 'ring-election', False),
      ("__learning-switch-ternary__", 'learning-switch-ternary', False),
      ("__lock-server-sync__", 'lock-server-sync', False),
      ("__2PC__", 'two-phase-commit', False),
      ("__multi_paxos__", 'multi-paxos', True),
      ("__flexible_paxos__", 'flexible-paxos', True),
      ("__fast_paxos__", 'fast-paxos', True),
      ("__vertical_paxos__", 'vertical-paxos', True),
      ("__stoppable_paxos__", 'stoppable-paxos', True),
      ("__chain__", 'chain', False),
      ("__chord__", 'chord', False),
      ("__distributed_lock__", 'distributed-lock', False),
      ("__paxos__", 'paxos', True),
      ("__paxos_epr_missing1__", 'paxos-missing1', True),
    ]

  for a in stuff:
    if a[0] in name:
      return a
  print("need bench name for", name)
  assert False

def get_basic_stats_or_fail(input_directory, r):
  return BasicStats(input_directory, get_bench_name(r), r)

def get_basic_stats(input_directory, r):
  #assert paper_benchmarks.does_exist(r), r
  try:
    return BasicStats(input_directory, get_bench_name(r), r)
  except FileNotFoundError:
    return None

def median(input_directory, r, a, b, fail_if_absent=False):
  t = []
  for i in range(a, b+1):
    r1 = r.replace("#", str(i))
    #if fail_if_absent:
    #  s = get_basic_stats_or_fail(input_directory, r1)
    #else:
    s = get_basic_stats(input_directory, r1)
    if s != None:
      t.append(s)

  if len(t) == b - a + 1:
    assert len(t) % 2 == 1

    t.sort(key=lambda s : s.total_time_sec)

    return t[len(t) // 2]
  else:
    if len(t) == 0:
      if fail_if_absent:
        raise Exception("median failed to get " + r + ": found none")
      return None

    all_timeout = all(s.timed_out_6_hours for s in t)
    if all_timeout:
      return t[0]
    else:
      if fail_if_absent:
        raise Exception("median failed to get " + r +
            ": did not time-out and there were not enough runs")
      return None

def median_or_fail(input_directory, r, a, b):
  return median(input_directory, r, a, b, fail_if_absent=True)

MAIN_TABLE_ROWS = [
    "mm_nonacc__simple-de-lock__auto__seed#_t8",

    "||",

    "mm_nonacc__leader-election__auto_full__seed#_t8",
    "mm_nonacc__learning-switch-ternary__auto_e0_full__seed#_t8",
    "mm_nonacc__lock-server-sync__auto_full__seed#_t8",
    "mm_nonacc__2PC__auto_full__seed#_t8",
    "mm_nonacc__chain__auto__seed#_t8",
    "mm_nonacc__chord__auto__seed#_t8",
    "mm_nonacc__distributed_lock__auto9__seed#_t8",

    "||",

    "mm_nonacc__toy_consensus_forall_pyv__auto__seed#_t8",
    "mm_nonacc__consensus_forall_pyv__auto__seed#_t8",
    "mm_nonacc__consensus_wo_decide_pyv__auto__seed#_t8",
    "mm_nonacc__learning-switch-quad_pyv__auto__seed#_t8",
    "mm_nonacc__lock-server-async_pyv__auto9__seed#_t8",
    "mm_nonacc__sharded_kv_pyv__auto9__seed#_t8",
    "mm_nonacc__ticket_pyv__auto9__seed#_t8",

    "mm_nonacc__toy_consensus_epr_pyv__auto__seed#_t8",
    "mm_nonacc__consensus_epr_pyv__auto__seed#_t8",
    "mm_nonacc__client_server_ae_pyv__auto__seed#_t8",
    "mm_nonacc__client_server_db_ae_pyv__auto__seed#_t8",
    "mm_nonacc__sharded_kv_no_lost_keys_pyv__auto_e2__seed#_t8",
    "mm_nonacc__hybrid_reliable_broadcast_cisa_pyv__auto__seed#_t8",

    #"mm_nonacc__firewall_pyv__auto__seed#_t8", # ignoring because not EPR
    #"mm_nonacc__ring_id_pyv__auto__seed#_t8",
    #"mm_nonacc__ring_id_not_dead_pyv__auto__seed#_t8", # ignoring because not EPR

    "||",

    "mm_nonacc__paxos__auto__seed#_t8",
    "mm_nonacc__flexible_paxos__auto__seed#_t8",
    "mm_nonacc__multi_paxos__auto__seed#_t8",
    "mm_nonacc__multi_paxos__basic__seed1_t8",
    "mm_nonacc__fast_paxos__auto__seed#_t8",
    "mm_nonacc__stoppable_paxos__auto__seed#_t8",
    "mm_nonacc__vertical_paxos__auto__seed#_t8",
  ]

def get_inv_analysis_info(input_directory, benchname):
  filename = os.path.join(input_directory, benchname, "inv_analysis")
  if os.path.exists(filename):
    with open(filename) as f:
      j = f.read()
    return json.loads(j)
  else:
    return {}

def get_or_question_mark(inv_analysis_info, r, name, succ=None):
  if succ != None:
    if succ:
      name = "synthesized_" + name
    else:
      name = "handwritten_" + name
  if r in inv_analysis_info and name in inv_analysis_info[r]:
    return str(inv_analysis_info[r][name])
  else:
    return "?"

def make_comparison_table(input_directory, table, median_of=5):
  if "camera-ready-2020august-serenity" in input_directory:
    global use_old_names
    use_old_names = True

  rows = MAIN_TABLE_ROWS

  stats = { } # r : get_basic_stats(input_directory, r) for r in rows }
  inv_analysis_info = { }
  for r in rows:
    if r != '||':
      if use_old_names:
        r = r.replace("auto_full", "auto").replace("auto_e0_full","auto_e0")

      stats[r] = median(input_directory, r, 1, median_of)
      if stats[r]:
        fname = stats[r].filename
        inv_analysis_info[r] = get_inv_analysis_info(
            input_directory, fname)

  # l|r|c||r|r|r||r|r|r|r

  SWISS_INVS = '\\begin{tabular}{@{}c@{}}\\name \\\\ invs\\end{tabular}'
  SWISS_TERMS = '\\begin{tabular}{@{}c@{}}\\name \\\\ terms\\end{tabular}'

  #I4_COL = '\\begin{tabular}{@{}c@{}}I4 \\\\ \\cite{I4}\\end{tabular}'
  #FOL_COL = '\\begin{tabular}{@{}c@{}}FOL \\\\ \\cite{fol-sep}\\end{tabular}'

  I4_COL = 'I4~\\cite{I4}'
  FOL_COL = 'FOL~\\cite{fol-sep}'

  MAX_TERMS_STR = '$mt$'
  MAX_TERMS_M1_STR = '$mt_B$'

  SPLIT_COL = True

  cols1 = [
    ('l', 'Benchmark'),
    #('r', 'invs'),
    #('r', 'terms'),
    ('c', '$\\exists$?'),
    '||',
    ('r', I4_COL),
    ('r', FOL_COL),
    ('r', '\\name'),
    ('r', 'Partial'),
    '||',
    ('r', '$t_B$'),
    ('r', '$t_F$'),
    ('r', '$n_B$'),
    #('r', SWISS_INVS),
    #('r', SWISS_TERMS),
    #('r', 'Partial'),
    '||',
    ('r', MAX_TERMS_STR),
    ('r', MAX_TERMS_M1_STR),
  ]

  LIST_NAME = 'list'
  cols2 = [
    ('l', 'Benchmark'),
    ('l', 'Solved'),
    '||',
    ('r', 'total terms'),
    ('r', 'max vars'),
    ('r', LIST_NAME),
    #('r', 'Partial'),
  ]

  folsep_json = read_folsep_json(input_directory)
  i4_data = read_I4_data(input_directory)

  sec_suffix = " s."

  def format_secs(s):
    s = float(s)
    if s < 1.0:
      return "{:.1f}".format(s) + sec_suffix
    else:
      return str(int(s)) + sec_suffix

  def calc(r, c):
    if use_old_names:
      r = r.replace("auto_full", "auto").replace("auto_e0_full","auto_e0")

    did_succeed = (stats[r] != None and (not stats[r].timed_out_6_hours) and stats[r].success)

    if c == "Benchmark":
      bname = get_bench_name(r)
      if bname == "multi-paxos" and "basic" in r:
        return bname + "$^*$"
      return bname
    elif c == I4_COL:
      i4_time = I4_get_res(i4_data, r)

      if i4_time == None:
        return Hatch()
      else:
        return format_secs(i4_time)
    elif c == FOL_COL:
      folsep_time = folsep_json_get_res(folsep_json, r)
      if folsep_time == None:
        return Hatch()
      else:
        return format_secs(folsep_time)
    elif c == '$\\exists$?':
      return "$\\checkmark$" if get_bench_existential(r) else ""
    elif c == 'invs':
      return get_or_question_mark(inv_analysis_info, r, "invs", succ=did_succeed)
    elif c == 'total terms':
      return get_or_question_mark(inv_analysis_info, r, "terms", succ=did_succeed)
    elif c == 'max vars':
      return get_or_question_mark(inv_analysis_info, r, "max_vars", succ=did_succeed)
    elif c == 'Solved':
      return "$\\checkmark$" if did_succeed else ""
    elif c in (LIST_NAME, MAX_TERMS_STR, MAX_TERMS_M1_STR):
      names = (['synthesized'] if did_succeed else []) + ['handwritten']
      t = []
      for name1 in names:
        name = name1 + '_k_terms_by_inv'

        if r in inv_analysis_info and name in inv_analysis_info[r]:
          l = inv_analysis_info[r][name]
          a = max([0] + l[:-1])
          b = l[-1]
          d = max(l)
          t.append((a,b,d))
      if len(t) == 0:
        return '?'
      if len(t) == 1:
        a,b,d = t[0]
      if len(t) == 2:
        a,b,d = (t[0] if t[0][1] < t[1][1] else t[1])
      if c == LIST_NAME:
        if a == 0:
          return str(b)
        else:
          return "[" + str(a) + "...], " + str(b)
      elif c == MAX_TERMS_STR:
        return d
      elif c == MAX_TERMS_M1_STR:
        return ('' if a == 0 else a)
      else:
        assert False
    else:
      if stats[r] == None:
        return "TODO1"
      if stats[r].timed_out_6_hours:
        if (SPLIT_COL and c == 'Partial') or ((not SPLIT_COL) and c == '\\name'):
          invs_got = get_or_question_mark(inv_analysis_info, r,
              "invs_got")
          invs_total = get_or_question_mark(inv_analysis_info, r,
              "invs_total")

          return (
            '(' + str(invs_got) + ' / ' + str(invs_total) + ')'
          )
        elif c == '\\name':
          return Hatch()
        else:
          return ""

      if not stats[r].success:
        return "TODO2"

      if c == "$n_B$":
        return stats[r].num_breadth_iters
      elif c == "$t_F$":
        if stats[r].num_finisher_iters > 0:
          return format_secs(stats[r].finisher_time_sec)
        else:
          return "-"
      elif c == "$t_B$":
        return format_secs(stats[r].breadth_total_time_sec)
      elif c == "\\name":
        return format_secs(stats[r].total_time_sec)
      elif c == SWISS_INVS:
        return str(stats[r].num_inv)
        return get_or_question_mark(inv_analysis_info, r, "synthesized_invs")
      elif c == SWISS_TERMS:
        return get_or_question_mark(inv_analysis_info, r, "synthesized_terms")
      elif c == 'Partial':
        return ''
      else:
        assert False, c

  if table == 1:
    t = Table(cols1, rows, calc, source_column=True)
    t.dump()
  elif table == 2:
    t = Table(cols2, rows, calc)
    t.dump()
  else:
    assert False

def make_optimization_step_table(input_directory):
  logfiles = [
      "mm__paxos__basic_b__seed1_t1",
      "mm_whole__paxos_epr_missing1__basic__seed1_t1"
    ]

  thread_stats = { }
  counts = { }
  rows = [ ]
  for r in logfiles:
    s = median(input_directory, r, 1, 5)
    #assert s.success
    ts = ThreadStats(input_directory, s.filename)
    thread_stats[r] = ts

    full_name = os.path.join(input_directory, s.filename, "summary")

    for alg in ('breadth', 'finisher'):
      if alg == 'breadth':
        bs = ts.get_breadth_stats()
        if len(bs) != 0:
          rows.append((r, alg))
          counts[(r, alg)] = get_counts.get_counts(full_name, alg)
      else:
        fs = ts.get_finisher_stats()
        if len(fs) != 0:
          rows.append((r, alg))
          counts[(r, alg)] = get_counts.get_counts(full_name, alg)

  columns = [
    ('l', ''),
    ('r', 'Baseline'),
    ('r', 'Sym.\\ '),
    ('r', 'Cex filters'),
    ('r', 'FastImpl'),
    #'||',
    ('r', 'Inv.'),
  ]

  def calc(row, col):
    r, alg = row
    ts = thread_stats[r]

    if alg == 'breadth':
      bs = ts.get_breadth_stats()
      stats = bs[0][-1][0]
    else:
      fs = ts.get_finisher_stats()
      stats = fs[0]

    if col == '':
      #return bench.name + (' (B)' if alg == 'breadth' else ' (F)')
      #rname = get_bench_name(r)
      #if rname == 'paxos-missing1':
      #  rname = 'paxos'
      #return rname + (' (B)' if alg == 'breadth' else ' (F)')
      return '$\\B$' if alg == 'breadth' else '$\\F$'
    elif col == 'Baseline':
      return commaify1(counts[row].presymm)
    elif col == 'Sym.\\ ':
      return commaify1(counts[row].postsymm)
    elif col == 'Cex filters':
      return commaify(stats["Counterexamples of type FALSE"]
          + stats["Counterexamples of type TRANSITION"]
          + stats["Counterexamples of type TRUE"]
          + stats["number of non-redundant invariants found"]
          + stats["number of redundant invariants found"]
          + stats["number of finisher invariants found"]
          + stats["number of enumerated filtered redundant invariants"]
        )

    elif col == 'FastImpl':
      return commaify(stats["Counterexamples of type FALSE"]
          + stats["Counterexamples of type TRANSITION"]
          + stats["Counterexamples of type TRUE"]
          + stats["number of non-redundant invariants found"]
          + stats["number of redundant invariants found"]
          + stats["number of finisher invariants found"]
        )

    elif col == 'Inv.':
      return commaify(
            stats["number of non-redundant invariants found"]
          + stats["number of redundant invariants found"]
          + stats["number of finisher invariants found"]
        )

  print("\\begin{center}")
  t = Table(columns, rows, calc, borderless=True)
  t.dump()
  print("\\end{center}")

def get_files_with_prefix(input_directory, prefix):
  files = []
  for filename in os.listdir(input_directory):
    if filename.startswith(prefix):
      files.append(filename)
  return files

#def make_nonacc_cmp_graph(ax, input_directory):
#  a = get_files_with_prefix(input_directory, "paxos_breadth_seed_")
#  b = get_files_with_prefix(input_directory, "nonacc_paxos_breadth_seed_")
#  columns = (
#    [(a[i], i+1, i+1) for i in range(len(a))] +
#    [(b[i], len(a) + 2 + i, i+1) for i in range(len(b))]
#  )
#  ax.set_title("accumulation (paxos)")
#  make_segmented_graph(ax, input_directory, "", "", columns=columns)

def make_parallel_graph(ax, input_directory, name, include_threads=False, include_breakdown=False, graph_cex_count=False, graph_inv_count=False, collapse_size_split=False, collapsed_breakdown=False, graph_title=None, smaller=False, legend=False):
  suffix = ''
  if graph_cex_count:
    suffix = ' (cex)'
  if graph_inv_count:
    suffix = ' (invs)'
  
  if graph_title == None:
    ax.set_title("parallel " + name + suffix)
  else:
    ax.set_title(graph_title)

  ax.set_ylabel("seconds")
  ax.set_xlabel("threads")
  make_segmented_graph(ax, input_directory, name, "_t", include_threads=include_threads, include_breakdown=include_breakdown, 
      collapsed_breakdown=collapsed_breakdown,
      graph_cex_count=graph_cex_count, graph_inv_count=graph_inv_count, collapse_size_split=collapse_size_split, smaller=smaller,legend=legend)

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

segment1_color = 'white' #'#009e73'
segment2_color = 'white' #'#f0e442'
filter_color = 'black'
cex_color =  '#d55e00'
redundant_color = '#0072b2'
nonredundant_color = '#56b4e9'
inv_color = 'white' #'#0072b2'
other_color = 'white' #'#cc79a7'
other_hatch = '////'

def make_segmented_graph(ax, input_directory, name, suffix,
        include_threads=False,
        include_breakdown=False,
        columns=None,
        graph_cex_count=False,
        graph_inv_count=False,
        skip_b=False,
        skip_f=False,
        collapse_size_split=False,
        collapsed_breakdown=False,
        smaller=False, legend=False):

  if legend:
    p = [
      patches.Patch(edgecolor='black', facecolor=filter_color, label='Filtering'),
      patches.Patch(edgecolor='black', facecolor=cex_color, label='Counterex.'),
      patches.Patch(edgecolor='black', facecolor=inv_color, label='Inv.'),
      #patches.Patch(edgecolor='black', facecolor=nonredundant_color, label='Non-red.'),
      #patches.Patch(edgecolor='black', facecolor=other_color, label='Other', hatch=other_hatch)
    ]
    ax.legend(handles=p)

  if graph_cex_count or graph_inv_count:
    include_breakdown = True

  if columns is None:
    columns = []
    for filename in os.listdir(input_directory):
      if filename.startswith(name + suffix):
        idx = int(filename[len(name + suffix) : ])
        x_label = idx
        columns.append((filename, idx, x_label))
    if smaller:
      columns = [c for c in columns if c[1] in (1,2,4,8)]

  if smaller:
    labels = [1,2,4,8]
    ticks = [1,2,3,4]
  else:
    labels = [a[2] for a in columns]
    ticks = [a[1] for a in columns]

  ax.set_xticks(ticks)
  ax.set_xticklabels(labels)

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
                colors.append(segment1_color)
              else:
                colors.append(segment2_color)

            this_size_time += longest["total_time"]

          if collapse_size_split:
            times.append(this_size_time)
            if odd1:
              colors.append(segment1_color)
            else:
              colors.append(segment2_color)

      if not skip_f:
        fs = ts.get_finisher_stats()
        if len(fs) > 0:
          time_per_thread = sorted([s["total_time"] for s in fs])
          thread_times.append(time_per_thread)
          longest = get_longest_that_succeed(fs)
          breakdowns.append(Breakdown(longest))
          times.append(longest["total_time"])

          colors.append(segment1_color)

      if collapsed_breakdown:
        assert not include_threads
        times = [sum(times)]
        breakdowns = [sum_breakdowns(breakdowns)]

      if smaller:
        idx = {1: 1, 2: 2, 4: 3, 8: 4}[idx]

      bottom = 0
      for i in range(len(times)):
        if graph_cex_count or graph_inv_count:
          breakdown = breakdowns[i]
          c = (breakdown.total_cex if graph_cex_count else breakdown.total_invs)
          ax.bar(idx, c, bottom=bottom, color=colors[i], edgecolor='black')
          bottom += c
        else:
          t = times[i]

          if not collapsed_breakdown:
            if include_breakdown:
              ax.bar(idx - 0.2, t, bottom=bottom, width=0.4, color=colors[i], edgecolor='black')
            else:
              ax.bar(idx, t, bottom=bottom, color=colors[i], edgecolor='black')

          if include_breakdown or collapsed_breakdown:
            breakdown = breakdowns[i]
            a = bottom
            b = a + breakdown.filter_secs
            c = b + breakdown.cex_secs
            d = c + breakdown.nonredundant_secs
            e = d + breakdown.redundant_secs
            top = bottom + breakdown.stats["total_time"]
            e = top - breakdown.filter_secs
            d = e - breakdown.cex_secs
            c = d - breakdown.nonredundant_secs
            b = c - breakdown.redundant_secs
            a = bottom

            if collapsed_breakdown:
              center = idx
              width = 0.8
            else:
              center = idx + 0.2
              width = 0.4

            ax.bar(center, b-a, bottom=a, width=width, color=other_color, edgecolor='black', hatch=other_hatch)
            ax.bar(center, c-b, bottom=b, width=width, color=inv_color, edgecolor='black')
            ax.bar(center, d-c, bottom=c, width=width, color=nonredundant_color, edgecolor='black')
            ax.bar(center, e-d, bottom=d, width=width, color=cex_color, edgecolor='black')
            ax.bar(center, top-e, bottom=e, width=width, color=filter_color, edgecolor='black')

          if not collapsed_breakdown:
            if include_threads:
              for j in range(len(thread_times[i])):
                thread_time = thread_times[i][j]
                m = len(thread_times[i])
                if include_breakdown:
                  ax.bar(idx - 0.4 + 0.4 * (j / float(m)) + 0.2 / float(m),
                      thread_time, bottom=bottom, width = 0.4 / float(m),
                      color=colors[i])
                else:
                  ax.bar(idx - 0.4 + 0.8 * (j / float(m)) + 0.4 / float(m),
                      thread_time, bottom=bottom, width = 0.8 / float(m),
                      color=colors[i])

          bottom += t

def get_total_time(input_directory, filename):
  with open(os.path.join(input_directory, map_filename_maybe(filename), "summary")) as f:
    for line in f:
      if line.startswith("total time: "):
        t = float(line.split()[2])
        return t

def make_opt_comparison_graph(ax, input_directory, opt_name):
  ax.set_yscale('log')

  #opts = ['mm_', 'postbmc_mm_', 'prebmc_mm_', 'postbmc_prebmc_mm_']
  #if large_ones:
  #  probs = ['flexible_paxos', 'learning_switch', 'paxos']
  #else:
  #  probs = ['2pc', 'leader_election_breadth', 'leader_election_fin', 'lock_server']

  if opt_name == 'bmc':
    opts = ['mm_', 'prebmc_mm_'] #, 'postbmc_']
    names = ['Baseline', 'With BMC']
  elif opt_name == 'mm':
    opts = ['', 'mm_'] #, 'postbmc_']
    names = ['Baseline', 'With minimal-models']
  elif opt_name == 'both':
    if use_old_names:
      opts = ['', 'mm_', 'prebmc_mm_']
    else:
      opts = ['nonacc_', 'mm_nonacc_', 'prebmc_mm_nonacc_']
    names = ['Baseline', 'Min-models', 'Min-models + BMC']
  else:
    assert False

  #colors = ['black', '#ff8080', '#8080ff', '#80ff80']
  colors = ['black', '#E69F00', '#56b4e9']

  prob_data = [
      ('leader-election__basic_b__seed1_t8'),
      ('2PC__basic__seed1_t8'),
      ('learning-switch-ternary__basic__seed1_t8'),
      ('paxos__basic__seed1_t8'),
      ('flexible_paxos__basic__seed1_t8'),
      ('multi_paxos__basic__seed1_t8'),
  ]

  failures = [
    'prebmc__paxos__basic__seed1_t8',
    'prebmc__multi_paxos__basic__seed1_t8',
  ]

  ax.set_xticks([i for i in range(1, len(prob_data) + 1)])
  ax.set_xticklabels([get_bench_name("_"+prob) for prob in prob_data])

  for tick in ax.get_xticklabels():
      tick.set_rotation(30)
      tick.set_ha('right')

  patterns = [ None, "\\" , "/" , "+" ]

  idx = 0
  for prob in prob_data:
    idx += 1
    opt_idx = -1
    for (opt, color, pattern) in zip(opts, colors, patterns):
      opt_idx += 1

      name = opt + "_" + prob
      if name not in failures:
        t = get_total_time(input_directory, name)
        ax.bar(idx - 0.4 + 0.4/len(opts) + 0.8/len(opts) * opt_idx, t,
            bottom=0, width=0.8/len(opts), color=color, edgecolor='black') #hatch=pattern)

  p = []
  for (opt, opt_name_label, color, pattern) in zip(opts, names, colors, patterns):
    p.append(patches.Patch(color=color, label=opt_name_label))
  ax.legend(handles=p)

PAXOS_FINISHER_THREAD_BENCH = "mm_whole__paxos_epr_missing1__basic__seed1" # _t1 _t2 ... _t8
PAXOS_NONACC_THREAD_BENCH = "mm_nonacc__paxos__basic_b__seed1"
  
def make_parallel_graphs(input_directory, save=False):
  if "camera-ready-2020august-serenity" in input_directory:
    global use_old_names
    use_old_names = True

  output_directory = "graphs"
  Path(output_directory).mkdir(parents=True, exist_ok=True)

  fig, ax = plt.subplots(nrows=2, ncols=3, figsize=[7, 6])

  plt.gcf().subplots_adjust(bottom=0.20)

  ax.flat[1].set_ylim(bottom=0, top=1500)
  ax.flat[2].set_ylim(bottom=0, top=1500)
  ax.flat[4].set_ylim(bottom=0, top=1500)
  ax.flat[5].set_ylim(bottom=0, top=1500)

  #finisher = "mm__paxos_epr_missing1__basic__seed1" # _t1 _t2 ... _t8
  finisher = PAXOS_FINISHER_THREAD_BENCH
  breadth_acc = "mm__paxos__basic_b__seed1"
  breadth_nonacc = PAXOS_NONACC_THREAD_BENCH

  make_parallel_graph(ax.flat[0], input_directory, finisher, graph_title="Finisher", smaller=True)
  make_parallel_graph(ax.flat[1], input_directory, breadth_nonacc, collapse_size_split=True, graph_title="Breadth", smaller=True)
  make_parallel_graph(ax.flat[2], input_directory, breadth_acc, graph_title="BreadthAccumulative", smaller=True)

  make_parallel_graph(ax.flat[3], input_directory, finisher, collapsed_breakdown=True, graph_title="Finisher", smaller=True)
  make_parallel_graph(ax.flat[4], input_directory, breadth_nonacc, collapsed_breakdown=True, graph_title="Breadth", smaller=True)
  make_parallel_graph(ax.flat[5], input_directory, breadth_acc, collapsed_breakdown=True, graph_title="BreadthAccumulative", smaller=True, legend=True)

  #https://stackoverflow.com/questions/26084231/draw-a-separator-or-lines-between-subplots
  # Get the bounding boxes of the axes including text decorations
  axes = ax
  r = fig.canvas.get_renderer()
  get_bbox = lambda ax: ax.get_tightbbox(r).transformed(fig.transFigure.inverted())
  bboxes = np.array(list(map(get_bbox, axes.flat)), mtrans.Bbox).reshape(axes.shape)

  #Get the minimum and maximum extent, get the coordinate half-way between those
  ymax = np.array(list(map(lambda b: b.y1, bboxes.flat))).reshape(axes.shape).max(axis=1)
  ymin = np.array(list(map(lambda b: b.y0, bboxes.flat))).reshape(axes.shape).min(axis=1)
  ys = np.c_[ymax[1:], ymin[:-1]].mean(axis=1)

  # Draw a horizontal lines at those coordinates
  for y in ys:
      line = plt.Line2D([0,1],[y-0.025,y-0.025], transform=fig.transFigure, color="black")
      fig.add_artist(line)

  plt.tight_layout()

  if save:
    fname = os.path.join(output_directory, 'paxos-parallel.png')
    print(fname)
    plt.savefig(fname)
  else:
    plt.show()

def make_opt_graphs_main(input_directory, save=False, mm=False, both=False):
  if "camera-ready-2020august-serenity" in input_directory:
    global use_old_names
    use_old_names = True

  output_directory = "graphs"
  Path(output_directory).mkdir(parents=True, exist_ok=True)

  fig, ax = plt.subplots(nrows=1, ncols=1, figsize=[6.5, 4])
  plt.gcf().subplots_adjust(bottom=0.45)
  ax.set_ylabel("seconds")

  if both:
    make_opt_comparison_graph(ax, input_directory, 'both')
  elif mm:
    make_opt_comparison_graph(ax, input_directory, 'mm')
  else:
    make_opt_comparison_graph(ax, input_directory, 'bmc')

  if save:
    if both:
      fname = os.path.join(output_directory, 'opt-comparison-both.png')
    elif mm:
      fname = os.path.join(output_directory, 'opt-comparison-mm.png')
    else:
      fname = os.path.join(output_directory, 'opt-comparison-bmc.png')

    print(fname)
    plt.savefig(fname)
  else:
    plt.show()

#def make_seed_graphs_main(input_directory, save=False):
#  output_directory = "graphs"
#  Path(output_directory).mkdir(parents=True, exist_ok=True)
#
#  fig, ax = plt.subplots(nrows=1, ncols=4, figsize=[12, 3])
#  plt.gcf().subplots_adjust(bottom=0.20)
#
#  make_seed_graph(ax.flat[0], input_directory, "wc_learning-switch-ternary", title="Learning switch (BreadthAccumulative)")
#  make_seed_graph(ax.flat[1], input_directory, "wc_bt_paxos", title="Paxos (BreadthAccumulative)", skip_f=True)
#  make_seed_graph(ax.flat[2], input_directory, "wc_bt_paxos", title="Paxos (Finisher)", skip_b=True)
#  make_seed_graph(ax.flat[3], input_directory, "wholespace_finisher_bt_paxos", title="Paxos (Finisher, entire space)")

#  fig.tight_layout()

#  if save:
#    plt.savefig(os.path.join(output_directory, 'seed.png'))
#  else:
#    plt.show()

def misc_stats(input_directory, median_of=5):
  if "camera-ready-2020august-serenity" in input_directory:
    global use_old_names
    use_old_names = True

  def p(key, value, comment=""):
    print("\\newcommand{\\" + key + "}{" + str(value) + "}" +
        ("" if comment == "" else " % " + str(comment)))

  paxos_1_threads = get_basic_stats_or_fail(input_directory, PAXOS_FINISHER_THREAD_BENCH + "_t1")
  paxos_2_threads = get_basic_stats_or_fail(input_directory, PAXOS_FINISHER_THREAD_BENCH + "_t2")
  paxos_8_threads = get_basic_stats_or_fail(input_directory, PAXOS_FINISHER_THREAD_BENCH + "_t8")

  paxos_b_1_threads = get_basic_stats_or_fail(input_directory, PAXOS_NONACC_THREAD_BENCH + "_t1")
  paxos_b_8_threads = get_basic_stats_or_fail(input_directory, PAXOS_NONACC_THREAD_BENCH + "_t8")

  def speedup(s, t):
    x = t.total_time_sec / s.total_time_sec
    return "{:.1f}".format(x)

  p("paxosTwoThreadSpeedup", speedup(paxos_2_threads, paxos_1_threads))
  p("paxosEightThreadSpeedup", speedup(paxos_8_threads, paxos_1_threads))
  p("paxosBreadthNonaccEightThreadsSpeedup", speedup(paxos_b_8_threads, paxos_b_1_threads))

  p("learningSwitchTernaryAutoEZeroNumTemplates", 45)
  p("learningSwitchTernaryAutoEZeroTotalSize", "\\ensuremath{\\sim 10^8}", 102141912)
  p("learningSwitchTernaryAutoNumTemplates", 69)
  p("learningSwitchTernaryAutoTotalSize", "\\ensuremath{\\sim 10^8}",      142327951)
  p("learningSwitchQuadAutoNumTemplates", 21)
  p("learningSwitchQuadAutoTotalSize", "\\ensuremath{8\\times 10^6}", 8174934)
  p("paxosBreadthNumTemplates", 684)
  p("paxosBreadthTotalSize", "\\ensuremath{3 \\times 10^5}", 366402)
  p("paxosFinisherTheOneSize", "\\ensuremath{1.6 \\times 10^{10}}", 16862630188)
  p("paxosBreadthNth", "\\ensuremath{569^{\\text{th}}}")

  if use_old_names:
    p("flexiblePaxosMMSpeedup", speedup(
        get_basic_stats_or_fail(input_directory, 'mm__flexible_paxos__basic__seed1_t8'),
        get_basic_stats_or_fail(input_directory, '_flexible_paxos__basic__seed1_t8')))
  else:
    p("flexiblePaxosMMSpeedup", speedup(
        get_basic_stats_or_fail(input_directory, 'mm_nonacc__flexible_paxos__basic__seed1_t8'),
        get_basic_stats_or_fail(input_directory, 'nonacc__flexible_paxos__basic__seed1_t8')))

  def read_counts():
    d = []
    with open("scripts/paxos-spaces-sorted.txt") as f:
      for l in f:
        s = l.split()
        k = int(s[2])
        count = int(s[6])
        vs = [int(s[8]), int(s[12]), int(s[16]), int(s[20])]
        d.append((k, count, vs))
    return d

  counts = read_counts()

  def num_nonzero(vs):
    t = 0
    for i in vs:
      if i != 0:
        t += 1
    return t

  def sum_counts(counts, max_k, max_vs):
    total = 0
    for (k, count, vs) in counts:
      #if (k <= max_k and 
      #    vs[0] <= max_vs[0] and
      #    vs[1] <= max_vs[1] and
      #    vs[2] <= max_vs[2] and
      #    vs[3] <= max_vs[3]):
      if count <= 16862630188:
        total += count * (1 + num_nonzero(vs))
    return total

  #p("paxosUpToTotal", sum_counts(counts, 6, [2,2,1,1]))
  #934,257,540,926
  p("paxosUpToTotal", "\ensuremath{\sim 10^{12}}",                934257540926)
  p("paxosUpToTotalSmall", "\ensuremath{\sim 2 \\times 10^{11}}", 232460599445)
  p("paxosUserGuidanceSavingPercent", str(int((934257540926 - 232460599445) * 100 / 934257540926)))


  m = median_or_fail(input_directory, "mm_nonacc__paxos__auto__seed#_t8", 1, median_of)

  p("paxosUserGuidanceSavingTime", speedup(
      get_basic_stats_or_fail(input_directory, "mm_nonacc__paxos__basic__seed1_t8"),
      m))

  p("paxosFinisherOneThreadSmtAverage", 
      "{:.1f}".format(paxos_1_threads.get_global_stat_avg("total")))
  p("paxosFinisherOneThreadSmtMedian", 
      paxos_1_threads.get_global_stat_percentile("total", 50))
  p("paxosFinisherOneThreadSmtNinetyFifthPercentile", 
      paxos_1_threads.get_global_stat_percentile("total", 95))
  p("paxosFinisherOneThreadSmtNinetyNinthPercentile", 
      paxos_1_threads.get_global_stat_percentile("total", 95))

  p("paxosFinisherOneThreadFilterAvg", 
      int(paxos_1_threads.total_time_filtering_ms * (10**6) / 232460599445))

  p("paxosFinisherOneThreadCandidateAvg", 
      int(paxos_1_threads.total_time_filtering_ms * (10**6) / 232460599445))

  p("paxosNumCexes", 
      paxos_1_threads.cex_false
      + paxos_1_threads.cex_true
      + paxos_1_threads.cex_trans)

  p("paxosNumInductivityChecks", 
      paxos_1_threads.cex_false
      + paxos_1_threads.cex_true
      + paxos_1_threads.cex_trans
      + paxos_1_threads.inv_indef
      + ThreadStats(input_directory, paxos_1_threads.filename).get_finisher_stats()[0]["number of finisher invariants found"])

  total_solved = 0
  total_solved_breadth_only = 0
  total_solved_finisher_only = 0

  for r in MAIN_TABLE_ROWS:
    if r == '||':
      continue

    if use_old_names:
      r = r.replace("auto_full", "auto").replace("auto_e0_full","auto_e0")

    stats = median_or_fail(input_directory, r, 1, median_of)
    if not stats.timed_out_6_hours and stats.success:
      total_solved += 1
      if stats.num_finisher_iters == 0:
        total_solved_breadth_only += 1
      s = r.replace('#', '1').split('__')
      s[0] += "_fonly"
      fonly_name = '__'.join(s)

      if use_old_names:
        maps = {
          "mm_nonacc__leader-election__auto__seed#_t8":
              "mm_nonacc_fonly__leader-election__auto_full__seed1_t8",
          "mm_nonacc__learning-switch-ternary__auto_e0__seed#_t8":
              "mm_nonacc_fonly__learning-switch-ternary__auto_e0_full__seed1_t8",
          "mm_nonacc__lock-server-sync__auto__seed#_t8":
              "mm_nonacc_fonly__lock-server-sync__auto_full__seed1_t8",
          "mm_nonacc__2PC__auto__seed#_t8":
              "mm_nonacc_fonly__2PC__auto_full__seed1_t8",
        }
        if r in maps:
          fonly_name = maps[r]

      fonly_stats = get_basic_stats(input_directory, fonly_name)

      if r == 'mm_nonacc__sharded_kv_no_lost_keys_pyv__auto_e2__seed#_t8':
        total_solved_finisher_only += 1
        continue

      if fonly_stats == None:
        pass #print(r)
      else:
        if not fonly_stats.timed_out_6_hours and fonly_stats.success:
          total_solved_finisher_only += 1

  p("totalSolved", total_solved)
  p("totalSolvedBreadthOnly", total_solved_breadth_only)
  p("totalSolvedFinisherOnly", total_solved_finisher_only)

  """ 
  def percent_of_time_hard_smt(r):
    return 100.0 * float(r.total_long_smtAllQueries_ms) / (float(r.total_cpu_time_sec) * 1000)

  for r in MAIN_TABLE_ROWS:
    if r == '||': continue
    m = median_or_fail(input_directory, r, 1, median_of)
    if hasattr(m, 'total_cpu_time_sec'):
      print(m.filename)
      print(percent_of_time_hard_smt(m))
  """

def templates_table(input_directory):
  rows = [
    "mm_nonacc_whole__paxos_epr_missing1__basic__seed1_t8",
    "mm_nonacc_whole__paxos_epr_missing1__basic2__seed1_t8",
    "||",
    "mm_nonacc__paxos_epr_missing1__wrong1__seed1_t8",
    "mm_nonacc__paxos_epr_missing1__wrong2__seed1_t8",
    "mm_nonacc__paxos_epr_missing1__wrong3__seed1_t8",
    "mm_nonacc__paxos_epr_missing1__wrong5__seed1_t8",
    "mm_nonacc__paxos_epr_missing1__wrong4__seed1_t8",
  ]

  unwhole_map = {
    "mm_nonacc_whole__paxos_epr_missing1__basic__seed1_t8":
      "mm_nonacc__paxos_epr_missing1__basic__seed1_t8",
    "mm_nonacc_whole__paxos_epr_missing1__basic2__seed1_t8":
      "mm_nonacce__paxos_epr_missing1__basic2__seed1_t8",
  }

  cols = [
    ('l', '\\#'),
    ('l', 'Template'),
    ('c', 'Inv?'),
    ('r', 'Size'),
    ('r', 'Time'),
  ]

  suite = templates.read_suite("benchmarks/paxos_epr_missing1.ivy")
  def q_string(q):
    s = q.split()
    sorts = s[1:]
    assert s[0] in ('forall', 'exists')
    res = "\\" + s[0] + " "

    seen = set()
    a = 0
    while a < len(sorts):
      assert sorts[a] not in seen
      seen.add(sorts[a])

      b = a+1
      while b < len(sorts) and sorts[b] == sorts[a]:
        b += 1

      n_occur = b-a
      l = sorts[a][0]
      if a > 0:
        res += ", "
      res += ",".join(l+"_{"+str(i)+"}" for i in range(1,n_occur+1))
      res += ":\\sort{" + sorts[a] + "}"

      a = b

    return res+".~"

  def templ_string(r):
    config_name = r.split('__')[2]
    bench = suite.get(config_name)
    t = bench.finisher_space.spaces[0].templ
    qs = t.split('.')
    return "$" + " ".join(q_string(q) for q in qs) + "*" + "$"

  def read_counts():
    d = []
    with open("scripts/paxos-spaces-sorted.txt") as f:
      for l in f:
        s = l.split()
        k = int(s[2])
        count = int(s[6])
        vs = [int(s[8]), int(s[12]), int(s[16]), int(s[20])]
        d.append((k, count, vs))
    return d

  counts = read_counts()

  def sum_counts(counts, max_k, max_vs):
    total = 0
    for (k, count, vs) in counts:
      if (k <= max_k and 
          vs[0] <= max_vs[0] and
          vs[1] <= max_vs[1] and
          vs[2] <= max_vs[2] and
          vs[3] <= max_vs[3]):
        total += count
    return total

  sizes = {
    #"basic": 16862630188
    #"basic2": 16862630188
    #"wrong1": 16862630188
    "basic": sum_counts(counts, 6, [2,2,1,1]),
    "basic2": sum_counts(counts, 6, [2,2,1,1]),
    "wrong1": sum_counts(counts, 6, [2,2,1,1]),
    "wrong2": sum_counts(counts, 6, [1,3,0,1]),
    "wrong3": sum_counts(counts, 6, [1,1,0,4]),
    "wrong4": sum_counts(counts, 6, [2,2,0,2]),
    "wrong5": sum_counts(counts, 6, [2,1,2,1]),
  }

  def calc(r, c):
    if c == 'Template':
      return templ_string(r)
    elif c == 'Inv?':
      if 'wrong' in r:
        return ''
      else:
        return '$\\checkmark$'
    elif c == 'Size':
      return commaify(sizes[r.split('__')[2]])
    elif c == '\\#':
      only_rows = [r for r in rows if r != '||']
      return only_rows.index(r) + 1
    elif c == 'Time':
      stats = get_basic_stats(input_directory, r)
      if stats.timed_out_6_hours:
        #return '$>21600$'
        assert r == "mm_nonacc__paxos_epr_missing1__wrong4__seed1_t8"
        return "47231"
      else:
        return int(stats.total_time_sec)
    #elif c == 'Solve time':
    #  if r in unwhole_map:
    #    r2 = unwhole_map[r]
    #    stats = get_basic_stats(input_directory, r2)
    #    return int(stats.total_time_sec)
    #  else:
    #    return ''

  t = Table(cols, rows, calc)
  t.dump()

def stuff():
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

  make_opt_comparison_graph(ax.flat[10], input_directory)
  make_opt_comparison_graph(ax.flat[11], input_directory)

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

def main():
  input_directory = sys.argv[1]

  if "camera-ready-2020august-serenity" in input_directory:
    global use_old_names
    use_old_names = True

  #stuff()
  #make_parallel_graphs(input_directory)
  #make_seed_graphs_main(input_directory)
  #make_smt_stats_table(input_directory)
  #make_opt_graphs_main(input_directory, both=True)
  #make_optimization_step_table(input_directory)
  #make_comparison_table(input_directory, 1, median_of=1)
  misc_stats(input_directory)
  #templates_table(input_directory)

if __name__ == '__main__':
  main()
