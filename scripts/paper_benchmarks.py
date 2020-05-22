import sys
import os
import subprocess
import shutil
import time
from pathlib import Path
import queue
import threading

NUM_PARTS = 7

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

THREADS = 7

for i in range(THREADS, 0, -1):
  benches.append(PaperBench(1,
      "nonacc_paxos_breadth_t" + str(i),
      "breadth-paxos-4-r3 --by-size --non-accumulative --minimal-models --threads " + str(i)))

for i in range(THREADS, 0, -1):
  benches.append(PaperBench(1,
      "paxos_breadth_t" + str(i),
      "breadth-paxos-4-r3 --minimal-models --threads " + str(i)))

for i in range(THREADS, 0, -1):
  benches.append(PaperBench(2,
      "nonacc_wc_bt_paxos_breadth_t" + str(i),
      "better-template-paxos-breadth4 --breadth-with-conjs --by-size --non-accumulative --minimal-models --threads " + str(i)))

for i in range(THREADS, 0, -1):
  benches.append(PaperBench(2,
      "paxos_wc_bt_breadth_t" + str(i),
      "better-template-paxos-breadth4 --breadth-with-conjs --minimal-models --threads " + str(i)))


for seed in range(1, 8):
  benches.append(PaperBench(1,
      "nonacc_learning_switch_seed_" + str(seed),
      "breadth-learning-switch --by-size --non-accumulative --minimal-models --threads " + str(THREADS) + " --seed " + str(seed)))

for seed in range(1, 9):
  benches.append(PaperBench(1,
      "learning_switch_seed_" + str(seed),
      "breadth-learning-switch --minimal-models --threads " + str(THREADS) + " --seed " + str(seed)))

for seed in range(1, 8):
  benches.append(PaperBench(2,
      "nonacc_wc_learning_switch_seed_" + str(seed),
      "breadth-learning-switch --breadth-with-conjs --by-size --non-accumulative --minimal-models --threads " + str(THREADS) + " --seed " + str(seed)))

for seed in range(1, 9):
  benches.append(PaperBench(2,
      "wc_learning_switch_seed_" + str(seed),
      "breadth-learning-switch --breadth-with-conjs --minimal-models --threads " + str(THREADS) + " --seed " + str(seed)))

for seed in range(1, 5):
  benches.append(PaperBench(1,
      "nonacc_paxos_seed_" + str(seed),
      "full-paxos-depth2 --by-size --non-accumulative --minimal-models --threads " + str(THREADS) + " --seed " + str(seed)))

for seed in range(1, 9):
  benches.append(PaperBench(1,
      "paxos_seed_" + str(seed),
      "full-paxos-depth2 --minimal-models --threads " + str(THREADS) + " --seed " + str(seed)))

for seed in range(1, 9):
  benches.append(PaperBench(1,
      "wholespace_finisher_paxos_seed_" + str(seed),
      "finisher-paxos-exist-1-depth2 --whole-space --minimal-models --threads " + str(THREADS) + " --seed " + str(seed)))

for seed in range(1, 5):
  benches.append(PaperBench(2,
      "nonacc_wc_bt_paxos_seed_" + str(seed),
      "better-template-paxos --breadth-with-conjs --by-size --non-accumulative --minimal-models --threads " + str(THREADS) + " --seed " + str(seed)))

for seed in range(1, 9):
  benches.append(PaperBench(2,
      "wc_bt_paxos_seed_" + str(seed),
      "better-template-paxos --breadth-with-conjs --minimal-models --threads " + str(THREADS) + " --seed " + str(seed)))

for seed in range(1, 9):
  benches.append(PaperBench(2,
      "wholespace_finisher_bt_paxos_seed_" + str(seed),
      "better-template-paxos-finisher --whole-space --minimal-models --threads " + str(THREADS) + " --seed " + str(seed)))

for seed in range(1, 5):
  benches.append(PaperBench(1,
      "nonacc_paxos_breadth_seed_" + str(seed),
      "breadth-paxos-4-r3 --non-accumulative --minimal-models --threads 1 --seed " + str(200 + seed)))
for seed in range(1, 8):
  benches.append(PaperBench(1,
      "paxos_breadth_seed_" + str(seed),
      "breadth-paxos-4-r3 --minimal-models --threads 1 --seed " + str(300 + seed)))

for seed in range(1, 5):
  benches.append(PaperBench(2,
      "nonacc_wc_bt_paxos_breadth_seed_" + str(seed),
      "better-template-paxos-breadth4 --breadth-with-conjs --non-accumulative --minimal-models --threads 1 --seed " + str(200 + seed)))
for seed in range(1, 8):
  benches.append(PaperBench(2,
      "wc_bt_paxos_breadth_seed_" + str(seed),
      "better-template-paxos-breadth4 --breadth-with-conjs --minimal-models --threads 1 --seed " + str(300 + seed)))



#for i in range(20, 0, -1):
#  benches.append(PaperBench(
#      "paxos_finisher_t" + str(i),
#      "finisher-paxos-exist-1-depth2 --minimal-models --whole-space --threads " + str(i)))

#for i in range(THREADS, 0, -1):
#  benches.append(PaperBench(
#      "paxos_implshape_finisher_t" + str(i),
#      "finisher-paxos-exist-1 --minimal-models --whole-space --threads " + str(i)))

for i in range(THREADS, 0, -1):
  benches.append(PaperBench(1,
      "paxos_depth2_finisher_t" + str(i),
      "finisher-paxos-exist-1-depth2 --minimal-models --whole-space --threads " + str(i)))

for i in range(THREADS, 0, -1):
  benches.append(PaperBench(2,
      "wc_bt_paxos_depth2_finisher_t" + str(i),
      "better-template-paxos-finisher --breadth-with-conjs --minimal-models --whole-space --threads " + str(i)))

benches.append(PaperBench(1, "breadth_paxos_epr_minus_easy_existential",
    "breadth_paxos_epr_minus_easy_existential --minimal-models --threads 1"))

benches.append(PaperBench(1, "finisher_paxos_epr_minus_easy_existential",
    "finisher_paxos_epr_minus_easy_existential --minimal-models --threads 1 --whole-space"))

benches.append(PaperBench(2, "breadth_wc_bt_paxos_epr_minus_easy_existential",
    "better_template_breadth_paxos_epr_minus_easy_existential --breadth-with-conjs --minimal-models --threads 1"))

benches.append(PaperBench(2, "finisher_wc_bt_paxos_epr_minus_easy_existential",
    "better_template_finisher_paxos_epr_minus_easy_existential --breadth-with-conjs --minimal-models --threads 1 --whole-space"))


for minmodels in (True, False):
#for minmodels in (True,):
  for postbmc in [False]: #(False, True):
    for prebmc in (False, True):
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

      c = 3 if minmodels else 4

      benches.append(PaperBench(c, name+"sdl", "sdl --threads "+str(THREADS)+args))
      benches.append(PaperBench(c, name+"leader_election_fin", "leader-election-depth2 --threads "+str(THREADS)+args))
      benches.append(PaperBench(c, name+"leader_election_breadth", "breadth-leader-election --threads "+str(THREADS)+args))
      benches.append(PaperBench(c, name+"learning_switch", "breadth-learning-switch --threads "+str(THREADS)+args))
      benches.append(PaperBench(c, name+"paxos", "full-paxos-depth2 --threads "+str(THREADS)+args))
      benches.append(PaperBench(c, name+"flexible_paxos", "full-flexible-paxos-depth2 --threads "+str(THREADS)+args))
      benches.append(PaperBench(c, name+"lock_server", "lock-server --threads "+str(THREADS)+args))
      benches.append(PaperBench(c, name+"2pc", "breadth-2pc --threads "+str(THREADS)+args))
      benches.append(PaperBench(c, name+"multi_paxos", "full-multi-paxos-depth2 --threads "+str(THREADS)+args))

      c = 5 if minmodels else 6

      benches.append(PaperBench(c, name+"wc_sdl", "sdl --breadth-with-conjs --threads "+str(THREADS)+args))
      benches.append(PaperBench(c, name+"wc_leader_election_fin", "leader-election-depth2 --breadth-with-conjs --threads "+str(THREADS)+args))
      benches.append(PaperBench(c, name+"wc_leader_election_breadth", "breadth-leader-election --breadth-with-conjs --threads "+str(THREADS)+args))
      benches.append(PaperBench(c, name+"wc_learning_switch", "breadth-learning-switch --breadth-with-conjs --threads "+str(THREADS)+args))
      benches.append(PaperBench(c, name+"wc_lock_server", "breadth-lock-server --breadth-with-conjs --threads "+str(THREADS)+args))
      benches.append(PaperBench(c, name+"wc_2pc", "breadth-2pc --breadth-with-conjs --threads "+str(THREADS)+args))
      benches.append(PaperBench(c, name+"wc_bt_paxos", "better-template-paxos --breadth-with-conjs --threads "+str(THREADS)+args))
      benches.append(PaperBench(c, name+"wc_bt_flexible_paxos", "better-template-flexible-paxos --breadth-with-conjs --threads "+str(THREADS)+args))
      benches.append(PaperBench(c, name+"wc_bt_multi_paxos", "better-template-multi-paxos --breadth-with-conjs --threads "+str(THREADS)+args))

for i in range(3, 0, -1):
  benches.append(PaperBench(1,
      "chord_breadth_t" + str(i),
      "breadth-chord --minimal-models --threads " + str(i)))

for i in range(3, 0, -1):
  benches.append(PaperBench(2,
      "nonacc_chord_breadth_t" + str(i),
      "breadth-chord --by-size --non-accumulative --minimal-models --threads " + str(i)))

for i in range(3, 0, -1):
  benches.append(PaperBench(2,
      "wc_chord_breadth_t" + str(i),
      "breadth-chord --breadth-with-conjs --minimal-models --threads " + str(i)))

for i in range(3, 0, -1):
  benches.append(PaperBench(2,
      "nonacc_wc_chord_breadth_t" + str(i),
      "breadth-chord --breadth-with-conjs --by-size --non-accumulative --minimal-models --threads " + str(i)))


#benches.append(PaperBench("finisher_paxos_minus_size4", "finisher-paxos-minus-size4 --whole-space --threads 1"))
#benches.append(PaperBench("breadth_paxos_minus_size4", "breadth-paxos-minus-size4 --threads 1"))

#benches.append(PaperBench("fail_chord", "fail-chord --minimal-models --threads 7"))
benches.append(PaperBench(7, "fail_chain", "chain --minimal-models --threads 7"))
#benches.append(PaperBench("fail_delock", "fail-delock --minimal-models --threads 7"))

benches.append(PaperBench(1, "gimme1_chord", "chord-gimme-1 --minimal-models --threads 7"))
benches.append(PaperBench(1, "gimme1_delock", "decentralized-lock-gimme-1 --minimal-models --threads 7"))

benches.append(PaperBench(2, "gimme1_wc_chord", "chord-gimme-1 --breadth-with-conjs --minimal-models --threads 7"))
benches.append(PaperBench(2, "gimme1_wc_delock", "decentralized-lock-gimme-1 --breadth-with-conjs --minimal-models --threads 7"))

#benches.append(PaperBench("fail_vertical", "fail-full-vertical--paxos-depth2 --minimal-models --threads 7"))
#benches.append(PaperBench("fail_stoppable", "fail-full-stoppable-paxos-depth2 --minimal-models --threads 7"))

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
  i = 0
  while i < len(args):
    if args[i] == '-j':
      j = int(args[i+1])
      i += 1
    elif args[i] == '-p':
      p = int(args[i+1])
      i += 1
    else:
      res.append(args[i])
    i += 1
  return j, p, res

def main():
  args = sys.argv[1:]
  j, p, args = parse_args(args)

  assert len(args) == 1
  directory = args[0]
  Path(directory).mkdir(parents=True, exist_ok=True)

  assert directory.startswith("paperlogs/")

  if j == None:
    for b in benches:
      if p == None or p == b.partition:
        run(directory, b)
  else:
    if p == None:
      awesome_async_run(directory, benches, j)
    else:
      awesome_async_run(directory, [b for b in benches if b.partition == p], j)

  print('done')

if __name__ == "__main__":
  main()
