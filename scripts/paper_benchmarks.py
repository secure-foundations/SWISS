import sys
import os
import subprocess
import shutil
import time
from pathlib import Path
import queue
import threading

def get_num_threads(args):
  args = args.split()
  for i in range(len(args)):
    if args[i] == '--threads':
      return int(args[i+1])
  assert False

class PaperBench(object):
  def __init__(self, name, args):
    self.name = name
    self.args = args
    self.threads = get_num_threads(args)

benches = [ ]

THREADS = 7

for i in range(THREADS, 0, -1):
  benches.append(PaperBench(
      "nonacc_paxos_breadth_t" + str(i),
      "breadth-paxos-4-r3 --by-size --non-accumulative --minimal-models --threads " + str(i)))

for i in range(THREADS, 0, -1):
  benches.append(PaperBench(
      "paxos_breadth_t" + str(i),
      "breadth-paxos-4-r3 --minimal-models --threads " + str(i)))

for seed in range(1, 8):
  benches.append(PaperBench(
      "nonacc_learning_switch_seed_" + str(seed),
      "breadth-learning-switch --by-size --non-accumulative --minimal-models --threads " + str(THREADS) + " --seed " + str(seed)))

for seed in range(1, 5):
  benches.append(PaperBench(
      "nonacc_paxos_seed_" + str(seed),
      "full-paxos-depth2 --by-size --non-accumulative --minimal-models --threads " + str(THREADS) + " --seed " + str(seed)))

for seed in range(1, 5):
  benches.append(PaperBench(
      "nonacc_paxos_breadth_seed_" + str(seed),
      "breadth-paxos-4-r3 --non-accumulative --minimal-models --threads 1 --seed " + str(200 + seed)))
for seed in range(1, 8):
  benches.append(PaperBench(
      "paxos_breadth_seed_" + str(seed),
      "breadth-paxos-4-r3 --minimal-models --threads 1 --seed " + str(300 + seed)))


#for i in range(20, 0, -1):
#  benches.append(PaperBench(
#      "paxos_finisher_t" + str(i),
#      "finisher-paxos-exist-1-depth2 --minimal-models --whole-space --threads " + str(i)))

for i in range(THREADS, 0, -1):
  benches.append(PaperBench(
      "paxos_implshape_finisher_t" + str(i),
      "finisher-paxos-exist-1 --minimal-models --whole-space --threads " + str(i)))


for postbmc in (False, True):
  for prebmc in (False, True):
    for minmodels in [True]: #(True, False):
      name = ""
      args = ""
      if postbmc:
        name += "postbmc_"
        args += " --post-bmc"
      if prebmc:
        name += "prebmc_"
        args += " --pre-bmc"
      if minmodels:
        name += "mm_"
        args += " --minimal-models"
      benches.append(PaperBench(name+"leader_election_fin", "leader-election-depth2 --threads 1"+args))
      benches.append(PaperBench(name+"leader_election_breadth", "breadth-leader-election --threads 4"+args))
      benches.append(PaperBench(name+"learning_switch", "breadth-learning-switch --threads "+str(THREADS)+args))
      benches.append(PaperBench(name+"paxos", "full-paxos-depth2 --threads "+str(THREADS)+args))
      benches.append(PaperBench(name+"flexible_paxos", "full-flexible-paxos-depth2 --threads "+str(THREADS)+args))
      benches.append(PaperBench(name+"lock_server", "breadth-lock-server --threads 1"+args))
      benches.append(PaperBench(name+"2pc", "breadth-2pc --threads 1"+args))

for i in range(3, 0, -1):
  benches.append(PaperBench(
      "chord_breadth_t" + str(i),
      "breadth-chord --minimal-models --threads " + str(i)))

all_names = [b.name for b in benches]
assert len(all_names) == len(list(set(all_names))) # check uniqueness

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
      if not done[i] and benches[i].threads + t <= j:
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

def parse_args(args):
  res = []
  j = None
  i = 0
  while i < len(args):
    if args[i] == '-j':
      j = int(args[i+1])
      i += 1
    else:
      res.append(args[i])
    i += 1
  return j, res

def main():
  args = sys.argv[1:]
  j, args = parse_args(args)

  assert len(args) == 1
  directory = args[0]
  Path(directory).mkdir(parents=True, exist_ok=True)

  if j == None:
    for b in benches:
      run(directory, b)
  else:
    awesome_async_run(directory, benches, j)

if __name__ == "__main__":
  main()
