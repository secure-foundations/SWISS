import sys
import random
import subprocess
from benchmarks import BENCHMARKS

n = int(sys.argv[1])
args = sys.argv[2 : ]

total_ms = 0

for i in xrange(n):
  seed = random.randint(1, 10**8)
  my_args = args + ["--seed", str(seed)]

  proc = subprocess.Popen(["./save.sh"] + my_args,
      stdin=subprocess.PIPE,
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE)
  out, err = proc.communicate()
  ret = proc.wait()
  if ret != 0:
    print 'Error:'
    print err
  assert ret == 0

  log_line = ""
  solver_time_line = ""
  total_time_line = ""
  found_invariant = ""
  for line in out.split('\n'):
    if "total solver time :" in line:
      solver_time_line = line
    elif "real\t" in line:
      total_time_line = line
    elif "logged to" in line:
      log_line = line
    elif "found invariant" in line:
      found_invariant = True

  assert solver_time_line != ""
  assert found_invariant
  solver_ms = int(solver_time_line.split(':')[1].split()[0])
  total_ms += solver_ms
  
  print 'iteration', i+1, '/', n
  print log_line
  print solver_time_line
  print total_time_line
  print 'average so far:', (total_ms / 1000.0 / (i+1)), 'seconds'
  print ''

  sys.stdout.flush()

print 'average total solver time:', (total_ms / 1000.0 / n), 'seconds'
