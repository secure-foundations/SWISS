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
  def __init__(self, partition, name, args):
    self.name = name
    self.args = args
    self.threads = get_num_threads(args)
    self.partition = partition

benches = [ ]

THREADS = 8

benches.append(PaperBench(28, "final-leader-election",
    "final-leader-election --breadth-with-conjs --threads 24 --minimal-models --by-size --non-accumulative"))

benches.append(PaperBench(29, "final-simple-de-lock",
    "final-simple-de-lock --breadth-with-conjs --threads 24 --minimal-models --by-size --non-accumulative"))

benches.append(PaperBench(30, "final-2pc",
    "final-2pc --breadth-with-conjs --threads 24 --minimal-models --by-size --non-accumulative"))

benches.append(PaperBench(31, "final-lock-server",
    "final-lock-server --breadth-with-conjs --threads 24 --minimal-models --by-size --non-accumulative"))

benches.append(PaperBench(32, "final-learning-switch",
    "final-learning-switch --breadth-with-conjs --threads 24 --minimal-models --by-size --non-accumulative"))

benches.append(PaperBench(33, "final-decentralized-lock",
    "final-decentralized-lock --breadth-with-conjs --threads 24 --minimal-models --by-size --non-accumulative"))

benches.append(PaperBench(34, "final-chord",
    "final-chord --breadth-with-conjs --threads 24 --minimal-models --by-size --non-accumulative"))

benches.append(PaperBench(35, "final-paxos",
    "final-paxos --breadth-with-conjs --threads 24 --minimal-models --by-size --non-accumulative"))

benches.append(PaperBench(36, "final-multi-paxos",
    "final-multi-paxos --breadth-with-conjs --threads 24 --minimal-models --by-size --non-accumulative"))

benches.append(PaperBench(37, "final-flexible-paxos",
    "final-flexible-paxos --breadth-with-conjs --threads 24 --minimal-models --by-size --non-accumulative"))

for i in range(THREADS, 0, -1):
  benches.append(PaperBench(1,
      "nonacc_wc_bt_paxos_breadth_t" + str(i),
      "better-template-paxos-breadth4 --breadth-with-conjs --by-size --non-accumulative --minimal-models --threads " + str(i)))

for i in range(THREADS, 0, -1):
  benches.append(PaperBench(2,
      "paxos_wc_bt_breadth_t" + str(i),
      "better-template-paxos-breadth4 --breadth-with-conjs --minimal-models --threads " + str(i)))

for i in range(THREADS, 0, -1):
  benches.append(PaperBench(3,
      "wc_bt_paxos_depth2_finisher_t" + str(i),
      "better-template-paxos-finisher --breadth-with-conjs --minimal-models --whole-space --threads " + str(i)))

for i in range(THREADS, 0, -1):
  benches.append(PaperBench(25,
      "wc_chord_t" + str(i),
      "chord-gimme-1 --breadth-with-conjs --minimal-models --whole-space --threads " + str(i)))

for i in range(THREADS, 0, -1):
  benches.append(PaperBench(26,
      "wc_bt_multi_paxos_t" + str(i),
      "better-template-multi-paxos --breadth-with-conjs --minimal-models --whole-space --threads " + str(i)))

for i in range(THREADS, 0, -1):
  benches.append(PaperBench(27,
      "wc_bt_flexible_paxos_t" + str(i),
      "better-template-flexible-paxos --breadth-with-conjs --minimal-models --whole-space --threads " + str(i)))


for i in range(THREADS, 0, -1):
  benches.append(PaperBench(4,
      "nonacc_wc_chord_t" + str(i),
      "chord-gimme-1 --breadth-with-conjs --by-size --non-accumulative --minimal-models --whole-space --threads " + str(i)))

for i in range(THREADS, 0, -1):
  benches.append(PaperBench(5,
      "nonacc_wc_bt_multi_paxos_t" + str(i),
      "better-template-multi-paxos --breadth-with-conjs --by-size --non-accumulative --minimal-models --whole-space --threads " + str(i)))

for i in range(THREADS, 0, -1):
  benches.append(PaperBench(6,
      "nonacc_wc_bt_flexible_paxos_t" + str(i),
      "better-template-flexible-paxos --breadth-with-conjs --by-size --non-accumulative --minimal-models --whole-space --threads " + str(i)))

benches.append(PaperBench(7, "breadth_wc_bt_paxos_epr_minus_easy_existential",
    "better_template_breadth_paxos_epr_minus_easy_existential --breadth-with-conjs --minimal-models --threads 1"))

benches.append(PaperBench(7, "finisher_wc_bt_paxos_epr_minus_easy_existential",
    "better_template_finisher_paxos_epr_minus_easy_existential --breadth-with-conjs --minimal-models --threads 1 --whole-space"))


for minmodels in (True, False):
#for minmodels in (True,):
  for postbmc in [False]: #(False, True):
    for prebmc in (False, True):
      for nonacc in (True, False):
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
        if nonacc:
          name += "nonacc_"
          args += " --by-size --non-accumulative"

        c = 8 + (
            + (1 if minmodels else 0)
            + (2 if prebmc else 0)
            + (4 if nonacc else 0)
        )

        benches.append(PaperBench(c, name+"wc_sdl", "sdl --breadth-with-conjs --threads "+str(THREADS)+args))
        benches.append(PaperBench(c, name+"wc_leader_election_fin", "leader-election-depth2 --breadth-with-conjs --threads "+str(THREADS)+args))
        benches.append(PaperBench(c, name+"wc_leader_election_breadth", "breadth-leader-election --breadth-with-conjs --threads "+str(THREADS)+args))
        benches.append(PaperBench(c, name+"wc_learning_switch", "breadth-learning-switch --breadth-with-conjs --threads "+str(THREADS)+args))
        benches.append(PaperBench(c, name+"wc_lock_server", "breadth-lock-server --breadth-with-conjs --threads "+str(THREADS)+args))
        benches.append(PaperBench(c, name+"wc_2pc", "breadth-2pc --breadth-with-conjs --threads "+str(THREADS)+args))
        benches.append(PaperBench(c, name+"wc_bt_paxos", "better-template-paxos --breadth-with-conjs --threads "+str(THREADS)+args))
        benches.append(PaperBench(c, name+"wc_bt_flexible_paxos", "better-template-flexible-paxos --breadth-with-conjs --threads "+str(THREADS)+args))
        benches.append(PaperBench(c, name+"wc_bt_multi_paxos", "better-template-multi-paxos --breadth-with-conjs --threads "+str(THREADS)+args))
        benches.append(PaperBench(c, name+"wc_chord_gimme1", "chord-gimme-1 --breadth-with-conjs --threads "+str(THREADS)+args))
        benches.append(PaperBench(c, name+"wc_delock_gimme1", "decentralized-lock-gimme-1 --breadth-with-conjs --threads "+str(THREADS)+args))

        if minmodels and not prebmc and not postbmc and nonacc:
          benches.append(PaperBench(16, name+"wc_sdl_one_thread", "sdl --whole-space --breadth-with-conjs --threads "+str(1)+args))
          benches.append(PaperBench(16, name+"wc_leader_election_fin_one_thread", "leader-election-depth2 --whole-space --breadth-with-conjs --threads "+str(1)+args))
          benches.append(PaperBench(16, name+"wc_leader_election_breadth_one_thread", "breadth-leader-election --whole-space --breadth-with-conjs --threads "+str(1)+args))
          benches.append(PaperBench(16, name+"wc_learning_switch_one_thread", "breadth-learning-switch --whole-space --breadth-with-conjs --threads "+str(11)+args))
          benches.append(PaperBench(16, name+"wc_lock_server_one_thread", "breadth-lock-server --whole-space --breadth-with-conjs --threads "+str(1)+args))
          benches.append(PaperBench(16, name+"wc_2pc_one_thread", "breadth-2pc --whole-space --breadth-with-conjs --threads "+str(1)+args))
          benches.append(PaperBench(16, name+"wc_bt_paxos_one_thread", "better-template-paxos --whole-space --breadth-with-conjs --threads "+str(1)+args))
          benches.append(PaperBench(16, name+"wc_bt_flexible_paxos_one_thread", "better-template-flexible-paxos --whole-space --breadth-with-conjs --threads "+str(1)+args))
          benches.append(PaperBench(16, name+"wc_bt_multi_paxos_one_thread", "better-template-multi-paxos --whole-space --breadth-with-conjs --threads "+str(1)+args))

          benches.append(PaperBench(16, name+"wc_chord_gimme1_one_thread", "chord-gimme-1 --breadth-with-conjs --threads "+str(1)+args))
          benches.append(PaperBench(16, name+"wc_delock_gimme1_one_thread", "decentralized-lock-gimme-1 --breadth-with-conjs --threads "+str(1)+args))

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
