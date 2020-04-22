import sys
import os
import subprocess
import shutil
import time
from pathlib import Path

class PaperBench(object):
  def __init__(self, name, args):
    self.name = name
    self.args = args

benches = [ ]

THREADS = 7

for i in range(THREADS, 0, -1):
  benches.append(PaperBench(
      "paxos_breadth_t" + str(i),
      "breadth-paxos-4-r3 --minimal-models --threads " + str(i)))

for seed in range(1, 15):
  benches.append(PaperBench(
      "learning_switch_seed_" + str(seed),
      "breadth-learning-switch --minimal-models --threads " + str(THREADS) + " --seed " + str(seed)))

for seed in range(1, 15):
  benches.append(PaperBench(
      "paxos_seed_" + str(seed),
      "full-paxos-depth2 --minimal-models --threads " + str(THREADS) + " --seed " + str(seed)))

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

all_names = [b.name for b in benches]
assert len(all_names) == len(list(set(all_names))) # check uniqueness

def get_statfile(out):
  for line in out.split(b'\n'):
    if line.startswith(b"statfile: "):
      t = line.split()
      assert len(t) == 2
      return t[1]
  assert False

def run(directory, bench):
  result_filename = os.path.join(directory, bench.name)
  if os.path.exists(result_filename):
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
  print("done (" + str(seconds) + " seconds)")

  statfile = get_statfile(out)
  shutil.copy(statfile, result_filename)

def main():
  assert len(sys.argv) == 2
  directory = sys.argv[1]
  Path(directory).mkdir(parents=True, exist_ok=True)
  for b in benches:
    run(directory, b)

if __name__ == "__main__":
  main()
