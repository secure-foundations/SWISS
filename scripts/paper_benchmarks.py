import sys
import os
import subprocess
import shutil
import time
from pathlib import Path
import queue
import threading

NUM_PARTS = 37

def get_num_threads(args):
  args = args.split()
  for i in range(len(args)):
    if args[i] == '--threads':
      return int(args[i+1])
  assert False

class PaperBench(object):
  def __init__(self, ivyname, config, threads=24, seed=1, mm=True, pre_bmc=False, post_bmc=False, nonacc=False, whole=False):
    self.ivyname = ivyname
    self.config = config
    self.threads = threads
    self.seed = seed
    self.mm = mm
    self.pre_bmc = pre_bmc
    self.post_bmc = post_bmc
    self.nonacc = nonacc
    self.args = args
    self.whole = whole

  def name(self):
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

    assert self.ivyname.endswith(".ivy")
    name += "_" + self.ivyname[:-4]
    name += "__" + self.config
    name += "__seed_" + str(self.seed) + "_t" + str(self.threads)

    return name

  def args(self):
    return ([self.ivyname, "--config", self.config, "--threads", str(self.threads),
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

for minmodels in (True, False):
  for postbmc in [False]: #(False, True):
    for prebmc in (False, True):
      for nonacc in (True, False):
        args = {"mm" : minmodels, "post_bmc" : postbmc, "pre_bmc" : prebmc, "nonacc" : nonacc, "threads" : 8 }
        benches.append("simple-de-lock.ivy", config="basic", **args)
        benches.append("leader-election.ivy", config="basic_b", **args)
        benches.append("leader-election.ivy", config="basic_f", **args)
        benches.append("learning-switch.ivy", config="basic", **args)
        benches.append("lock_server.ivy", config="basic", **args)
        benches.append("2PC.ivy", config="basic", **args)
        benches.append("paxos.ivy", config="basic", **args)
        benches.append("multi_paxos.ivy", config="basic", **args)
        benches.append("flexible_paxos.ivy", config="basic", **args)
        benches.append("chord-gimme-1.ivy", config="basic", **args)
        benches.append("decentralized-lock-gimme-1.ivy", config="basic", **args)

benches.append(PaperBench(17, "fail_wc_bt_vertical", "better-template-vertical-paxos --breadth-with-conjs--minimal-models --threads 7"))
benches.append(PaperBench(17, "fail_wc_bt_stoppable", "better-template-stoppable-paxos --breadth-with-conjs --minimal-models --threads 7"))

for seed in range(1, 8):
  benches.append(PaperBench(18,
      "nonacc_wc_learning_switch_seed_" + str(seed),
      "breadth-learning-switch --breadth-with-conjs --by-size --non-accumulative --minimal-models --threads " + str(THREADS) + " --seed " + str(seed)))

for seed in range(1, 9):
  benches.append(PaperBench(19,
      "wc_learning_switch_seed_" + str(seed),
      "breadth-learning-switch --breadth-with-conjs --minimal-models --threads " + str(THREADS) + " --seed " + str(seed)))

for seed in range(1, 5):
  benches.append(PaperBench(20,
      "nonacc_wc_bt_paxos_seed_" + str(seed),
      "better-template-paxos --breadth-with-conjs --by-size --non-accumulative --minimal-models --threads " + str(THREADS) + " --seed " + str(seed)))

for seed in range(1, 9):
  benches.append(PaperBench(21,
      "wc_bt_paxos_seed_" + str(seed),
      "better-template-paxos --breadth-with-conjs --minimal-models --threads " + str(THREADS) + " --seed " + str(seed)))

for seed in range(1, 9):
  benches.append(PaperBench(22,
      "wholespace_finisher_bt_paxos_seed_" + str(seed),
      "better-template-paxos-finisher --whole-space --minimal-models --threads " + str(THREADS) + " --seed " + str(seed)))

for seed in range(1, 5):
  benches.append(PaperBench(23,
      "nonacc_wc_bt_paxos_breadth_seed_" + str(seed),
      "better-template-paxos-breadth4 --breadth-with-conjs --non-accumulative --minimal-models --threads 1 --seed " + str(200 + seed)))
for seed in range(1, 8):
  benches.append(PaperBench(24,
      "wc_bt_paxos_breadth_seed_" + str(seed),
      "better-template-paxos-breadth4 --breadth-with-conjs --minimal-models --threads 1 --seed " + str(300 + seed)))


all_names = [b.name for b in benches]
assert len(all_names) == len(list(set(all_names))) # check uniqueness

for b in benches:
  assert 1 <= b.partition <= NUM_PARTS

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

def run(directory, bench):
  if exists(directory, bench):
    print("already done " + bench.name)
    return

  print("doing " + bench.name) 
  sys.stdout.flush()

  t1 = time.time()

  proc = subprocess.Popen(["./bench.sh"] + bench.args.split(),
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE)
  out, err = proc.communicate()
  ret = proc.wait()
  assert ret == 0

  t2 = time.time()
  seconds = t2 - t1

  statfile = get_statfile(out)
  if statfile is None:
    print("failed " + bench.name + " (" + str(seconds) + " seconds)")
    return False
  else:
    print("done " + bench.name + " (" + str(seconds) + " seconds)")
    result_filename = os.path.join(directory, bench.name)
    shutil.copy(statfile, result_filename)
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
