import sys
import os
import subprocess
import shutil
import time
from pathlib import Path
import queue
import threading

NUM_PARTS = 43

class PaperBench(object):
  def __init__(self, partition, ivyname, config, threads=8, seed=1, mm=True, pre_bmc=False, post_bmc=False, nonacc=False, whole=False, expect_success=True):
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
    if self.ivyname.endswith(".ivy"):
      p = os.path.join("benchmarks", self.ivyname)
    else:
      p = os.path.join("pyv_benchmarks", self.ivyname)

    return ([p, "--config", self.config, "--threads", str(self.threads),
        "--seed", str(self.seed), "--with-conjs", "--breadth-with-conjs"]
      + (["--minimal-models"] if self.mm else [])
      + (["--pre-bmc"] if self.pre_bmc else [])
      + (["--post-bmc"] if self.post_bmc else [])
      + (["--by-size", "--non-accumulative"] if self.nonacc else [])
      + (["--whole-space"] if self.whole else [])
    )

benches = [ ]

THREADS = 8

#for i in range(THREADS, 0, -1):
#  benches.append(PaperBench(25,
#      "wc_chord_t" + str(i),
#      "chord-gimme-1 --breadth-with-conjs --minimal-models --whole-space --threads " + str(i)))

#for i in range(THREADS, 0, -1):
#  benches.append(PaperBench(4,
#      "nonacc_wc_chord_t" + str(i),
#      "chord-gimme-1 --breadth-with-conjs --by-size --non-accumulative --minimal-models --whole-space --threads " + str(i)))

for seed in range(1, 2): #6):
  benches.append(PaperBench(6, "simple-de-lock.ivy", config="auto", seed=seed))
  benches.append(PaperBench(6, "leader-election.ivy", config="auto", seed=seed))
  benches.append(PaperBench(6, "learning-switch.ivy", config="auto_e0", seed=seed))
  benches.append(PaperBench(3, "learning-switch.ivy", config="auto", seed=seed))
  benches.append(PaperBench(6, "lock_server.ivy", config="auto", seed=seed))
  benches.append(PaperBench(6, "2PC.ivy", config="auto", seed=seed))
  benches.append(PaperBench(6, "paxos.ivy", config="auto", seed=seed))
  benches.append(PaperBench(7, "multi_paxos.ivy", config="auto", seed=seed))
  benches.append(PaperBench(6, "flexible_paxos.ivy", config="auto", seed=seed))
  benches.append(PaperBench(9, "chord-gimme-1.ivy", config="auto", seed=seed))
  benches.append(PaperBench(10, "decentralized-lock-gimme-1.ivy", config="auto", seed=seed))

  benches.append(PaperBench(6, "client_server_ae.pyv", config="auto", seed=seed))
  benches.append(PaperBench(12, "client_server_db_ae.pyv", config="auto", seed=seed))
  benches.append(PaperBench(6, "consensus_epr.pyv", config="auto", seed=seed))
  benches.append(PaperBench(6, "consensus_forall.pyv", config="auto", seed=seed))
  benches.append(PaperBench(6, "consensus_wo_decide.pyv", config="auto", seed=seed))
  benches.append(PaperBench(16, "firewall.pyv", config="auto", seed=seed))
  benches.append(PaperBench(17, "hybrid_reliable_broadcast_cisa.pyv", config="auto", seed=seed))
  benches.append(PaperBench(6, "learning_switch.pyv", config="auto", seed=seed))
  benches.append(PaperBench(19, "lockserv.pyv", config="auto", seed=seed))
  benches.append(PaperBench(6, "lockserv.pyv", config="auto9", seed=seed))
  benches.append(PaperBench(6, "ring_id.pyv", config="auto", seed=seed))
  benches.append(PaperBench(21, "ring_id_not_dead.pyv", config="auto", seed=seed))
  benches.append(PaperBench(22, "sharded_kv.pyv", config="auto", seed=seed))
  benches.append(PaperBench(6, "sharded_kv.pyv", config="auto9", seed=seed))
  benches.append(PaperBench(23, "sharded_kv_no_lost_keys.pyv", config="auto", seed=seed))
  benches.append(PaperBench(23, "sharded_kv_no_lost_keys.pyv", config="auto9", seed=seed))
  benches.append(PaperBench(24, "ticket.pyv", config="auto", seed=seed))
  benches.append(PaperBench(6, "toy_consensus_epr.pyv", config="auto", seed=seed))
  benches.append(PaperBench(6, "toy_consensus_forall.pyv", config="auto", seed=seed))

for i in range(THREADS, 0, -1):
  benches.append(PaperBench(6, "paxos.ivy", "basic_b", nonacc=True, threads=i, expect_success=False))
  benches.append(PaperBench(6, "paxos.ivy", "basic_b", threads=i, expect_success=False))

  benches.append(PaperBench(6, "paxos_epr_missing1.ivy", "basic", threads=i))

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
        benches.append(PaperBench(c, "learning-switch.ivy", config="basic", **args))
        benches.append(PaperBench(c, "lock_server.ivy", config="basic", **args))
        benches.append(PaperBench(c, "2PC.ivy", config="basic", **args))
        benches.append(PaperBench(c, "paxos.ivy", config="basic", **args))
        benches.append(PaperBench(c, "multi_paxos.ivy", config="basic", **args))
        benches.append(PaperBench(c, "flexible_paxos.ivy", config="basic", **args))
        #benches.append(PaperBench(c, "chord-gimme-1.ivy", config="basic", **args))
        #benches.append(PaperBench(c, "decentralized-lock-gimme-1.ivy", config="basic", **args))

for seed in range(1, 9):
  #benches.append(PaperBench(40, "learning-switch.ivy", "basic", threads=THREADS, seed=seed, nonacc=True))
  benches.append(PaperBench(40, "learning-switch.ivy", "basic", threads=THREADS, seed=seed))

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

  proc = subprocess.Popen(["mktemp", log_start+dt+"-XXXXXXXXX"],
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

  logfile = make_logfile()

  print("doing " + bench.name + "    " + logfile)

  sys.stdout.flush()

  t1 = time.time()

  env = dict(os.environ)
  env["SYNTHESIS_LOGFILE"] = logfile

  proc = subprocess.Popen(["./save.sh"] + bench.args,
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      env=env)
  out, err = proc.communicate()
  ret = proc.wait()
  assert ret == 0

  t2 = time.time()
  seconds = t2 - t1

  statfile = get_statfile(out)
  if statfile is None:
    print("failed " + bench.name + " (" + str(seconds) + " seconds) " + logfile)
    return False
  else:
    print("done " + bench.name + " (" + str(seconds) + " seconds) " + logfile)
    result_filename = os.path.join(directory, bench.name)
    shutil.copy(statfile, result_filename)

    if bench.expect_success and not success_true(out):
      print("WARNING: " + bench.name + " did not succeed")
      
    return True

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
