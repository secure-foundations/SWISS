import sys
import os
import subprocess
import shutil
import time
from pathlib import Path
import queue
import threading
import signal
import atexit
import traceback
import protocol_parsing
import json

NUM_PARTS = 100

TIMEOUT_SECS = 6*3600

all_procs = []

DEFAULT_THREADS = 8

class PaperBench(object):
  def __init__(self, partition, ivyname, config, threads=DEFAULT_THREADS, seed=1, mm=True, pre_bmc=False, post_bmc=False, nonacc=False, whole=False, expect_success=True, finisher_only=False):
    self.partition = partition

    self.ivyname = ivyname
    self.config = config
    self.threads = threads
    self.seed = seed
    self.mm = mm
    self.pre_bmc = pre_bmc
    self.post_bmc = post_bmc
    self.nonacc = nonacc
    self.whole = whole
    self.expect_success = expect_success
    self.finisher_only = finisher_only

    self.args = self.get_args()
    self.name = self.get_name()

  def get_name(self):
    name = ""
    if self.post_bmc:
      name += "postbmc_"
    if self.pre_bmc:
      name += "prebmc_"
    if self.mm:
      name += "mm_"
    if self.nonacc:
      name += "nonacc_"
    if self.whole:
      name += "whole_"
    if self.finisher_only:
      name += "fonly_"

    name += "_"

    assert self.ivyname.endswith(".pyv") or self.ivyname.endswith(".ivy")

    if self.ivyname.endswith(".ivy"):
      name += self.ivyname[:-4]
    else:
      name += self.ivyname[:-4] + "_pyv"

    name += "__" + self.config
    name += "__seed" + str(self.seed) + "_t" + str(self.threads)

    return name

  def get_args(self):
    p = os.path.join("benchmarks", self.ivyname)

    return ([p, "--config", self.config, "--threads", str(self.threads),
        "--seed", str(self.seed), "--with-conjs", "--breadth-with-conjs"]
      + (["--minimal-models"] if self.mm else [])
      + (["--pre-bmc"] if self.pre_bmc else [])
      + (["--post-bmc"] if self.post_bmc else [])
      + (["--by-size", "--non-accumulative"] if self.nonacc else [])
      + (["--whole-space"] if self.whole else [])
      + (["--finisher-only"] if self.finisher_only else [])
    )

benches = [ ]

THREADS = 8

benches.append(PaperBench(6, "paxos.ivy", config="auto_breadth", seed=1, nonacc=True))
benches.append(PaperBench(1, "multi_paxos.ivy", config="auto_breadth", seed=1, nonacc=True))
benches.append(PaperBench(6, "flexible_paxos.ivy", config="auto_breadth", seed=1, nonacc=True))
benches.append(PaperBench(1, "fast_paxos.ivy", config="auto", seed=1, nonacc=True))
benches.append(PaperBench(1, "stoppable_paxos.ivy", config="auto", seed=1, nonacc=True))
benches.append(PaperBench(1, "vertical_paxos.ivy", config="auto", seed=1, nonacc=True))

for seed in range(1, 6):
  for fonly in (True, False):
    if fonly and seed > 2:
      continue

    benches.append(PaperBench(8, "simple-de-lock.ivy", config="auto", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(8, "leader-election.ivy", config="auto_full", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(8, "learning-switch-ternary.ivy", config="auto_e0_full", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(6, "learning-switch-ternary.ivy", config="auto", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(8, "lock-server-sync.ivy", config="auto_full", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(8, "2PC.ivy", config="auto_full", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(6, "paxos.ivy", config="auto", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(4, "multi_paxos.ivy", config="basic", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(1, "multi_paxos.ivy", config="auto", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(6, "flexible_paxos.ivy", config="auto", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(1, "fast_paxos.ivy", config="auto", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(1, "stoppable_paxos.ivy", config="auto", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(1, "vertical_paxos.ivy", config="auto", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(1, "chain.ivy", config="auto", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(1, "chord.ivy", config="auto", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(1, "distributed_lock.ivy", config="auto9", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(9, "chord-gimme-1.ivy", config="auto", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(10, "decentralized-lock-gimme-1.ivy", config="auto", seed=seed, nonacc=True, finisher_only=fonly))

    benches.append(PaperBench(8, "client_server_ae.pyv", config="auto", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(8, "client_server_db_ae.pyv", config="auto", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(8, "consensus_epr.pyv", config="auto", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(9, "consensus_forall.pyv", config="auto", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(9, "consensus_wo_decide.pyv", config="auto", seed=seed, nonacc=True, finisher_only=fonly))
    #benches.append(PaperBench(9, "firewall.pyv", config="auto", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(9, "hybrid_reliable_broadcast_cisa.pyv", config="auto", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(9, "learning-switch-quad.pyv", config="auto", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(19, "lock-server-async.pyv", config="auto", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(9, "lock-server-async.pyv", config="auto9", seed=seed, nonacc=True, finisher_only=fonly))
    #benches.append(PaperBench(9, "ring_id_not_dead.pyv", config="auto", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(22, "sharded_kv.pyv", config="auto", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(9, "sharded_kv.pyv", config="auto9", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(9, "sharded_kv_no_lost_keys.pyv", config="auto", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(9, "sharded_kv_no_lost_keys.pyv", config="auto9", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(9, "ticket.pyv", config="auto", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(9, "toy_consensus_epr.pyv", config="auto", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(9, "toy_consensus_forall.pyv", config="auto", seed=seed, nonacc=True, finisher_only=fonly))
    benches.append(PaperBench(4, "sharded_kv_no_lost_keys.pyv", config="auto_e2", seed=seed, nonacc=True))



benches.append(PaperBench(2, "paxos.ivy", config="auto_breadth", nonacc=True, expect_success=False))
benches.append(PaperBench(2, "paxos.ivy", config="auto_finisher", nonacc=True, expect_success=False))

benches.append(PaperBench(3, "paxos_epr_missing1.ivy", config="wrong1", nonacc=True, expect_success=False))
benches.append(PaperBench(3, "paxos_epr_missing1.ivy", config="wrong2", nonacc=True, expect_success=False))
benches.append(PaperBench(3, "paxos_epr_missing1.ivy", config="wrong3", nonacc=True, expect_success=False))
benches.append(PaperBench(3, "paxos_epr_missing1.ivy", config="wrong4", nonacc=True, expect_success=False))
benches.append(PaperBench(3, "paxos_epr_missing1.ivy", config="wrong5", nonacc=True, expect_success=False))

benches.append(PaperBench(3, "paxos_epr_missing1.ivy", config="basic", nonacc=True, expect_success=False, whole=True))
benches.append(PaperBench(3, "paxos_epr_missing1.ivy", config="basic2", nonacc=True, expect_success=False, whole=True))

benches.append(PaperBench(4, "paxos_epr_missing1.ivy", config="basic", nonacc=True))
benches.append(PaperBench(4, "paxos_epr_missing1.ivy", config="basic2", nonacc=True))

benches.append(PaperBench(11, "paxos_epr_missing1.ivy", "basic", threads=1, whole=True, expect_success=False))
benches.append(PaperBench(11, "paxos_epr_missing1.ivy", "basic", threads=2, whole=True, expect_success=False))
benches.append(PaperBench(11, "paxos_epr_missing1.ivy", "basic", threads=4, whole=True, expect_success=False))
benches.append(PaperBench(11, "paxos_epr_missing1.ivy", "basic", threads=3, whole=True, expect_success=False))
benches.append(PaperBench(11, "paxos_epr_missing1.ivy", "basic", threads=5, whole=True, expect_success=False))
benches.append(PaperBench(11, "paxos_epr_missing1.ivy", "basic", threads=6, whole=True, expect_success=False))
benches.append(PaperBench(11, "paxos_epr_missing1.ivy", "basic", threads=7, whole=True, expect_success=False))

for i in range(THREADS, 0, -1):
  benches.append(PaperBench(6, "paxos.ivy", "basic_b", nonacc=True, threads=i, expect_success=False))
  benches.append(PaperBench(6, "paxos.ivy", "basic_b", threads=i, expect_success=False))

  benches.append(PaperBench(11, "paxos_epr_missing1.ivy", "basic", threads=i, whole=True, expect_success=False))

  #benches.append(PaperBench(29, "chord-gimme-1.ivy", "basic", threads=i))
  #benches.append(PaperBench(29, "chord-gimme-1.ivy", "basic", threads=i, nonacc=True))

  benches.append(PaperBench(30, "multi_paxos.ivy", "basic", threads=i, whole=True, expect_success=False))
  #benches.append(PaperBench(30, "multi_paxos.ivy", "basic", threads=i, whole=True, expect_success=False, nonacc=True))
  benches.append(PaperBench(31, "flexible_paxos.ivy", "basic", threads=i, whole=True, expect_success=False))
  #benches.append(PaperBench(31, "flexible_paxos.ivy", "basic", threads=i, whole=True, expect_success=False, nonacc=True))

for minmodels in (True, False):
  for postbmc in [False]: #(False, True):
    for prebmc in (False, True):
      for nonacc in (False, True):
        c = (
            (1 if minmodels else 0) +
            (2 if prebmc else 0) +
            (4 if nonacc else 0)
        )
        c += 32

        c = 6

        args = {"mm" : minmodels, "post_bmc" : postbmc, "pre_bmc" : prebmc, "nonacc" : nonacc, "threads" : 8 }
        benches.append(PaperBench(c, "simple-de-lock.ivy", config="basic", **args))
        benches.append(PaperBench(c, "leader-election.ivy", config="basic_b", **args))
        benches.append(PaperBench(c, "leader-election.ivy", config="basic_f", **args))
        benches.append(PaperBench(c, "learning-switch-ternary.ivy", config="basic", **args))
        benches.append(PaperBench(c, "lock-server-sync.ivy", config="basic", **args))
        benches.append(PaperBench(c, "2PC.ivy", config="basic", **args))
        benches.append(PaperBench(c, "paxos.ivy", config="basic", **args))
        benches.append(PaperBench(c, "multi_paxos.ivy", config="basic", **args))
        benches.append(PaperBench(c, "flexible_paxos.ivy", config="basic", **args))
        #benches.append(PaperBench(c, "chord-gimme-1.ivy", config="basic", **args))
        #benches.append(PaperBench(c, "decentralized-lock-gimme-1.ivy", config="basic", **args))

for seed in range(1, 9):
  #benches.append(PaperBench(40, "learning-switch-ternary.ivy", "basic", threads=THREADS, seed=seed, nonacc=True))
  benches.append(PaperBench(40, "learning-switch-ternary.ivy", "basic", threads=THREADS, seed=seed))

  #benches.append(PaperBench(41, "paxos.ivy", "basic", threads=THREADS, seed=seed, nonacc=True))
  benches.append(PaperBench(7, "paxos.ivy", "basic", threads=THREADS, seed=seed))

  #benches.append(PaperBench(42, "paxos.ivy", "basic_b", threads=THREADS, seed=seed, nonacc=True, expect_success=False))
  benches.append(PaperBench(7, "paxos.ivy", "basic_b", threads=THREADS, seed=seed, expect_success=False))

  #benches.append(PaperBench(43, "paxos_epr_missing1.ivy", "basic", threads=THREADS, whole=True, seed=seed, nonacc=True, expect_success=False))
  benches.append(PaperBench(7, "paxos_epr_missing1.ivy", "basic", threads=THREADS, whole=True, seed=seed, expect_success=False))

#all_names = list(set([b.name for b in benches]))
#assert len(all_names) == len(list(set(all_names))) # check uniqueness

def unique_benches(old_benches):
  benches = []
  names = set()
  for b in old_benches:
    assert 1 <= b.partition <= NUM_PARTS
    if b.name not in names:
      names.add(b.name)
      benches.append(b)
  return benches

benches = unique_benches(benches)

#for b in benches:
#  print(b.get_args()[0])
#  assert os.path.exists(b.get_args()[0])

def get_statfile(out):
  for line in out.split(b'\n'):
    if line.startswith(b"statfile: "):
      t = line.split()
      assert len(t) == 2
      return t[1]
  return None

def exists(directory, bench):
  result_filename = os.path.join(directory, bench.name)
  return os.path.exists(result_filename)

def success_true(out):
  return b"\nSuccess: True" in out

def collect_partial_invariants(logdir):
  try:
    inv_file = os.path.join(logdir, "invariants")
    if os.path.exists(inv_file):
      print("WARNING: invariants file already exists, this is unexpected")
      return

    i = 0
    while True:
      if os.path.exists(os.path.join(logdir, "partial_invs." + str(i+1) + ".base")):
        i += 1
      else:
        break
    if i == 0:
      print("WARNING: collect_partial_invariants found no base file???")
      return

    t = []

    with open(os.path.join(logdir, "partial_invs." + str(i) + ".base")) as f:
      j = f.read()
      if j != "empty\n":
        j = json.loads(j)
        t = j["base_invs"] + j["new_invs"]

    for fname in os.listdir(logdir):
      if fname.startswith("partial_invs." + str(i) + "."):
        spl = fname.split('.')
        if spl[2] != 'base':
          with open(os.path.join(logdir, fname), "r") as f:
            for line in f:
              if line.strip() != '':
                j = json.loads(line)
                assert type(j) == list
                t.append(j)

    strs = [
      protocol_parsing.value_json_to_string(j)
      for j in t
    ]

    with open(os.path.join(logdir, "invariants"), "w") as f:
      f.write("# timed out and extracted\n")
      for j in strs:
        f.write("conjecture " + j + "\n")
        
  except Exception:
    print("WARNING: failed to collect partial invariants for " + logdir)
    traceback.print_exc()

def copy_dir(out_directory, logdir, bench, out):
  summary_filename = os.path.join(logdir, "summary")
  inv_filename = os.path.join(logdir, "invariants")

  if (
      not os.path.exists(summary_filename) or
      not os.path.exists(inv_filename)):
    return "crash"

  newdir = os.path.join(out_directory, bench.name)
  summary_filename_new = os.path.join(newdir, "summary")
  inv_filename_new = os.path.join(newdir, "invariants")

  os.mkdir(newdir)
  shutil.copy(summary_filename, summary_filename_new)
  shutil.copy(inv_filename, inv_filename_new)

  if success_true(out):
    return "success"
  else:
    return "fail"

def make_logfile():
  proc = subprocess.Popen(["date", "+%Y-%m-%d_%H.%M.%S"],
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE)
  out, err = proc.communicate()
  ret = proc.wait()
  assert ret == 0

  dt = out.strip().decode("utf-8")

  if "SCRATCH" in os.environ:
    scratch = os.environ["SCRATCH"]
    assert "pylon5" in scratch
    log_start = os.path.join(scratch, "log.")
  else:
    log_start = "./logs/log."

  proc = subprocess.Popen(["mktemp", "-d", log_start+dt+"-XXXXXXXXX"],
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE)
  out, err = proc.communicate()
  ret = proc.wait()
  assert ret == 0

  logfile = out.strip().decode("utf-8")
  assert logfile.startswith(log_start)

  return logfile

def run(directory, bench):
  if exists(directory, bench):
    print("already done " + bench.name)
    return

  logdir = make_logfile()

  print("doing " + bench.name + "    " + logdir)

  sys.stdout.flush()

  t1 = time.time()

  env = dict(os.environ)
  env["SYNTHESIS_LOGDIR"] = logdir

  proc = subprocess.Popen(["./save.sh"] + bench.args,
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      env=env,
      preexec_fn=os.setsid)

  all_procs.append(proc)

  try:
    out, err = proc.communicate(timeout=TIMEOUT_SECS)
    timed_out = False
  except subprocess.TimeoutExpired:
    os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
    timed_out = True
  
  if timed_out:
    print("timed out " + bench.name + " (" + str(TIMEOUT_SECS) + " seconds) " + logdir)
    newdir = os.path.join(directory, bench.name)
    os.mkdir(newdir)
    result_filename = os.path.join(newdir, "summary")
    with open(result_filename, "w") as f:
      f.write(" ".join(["./save.sh"] + bench.args) + "\n")
      f.write("TIMED OUT " + str(TIMEOUT_SECS) + " seconds\n")
      f.write(logdir + "\n")
    collect_partial_invariants(logdir)
    shutil.copy(
      os.path.join(logdir, "invariants"),
      os.path.join(newdir, "invariants")
    )
    return False
  else:
    ret = proc.wait()
    assert ret == 0

    t2 = time.time()
    seconds = t2 - t1

    all_procs.remove(proc)

    result = copy_dir(directory, logdir, bench, out)

    assert result in ('crash', 'success', 'fail')

    if result == "crash":
      print("failed " + bench.name + " (" + str(seconds) + " seconds) " + logdir)
      return False
    else:
      print("done " + bench.name + " (" + str(seconds) + " seconds) " + logdir)
      if bench.expect_success and result != 'success':
        print("WARNING: " + bench.name + " did not succeed")

def run_wrapper(directory, bench, idx, q):
  run(directory, bench)
  q.put(idx)

def awesome_async_run(directory, benches, j):
  for bench in benches:
    assert bench.threads <= j

  benches = sorted(benches, key = lambda b : -b.threads)
  done = [False] * len(benches)
  started = [False] * len(benches)
  c = 0
  for i in range(len(benches)):
    if exists(directory, benches[i]):
      print("already done " + benches[i].name)
      done[i] = True
      c += 1
  t = 0

  q = queue.Queue()

  while c < len(benches):
    cur = None
    for i in range(len(benches)):
      if (not done[i]) and (not started[i]) and benches[i].threads + t <= j:
        cur = i
        break
    if cur is None:
      idx = q.get()
      done[idx] = True
      c += 1
      t -= benches[idx].threads
    else:
      thr = threading.Thread(target=run_wrapper, daemon=True,
          args=(directory, benches[cur], cur, q))
      thr.start()
      t += benches[cur].threads
      started[cur] = True

def get_all_main_benchmarks():
  res = []
  for b in benches:
    if b.threads == DEFAULT_THREADS and b.mm and (not b.pre_bmc) and (not b.post_bmc) and b.nonacc and (not b.whole) and (not b.finisher_only):
      res.append(b)
  return res

def parse_args(args):
  res = []
  j = None
  p = None
  one = None
  i = 0
  while i < len(args):
    if args[i] == '-j':
      j = int(args[i+1])
      i += 1
    elif args[i] == '-p':
      p = int(args[i+1])
      i += 1
    elif args[i] == '--one':
      one = args[i+1]
      i += 1
    else:
      res.append(args[i])
    i += 1
  return j, p, one, res

#for b in benches:
#  print(b.name)
#print('IGNORING nonacc')
#benches = [b for b in benches if not b.nonacc]

def cleanup():
  for proc in all_procs: # list of your processes
    try:
      print('killing pid ' + str(proc.pid))
      os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
    except Exception:
      print("warning: could not kill sub process " + str(proc.pid))
      pass

atexit.register(cleanup)

def main():
  args = sys.argv[1:]
  j, p, one, args = parse_args(args)

  assert len(args) == 1
  directory = args[0]
  Path(directory).mkdir(parents=True, exist_ok=True)

  assert directory.startswith("paperlogs/")

  if j == None:
    for b in benches:
      if (p == None or p == b.partition) and (b.name == one or one == None):
        run(directory, b)
  else:
    if p == None:
      awesome_async_run(directory, benches, j)
    else:
      awesome_async_run(directory, [b for b in benches if b.partition == p], j)

  print('done')


if __name__ == "__main__":
  main()
