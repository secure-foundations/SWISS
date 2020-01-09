#!/usr/bin/env python3

import subprocess
import time
import sys
import os
import signal

HOUR = 60*60
TIMEOUT = 4 * HOUR

BENCHMARKS = [
  "naive-leader-election",
  "naive-inc-leader-election",
  "naive-breadth-leader-election",
  "naive-strat2-breadth-leader-election",
  "sat-leader-election",
  "sat-inc-leader-election",
  "sat-breadth-leader-election",

  "sat-inc-learning-switch",
  "sat-breadth-learning-switch",
  "naive-inc-learning-switch",
  "naive-breadth-learning-switch",
  "naive-strat2-breadth-learning-switch",

  "sat-breadth-chord",
  "naive-breadth-chord-size3",
  "naive-inc-chord-size3",
  "naive-strat2-breadth-chord",

  "naive-paxos-missing1",
  "sat-inc-paxos",
  #"naive-breadth-paxos-size3",
  #"naive-breadth-paxos-size4",
  "naive-strat2-breadth-paxos-4-r2",
  "naive-strat2-breadth-paxos-4-r3",
]

def run_benchmark(name):
  proc = subprocess.Popen(["./bench.sh", name],
      preexec_fn=os.setsid,
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE)

  timed_out = False
  ret = None
  try:
    t1 = time.time()
    out, err = proc.communicate(timeout = TIMEOUT)
    ret = proc.wait()
    t2 = time.time()
  except subprocess.TimeoutExpired:
    # Kills the subprocess and all its child process.
    os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
    # Get the partial output so far
    out, errs = proc.communicate()
    timed_out = True

  logfile = get_log_file(out)

  if timed_out:
    res = "(timed out)"
  elif ret != 0:
    res = "(exit code " + str(ret) + ")"
  else:
    time_secs = int(t2 - t1)
    res = str(time_secs) + " seconds"

  return (logfile, res)

def get_log_file(out):
  lines = out.split(b"\n")

  for l in lines:
    if l.startswith(b"logging to "):
      return l[len("logging to ") : ].decode("utf-8")

  return "???"

def pad_right(name, l):
  while len(name) < l:
    name = name + " "
  return name

def pad_left(name, l):
  while len(name) < l:
    name = " " + name
  return name

def do_benchmark(name):
  print(pad_right(name + " ...", 50), end="")
  sys.stdout.flush()

  logfile, res = run_benchmark(name)
  print(" " + pad_left(res, 16) + "   [" + logfile + "]")

for bench in BENCHMARKS:
  do_benchmark(bench)
