import sys
import os
import subprocess
import shutil
import time
from pathlib import Path
import queue
import threading

NUM_PARTS = 37

class PaperBench(object):
  def __init__(self, ivyname, config, threads=24, seed=1, mm=True, pre_bmc=False, post_bmc=False, nonacc=False, whole=False, expect_success=True):
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

    self.partition = 1

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

    assert self.ivyname.endswith(".ivy")
    name += "_" + self.ivyname[:-4]
    name += "__" + self.config
    name += "__seed" + str(self.seed) + "_t" + str(self.threads)

    return name

  def get_args(self):
    return ([os.path.join("benchmarks", self.ivyname), "--config", self.config, "--threads", str(self.threads),
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

for i in range(THREADS, 0, -1):
  benches.append(PaperBench("paxos.ivy", "basic_b", nonacc=True, threads=i, expect_success=False))
  benches.append(PaperBench("paxos.ivy", "basic_b", threads=i, expect_success=False))

  benches.append(PaperBench("paxos_epr_missing1.ivy", "basic", threads=i))

  benches.append(PaperBench("chord-gimme-1.ivy", "basic", threads=i))
  benches.append(PaperBench("chord-gimme-1.ivy", "basic", threads=i, nonacc=True))

  benches.append(PaperBench("multi_paxos.ivy", "basic", threads=i, whole=True, expect_success=False))
  benches.append(PaperBench("multi_paxos.ivy", "basic", threads=i, whole=True, expect_success=False, nonacc=True))
  benches.append(PaperBench("flexible_paxos.ivy", "basic", threads=i, whole=True, expect_success=False))
  benches.append(PaperBench("flexible_paxos.ivy", "basic", threads=i, whole=True, expect_success=False, nonacc=True))

for minmodels in (True, False):
  for postbmc in [False]: #(False, True):
    for prebmc in (False, True):
      for nonacc in (True, False):
        args = {"mm" : minmodels, "post_bmc" : postbmc, "pre_bmc" : prebmc, "nonacc" : nonacc, "threads" : 8 }
        benches.append(PaperBench("simple-de-lock.ivy", config="basic", **args))
        benches.append(PaperBench("leader-election.ivy", config="basic_b", **args))
        benches.append(PaperBench("leader-election.ivy", config="basic_f", **args))
        benches.append(PaperBench("learning-switch.ivy", config="basic", **args))
        benches.append(PaperBench("lock_server.ivy", config="basic", **args))
        benches.append(PaperBench("2PC.ivy", config="basic", **args))
        benches.append(PaperBench("paxos.ivy", config="basic", **args))
        benches.append(PaperBench("multi_paxos.ivy", config="basic", **args))
        benches.append(PaperBench("flexible_paxos.ivy", config="basic", **args))
        benches.append(PaperBench("chord-gimme-1.ivy", config="basic", **args))
        benches.append(PaperBench("decentralized-lock-gimme-1.ivy", config="basic", **args))

for seed in range(1, 9):
  benches.append(PaperBench("learning_switch.ivy", "basic", threads=THREADS, seed=seed, nonacc=True))
  benches.append(PaperBench("learning_switch.ivy", "basic", threads=THREADS, seed=seed))

  benches.append(PaperBench("paxos.ivy", "basic", threads=THREADS, seed=seed, nonacc=True))
  benches.append(PaperBench("paxos.ivy", "basic", threads=THREADS, seed=seed))

  benches.append(PaperBench("paxos.ivy", "basic_b", threads=THREADS, seed=seed, nonacc=True, expect_success=False))
  benches.append(PaperBench("paxos.ivy", "basic_b", threads=THREADS, seed=seed, expect_success=False))

  benches.append(PaperBench("paxos_epr_missing1.ivy", "basic", threads=THREADS, whole=True, seed=seed, nonacc=True, expect_success=False))
  benches.append(PaperBench("paxos_epr_missing1.ivy", "basic", threads=THREADS, whole=True, seed=seed, expect_success=False))

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
  return "\nSuccess: True" in out

def run(directory, bench):
  if exists(directory, bench):
    print("already done " + bench.name)
    return

  print("doing " + bench.name) 
  sys.stdout.flush()

  t1 = time.time()

  proc = subprocess.Popen(["./save.sh"] + bench.args,
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
print 'IGNORING nonacc'
benches = [b for b in benches if not b.nonacc]

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
