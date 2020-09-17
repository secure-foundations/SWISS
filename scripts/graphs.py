import matplotlib.pyplot as plt
import matplotlib.patches as patches
import numpy as np
from pathlib import Path
import sys
import os
import json

import get_counts
import templates

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
    if log.startswith("./logs/log."):
      name = '.'.join(log.split('.')[5:])
    elif log.startswith("/pylon5/ms5pijp/tjhance/log."):
      name = '.'.join(log.split('.')[4:])
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

class BasicStats(object):

  def __init__(self, input_directory, name, filename, I4=None):
    self.I4_time = I4
    self.name = name
    self.filename = filename
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
      self.timed_out_6_hours = False
      self.global_stats = {}
      for line in f:
        if line.strip() == "total":
          doing_total = True
        if line.startswith("total time: "):
          self.total_time_sec = float(line.split()[2])
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
  def __init__(self, column_names, rows, calc_fn):
    self.column_names = []
    self.column_alignments = []
    self.column_double = []
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
    for (al, d) in zip(self.column_alignments, self.column_double):
      if d:
        colspec += al+"||"
      else:
        colspec += al+"|"

    s = "\\begin{tabular}{" + colspec + "}\n"
    s += "\\hline\n"
    column_widths = [max(len(self.rows[r][c]) for r in range(len(self.rows))) + 1 for c in range(len(self.column_names))]
    for i in range(len(self.rows)):
      for j in range(len(self.column_names)):
        s += (" " * (column_widths[j] - len(self.rows[i][j]))) + self.rows[i][j]
        if j == len(self.column_names) - 1:
          s += " \\\\ \\hline"
          if i == 0 or self.row_double[i-1]:
            s += " \\hline"
          s += "\n"
        else:
          s += " &"
    s += "\\end{tabular}\n"
    for h in self.hatch_lines:
      s += h + "\n"
    print(s)

def read_I4_data(input_directory):
  def real_line_parse_secs(l):
    # e.g., real	0m0.285s
    k = l.split('\t')[1]
    t = k.split('m')
    minutes = int(t[0])
    assert t[1][-1] == "s"
    seconds = float(t[1][:-1])
    return minutes*60.0 + seconds

  d = {}
  with open(os.path.join(input_directory, "I4-output.txt")) as f:
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

def I4_get_res(d, r):
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
  with open(os.path.join(input_directory, "folsep.json")) as f:
    j = f.read()
  return json.loads(j)

def folsep_json_get_res(j, r):
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

def get_bench_num_handwritten_invs(name):
  return get_bench_info(name)[3]

def get_bench_num_handwritten_terms(name):
  return get_bench_info(name)[4]

def get_bench_info(name):
  name = "_"+name

  if "pyv" in name:
    stuff = [
      ("__client_server_ae_pyv__", "client-server-ae", True, 1, 3),
      ("__client_server_db_ae_pyv__", "client-server-db-ae", True, 4, 12),
      ("__toy_consensus_epr_pyv__", "toy-consensus-epr", True, 3, 8),
      ("__toy_consensus_forall_pyv__", "toy-consensus-forall", False, 3, 8),
      ("__consensus_epr_pyv__", "consensus-epr", True, 6, 15),
      ("__consensus_forall_pyv__", "consensus-forall", False, 6, 15),
      ("__consensus_wo_decide_pyv__", "consensus-wo-decide", False, 4, 10),
      ("__firewall_pyv__", "firewall", True, 1, 3),
      ("__hybrid_reliable_broadcast_cisa_pyv__", "hybrid-reliable-broadcast", True, 7, 30),
      ("__learning_switch_pyv__", "learning-switch-quad", False, 2, 7),
      ("__lockserv_pyv__", "lock-server-async", False, 8, 18),
      #("__ring_id_pyv__", "ring-election-mypyvy"),
      ("__ring_id_not_dead_pyv__", "ring-election-not-dead", True, 4, 11),
      ("__sharded_kv_pyv__", "sharded-kv", False, 4, 11),
      ("__sharded_kv_no_lost_keys_pyv__", "sharded-kv-no-lost-keys", True, 1, 2),
      ("__ticket_pyv__", "ticket", False, 13, 35),
    ]
  else:
    stuff = [
      ("__simple-de-lock__", 'sdl', False, 3, 8),
      ("__leader-election__", 'ring-election', False, 3, 9),
      ("__learning-switch__", 'learning-switch-ternary', False, 4, 10),
      ("__lock_server__", 'lock-server-sync', False, 1, 2),
      ("__2PC__", 'two-phase-commit', False, 8, 18),
      ("__multi_paxos__", 'multi-paxos', True, 11, 34),
      ("__flexible_paxos__", 'flexible-paxos', True, 10, 31),
      ("__fast_paxos__", 'fast-paxos', True, 16, 60),
      ("__vertical_paxos__", 'vertical-paxos', True, 19, 61),
      ("__stoppable_paxos__", 'stoppable-paxos', True, 14, 50),
      ("__chain__", 'chain', False, 7, 24),
      ("__chord__", 'chord', False, 11, 31),
      ("__distributed_lock__", 'distributed-lock', False, 8, 28),
      ("__paxos__", 'paxos', True, 10, 34),
      ("__paxos_epr_missing1__", 'paxos-missing1', True, 10, 34),
    ]

  for a in stuff:
    if a[0] in name:
      return a
  print("need bench name for", name)
  assert False

def get_basic_stats(input_directory, r):
  try:
    return BasicStats(input_directory, get_bench_name(r), r)
  except FileNotFoundError:
    return None

def median(input_directory, r, a, b):
  t = []
  for i in range(a, b+1):
    r1 = r.replace("#", str(i))
    s = get_basic_stats(input_directory, r1)
    if s != None:
      t.append(s)

  if len(t) == b - a + 1:
    assert len(t) % 2 == 1

    t.sort(key=lambda s : s.total_time_sec)

    return t[len(t) // 2]
  else:
    if len(t) == 0:
      return None

    all_timeout = all(s.timed_out_6_hours for s in t)
    if all_timeout:
      return t[0]
    else:
      return None

MAIN_TABLE_ROWS = [
    "mm_nonacc__simple-de-lock__auto__seed#_t8",
    "mm_nonacc__leader-election__auto__seed#_t8",
    "mm_nonacc__learning-switch__auto_e0__seed#_t8",
    "mm_nonacc__lock_server__auto__seed#_t8",
    "mm_nonacc__2PC__auto__seed#_t8",
    "mm_nonacc__chain__auto__seed#_t8",
    "mm_nonacc__chord__auto__seed#_t8",
    "mm_nonacc__distributed_lock__auto9__seed#_t8",

    "||",

    "mm_nonacc__toy_consensus_forall_pyv__auto__seed#_t8",
    "mm_nonacc__consensus_forall_pyv__auto__seed#_t8",
    "mm_nonacc__consensus_wo_decide_pyv__auto__seed#_t8",
    "mm_nonacc__learning_switch_pyv__auto__seed#_t8",
    "mm_nonacc__lockserv_pyv__auto9__seed#_t8",
    "mm_nonacc__sharded_kv_pyv__auto9__seed#_t8",
    "mm_nonacc__ticket_pyv__auto__seed#_t8",

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

def make_comparison_table(input_directory):
  rows = MAIN_TABLE_ROWS

  terms_entries = """mm_nonacc__simple-de-lock__auto__seed3_t8 7
    mm_nonacc__leader-election__auto__seed4_t8 13
    mm_nonacc__learning-switch__auto_e0__seed3_t8 21
    mm_nonacc__lock_server__auto__seed2_t8 2
    mm_nonacc__2PC__auto__seed3_t8 24
    mm_nonacc__chain__auto__seed1_t8 TIMEOUT
    mm_nonacc__chord__auto__seed1_t8 TIMEOUT
    mm_nonacc__distributed_lock__auto9__seed1_t8 TIMEOUT
    mm_nonacc__toy_consensus_forall_pyv__auto__seed3_t8 14
    mm_nonacc__consensus_forall_pyv__auto__seed5_t8 24
    mm_nonacc__consensus_wo_decide_pyv__auto__seed5_t8 22
    mm_nonacc__learning_switch_pyv__auto__seed1_t8 79
    mm_nonacc__lockserv_pyv__auto9__seed4_t8 8
    mm_nonacc__sharded_kv_pyv__auto9__seed3_t8 10
    mm_nonacc__ticket_pyv__auto__seed1_t8 TIMEOUT
    mm_nonacc__toy_consensus_epr_pyv__auto__seed5_t8 11
    mm_nonacc__consensus_epr_pyv__auto__seed3_t8 15
    mm_nonacc__client_server_ae_pyv__auto__seed2_t8 9
    mm_nonacc__client_server_db_ae_pyv__auto__seed4_t8 34
    mm_nonacc__sharded_kv_no_lost_keys_pyv__auto9__seed1_t8 TIMEOUT
    mm_nonacc__hybrid_reliable_broadcast_cisa_pyv__auto__seed1_t8 TIMEOUT
    mm_nonacc__paxos__auto__seed5_t8 42
    mm_nonacc__multi_paxos__auto__seed1_t8 TIMEOUT
    mm_nonacc__multi_paxos__basic__seed1_t8 56
    mm_nonacc__flexible_paxos__auto__seed1_t8 42
    mm_nonacc__fast_paxos__auto__seed1_t8 TIMEOUT
    mm_nonacc__stoppable_paxos__auto__seed1_t8 TIMEOUT
    mm_nonacc__sharded_kv_no_lost_keys_pyv__auto_e2__seed4_t8 6
    mm_nonacc__vertical_paxos__auto__seed1_t8 TIMEOUT""".split('\n')
  terms = {}
  for te in terms_entries:
    x = te.split()
    terms[x[0]] = x[1]

  stats = { } # r : get_basic_stats(input_directory, r) for r in rows }
  for r in rows:
    if r != '||':
      stats[r] = median(input_directory, r, 1, 5)

  #I4_times = {
  #    "mm__simple-de-lock__auto__seed#_t8": -1,
  #    "mm__leader-election__auto__seed#_t8" : 1.686,
  #    "mm__learning-switch__auto_e0__seed#_t8" : 9.392,
  #    "mm__lock_server__auto__seed#_t8" : 1.598,
  #    "mm__2PC__auto__seed#_t8" : 1.994,
  #    "mm__paxos__auto__seed#_t8" : None,
  #    "mm__multi_paxos__auto__seed#_t8" : None,
  #    "mm__flexible_paxos__auto__seed#_t8" : None,
  #    "chord" : 29.193,
  #    "chain" : 11.679,
  #}

  # l|r|c||r|r|r||r|r|r|r

  SWISS_INVS = '\\begin{tabular}{@{}c@{}}\\name \\\\ invs\\end{tabular}'
  SWISS_TERMS = '\\begin{tabular}{@{}c@{}}\\name \\\\ terms\\end{tabular}'

  cols = [
    ('l', 'Benchmark'),
    ('r', 'invs'),
    ('r', 'terms'),
    ('c', '$\\exists$?'),
    '||',
    ('r', 'I4~\\cite{I4}'),
    ('r', 'FOL~\\cite{fol-sep}'),
    ('r', '\\name'),
    '||',
    ('r', '$t_B$'),
    ('r', '$t_F$'),
    ('r', '$n_B$'),
    ('r', SWISS_INVS),
    ('r', SWISS_TERMS),
    #('r', 'Partial'),
  ]

  folsep_json = read_folsep_json(input_directory)
  i4_data = read_I4_data(input_directory)

  def calc(r, c):
    if c == "Benchmark":
      bname = get_bench_name(r)
      if bname == "multi-paxos" and "basic" in r:
        return bname + "$^*$"
      return bname
    elif c == "I4~\\cite{I4}":
      i4_time = I4_get_res(i4_data, r)

      if i4_time == None:
        return Hatch()
      else:
        return str(int(float(i4_time)))
    elif c == "FOL~\\cite{fol-sep}":
      folsep_time = folsep_json_get_res(folsep_json, r)
      if folsep_time == None:
        return Hatch()
      else:
        return str(int(float(folsep_time)))
    elif c == '$\\exists$?':
      return "$\\checkmark$" if get_bench_existential(r) else ""
    elif c == 'invs':
      x = get_bench_num_handwritten_invs(r)
      if x == -1:
        return "TODO"
      else:
        return str(x)
    elif c == 'terms':
      x = get_bench_num_handwritten_terms(r)
      if x == -1:
        return "TODO"
      else:
        return str(x)
    else:
      if stats[r] == None:
        return "TODO"
      if stats[r].timed_out_6_hours:
        if c == "\\name":
          partial_inv_counts = {
            "mm_nonacc__chain__auto__seed1_t8"  :  6,
            "mm_nonacc__chord__auto__seed1_t8"  :  8,
            "mm_nonacc__distributed_lock__auto9__seed1_t8"  :   1,
            "mm_nonacc__hybrid_reliable_broadcast_cisa_pyv__auto__seed1_t8"  :   1,
            "mm_nonacc__sharded_kv_no_lost_keys_pyv__auto9__seed1_t8"  :   0,
            "mm_nonacc__ticket_pyv__auto__seed1_t8"  :   5,
            "mm_nonacc__multi_paxos__auto__seed1_t8"  :   6,
            "mm_nonacc__vertical_paxos__auto__seed1_t8"  :   15,
            "mm_nonacc__fast_paxos__auto__seed1_t8" :   8,
            "mm_nonacc__stoppable_paxos__auto__seed1_t8" :   6,
          }
          assert stats[r].filename
          return (
            '(' + str(partial_inv_counts[stats[r].filename])
            + ' / ' +
            str(get_bench_num_handwritten_invs(r)) + ')'
          )

        else:
          return ""
      if not stats[r].success:
        return "TODO"

      if c == "$n_B$":
        return stats[r].num_breadth_iters
      elif c == "$t_F$":
        if stats[r].num_finisher_iters > 0:
          return int(stats[r].finisher_time_sec)
        else:
          return "-"
      elif c == "$t_B$":
        return int(stats[r].breadth_total_time_sec)
      elif c == "\\name":
        return int(stats[r].total_time_sec)
      elif c == SWISS_INVS:
        return str(stats[r].num_inv)
      elif c == SWISS_TERMS:
        return str(terms[stats[r].filename])
      else:
        assert False, c

  t = Table(cols, rows, calc)
  t.dump()

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

    full_name = os.path.join(input_directory, s.filename)

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
    ('l', 'Benchmark'),
    ('r', 'Baseline'),
    ('r', 'Symmetries'),
    ('r', 'Counterexample filtering'),
    ('r', 'FastImplies'),
    '||',
    ('r', 'Invariants'),
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

    if col == 'Benchmark':
      #return bench.name + (' (B)' if alg == 'breadth' else ' (F)')
      rname = get_bench_name(r)
      if rname == 'paxos-missing1':
        rname = 'paxos'
      return rname + (' (B)' if alg == 'breadth' else ' (F)')
    elif col == 'Baseline':
      return counts[row].presymm
    elif col == 'Symmetries':
      return counts[row].postsymm
    elif col == 'Counterexample filtering':
      return (stats["Counterexamples of type FALSE"]
          + stats["Counterexamples of type TRANSITION"]
          + stats["Counterexamples of type TRUE"]
          + stats["number of non-redundant invariants found"]
          + stats["number of redundant invariants found"]
          + stats["number of finisher invariants found"]
          + stats["number of enumerated filtered redundant invariants"]
        )

    elif col == 'FastImplies':
      return (stats["Counterexamples of type FALSE"]
          + stats["Counterexamples of type TRANSITION"]
          + stats["Counterexamples of type TRUE"]
          + stats["number of non-redundant invariants found"]
          + stats["number of redundant invariants found"]
          + stats["number of finisher invariants found"]
        )

    elif col == 'Invariants':
      return (
            stats["number of non-redundant invariants found"]
          + stats["number of redundant invariants found"]
          + stats["number of finisher invariants found"]
        )

  t = Table(columns, rows, calc)
  t.dump()

def make_table(input_directory, which):
  s = [
    BasicStats(input_directory, "Simple decentralized lock", "mm_wc_sdl"),
    BasicStats(input_directory, "Leader election (1)", "mm_wc_leader_election_breadth", I4='6.1'),
    BasicStats(input_directory, "Leader election (2)", "mm_wc_leader_election_fin", I4='6.1'),
    BasicStats(input_directory, "Two-phase commit", "mm_wc_2pc", I4='4.3'),
    BasicStats(input_directory, "Lock server", "mm_wc_lock_server", I4='0.8'),
    BasicStats(input_directory, "Learning switch", "mm_wc_learning_switch", I4='10.7'),
    BasicStats(input_directory, "Paxos", "mm_wc_bt_paxos"),
    BasicStats(input_directory, "Flexible Paxos", "mm_wc_bt_flexible_paxos"),
    BasicStats(input_directory, "Multi-Paxos", "mm_wc_bt_multi_paxos"),
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

segment1_color = '#009e73'
segment2_color = '#f0e442'
filter_color = 'black'
cex_color =  '#d55e00'
redundant_color = '#0072b2'
nonredundant_color = '#56b4e9'
other_color = '#cc79a7'

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
      patches.Patch(color=filter_color, label='Filtering'),
      patches.Patch(color=cex_color, label='Counterex.'),
      patches.Patch(color=redundant_color, label='Redundant'),
      #patches.Patch(color=nonredundant_color, label='Non-red.'),
      patches.Patch(color=other_color, label='Other'),
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
          longest = get_longest(fs)
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

            ax.bar(center, b-a, bottom=a, width=width, color=other_color, edgecolor='black')
            ax.bar(center, c-b, bottom=b, width=width, color=redundant_color, edgecolor='black')
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
  with open(os.path.join(input_directory, filename)) as f:
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
    opts = ['', 'mm_', 'prebmc_mm_']
    names = ['Baseline', 'Min-models', 'Min-models + BMC']
  else:
    assert False

  #colors = ['black', '#ff8080', '#8080ff', '#80ff80']
  colors = ['black', '#E69F00', '#56b4e9']

  prob_data = [
      ('leader-election__basic_b__seed1_t8'),
      ('2PC__basic__seed1_t8'),
      ('learning-switch__basic__seed1_t8'),
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

PAXOS_FINISHER_THREAD_BENCH = "mm__paxos_epr_missing1__basic__seed1" # _t1 _t2 ... _t8
PAXOS_NONACC_THREAD_BENCH = "mm_nonacc__paxos__basic_b__seed1"
  
def make_parallel_graphs(input_directory, save=False):
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

  plt.tight_layout()

  if save:
    plt.savefig(os.path.join(output_directory, 'paxos-parallel.png'))
  else:
    plt.show()

def make_opt_graphs_main(input_directory, save=False, mm=False, both=False):
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
      plt.savefig(os.path.join(output_directory, 'opt-comparison-both.png'))
    elif mm:
      plt.savefig(os.path.join(output_directory, 'opt-comparison-mm.png'))
    else:
      plt.savefig(os.path.join(output_directory, 'opt-comparison-bmc.png'))
  else:
    plt.show()

def make_seed_graphs_main(input_directory, save=False):
  output_directory = "graphs"
  Path(output_directory).mkdir(parents=True, exist_ok=True)

  fig, ax = plt.subplots(nrows=1, ncols=4, figsize=[12, 3])
  plt.gcf().subplots_adjust(bottom=0.20)

  make_seed_graph(ax.flat[0], input_directory, "wc_learning_switch", title="Learning switch (BreadthAccumulative)")
  make_seed_graph(ax.flat[1], input_directory, "wc_bt_paxos", title="Paxos (BreadthAccumulative)", skip_f=True)
  make_seed_graph(ax.flat[2], input_directory, "wc_bt_paxos", title="Paxos (Finisher)", skip_b=True)
  make_seed_graph(ax.flat[3], input_directory, "wholespace_finisher_bt_paxos", title="Paxos (Finisher, entire space)")

  fig.tight_layout()

  if save:
    plt.savefig(os.path.join(output_directory, 'seed.png'))
  else:
    plt.show()

def misc_stats(input_directory):
  def p(key, value, comment=""):
    print("\\newcommand{\\" + key + "}{" + str(value) + "}" +
        ("" if comment == "" else " % " + str(comment)))

  paxos_1_threads = get_basic_stats(input_directory, PAXOS_FINISHER_THREAD_BENCH + "_t1")
  paxos_2_threads = get_basic_stats(input_directory, PAXOS_FINISHER_THREAD_BENCH + "_t2")
  paxos_8_threads = get_basic_stats(input_directory, PAXOS_FINISHER_THREAD_BENCH + "_t8")

  paxos_b_1_threads = get_basic_stats(input_directory, PAXOS_NONACC_THREAD_BENCH + "_t1")
  paxos_b_8_threads = get_basic_stats(input_directory, PAXOS_NONACC_THREAD_BENCH + "_t8")

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

  p("flexiblePaxosMMSpeedup", speedup(
      get_basic_stats(input_directory, 'mm__flexible_paxos__basic__seed1_t8'),
      get_basic_stats(input_directory, '_flexible_paxos__basic__seed1_t8')))

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
  p("paxosUserGuidanceSavingTime", speedup(
      get_basic_stats(input_directory, "mm_nonacc__paxos__basic__seed1_t8"),
      median(input_directory, "mm_nonacc__paxos__auto__seed#_t8", 1, 5)))

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
      + paxos_1_threads.cex_trans
      + paxos_1_threads.inv_indef)

  total_solved = 0
  total_solved_breadth_only = 0
  total_solved_finisher_only = 0

  for r in MAIN_TABLE_ROWS:
    if r == '||':
      continue
    stats = median(input_directory, r, 1, 5)  
    if not stats.timed_out_6_hours and stats.success:
      total_solved += 1
      if stats.num_finisher_iters == 0:
        total_solved_breadth_only += 1
      s = r.replace('#', '1').split('__')
      s[0] += "_fonly"
      fonly_name = '__'.join(s)

      maps = {
        "mm_nonacc__leader-election__auto__seed#_t8":
            "mm_nonacc_fonly__leader-election__auto_full__seed1_t8",
        "mm_nonacc__learning-switch__auto_e0__seed#_t8":
            "mm_nonacc_fonly__learning-switch__auto_e0_full__seed1_t8",
        "mm_nonacc__lock_server__auto__seed#_t8":
            "mm_nonacc_fonly__lock_server__auto_full__seed1_t8",
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
      return sizes[r.split('__')[2]]
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
  #make_table(input_directory, 0)
  #stuff()
  #make_parallel_graphs(input_directory)
  #make_seed_graphs_main(input_directory)
  #make_smt_stats_table(input_directory)
  #make_opt_graphs_main(input_directory, both=True)
  #make_optimization_step_table(input_directory)
  #make_comparison_table(input_directory)
  misc_stats(input_directory)
  #templates_table(input_directory)

if __name__ == '__main__':
  main()
