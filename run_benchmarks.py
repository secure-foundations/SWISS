#!/usr/bin/env python3

import subprocess
import time
import sys
import os
import signal
from benchmarks import BENCHMARKS

HOUR = 60*60
TIMEOUT = 4 * HOUR

def run_benchmark(name, args):
  proc = subprocess.Popen(["./bench.sh", name] + args,
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

def benchmark_print_1(name, args = []):
  cmd = " ".join([name] + args)
  return pad_right(cmd + " ...", 50)

def benchmark_print_2(name, res, logfile):
  return " " + pad_left(res, 16) + "   [" + logfile + "]"

def benchmark_print_line(name, res, logfile):
  return benchmark_print_1(name) + benchmark_print_2(name, res, logfile)

def do_benchmark(name, args):
  logfile, res = run_benchmark(name, args)
  print(benchmark_print_1(name, args) +
        benchmark_print_2(name, res, logfile))

if __name__ == '__main__':
  #benches = sys.argv[1:]
  #if len(benches) == 0:
  #  for bench in BENCHMARKS:
  #    do_benchmark(bench)
  #else:
  #  for bench in benches:
  #    do_benchmark(bench)
  bench = sys.argv[1]
  args = sys.argv[2:]
  do_benchmark(bench, args)
