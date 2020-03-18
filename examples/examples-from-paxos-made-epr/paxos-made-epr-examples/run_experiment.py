"""
Run like this:

bash: time python run_experiment.py  1>experiment.out 2>experiment.err &
tcsh: nohup python run_experiment.py  >& experiment.out &
"""
import subprocess
import time
from numpy import std, mean
from os.path import isfile, expanduser
import sys

def flush():
    sys.stdout.flush()
    sys.stderr.flush()

def time_command(cmd, n=10):
    print "\n=== running {} times of {}\n".format(n, cmd)
    times = []
    for i in range(n):
        print "=== starting # {:3} of {}\n".format(i + 1, cmd)
        flush()
        t = time.time()
        subprocess.call(cmd)
        t = time.time() - t
        flush()
        print "\n=== finished # {:3}, took {} seconds\n".format(i + 1, t)
        times.append(t)
    print "=== timing results for {} times of {}:\n=== mean = {}\n=== std = {}\n=== times = {}\n\n".format(n, cmd, mean(times), std(times), times)
    return times

results = {}
for ext in ['_epr.ivy', '_aux_inv.ivy', '_epr_rewrite_verify.ivy'] + ['_fol_bmc_{}.ivy'.format(n) for n in [2,4,8,16]]:
    for name in ['paxos', 'multi_paxos', 'vertical_paxos', 'fast_paxos', 'flexible_paxos', 'stoppable_paxos']:
        fn = '{}/{}{}'.format(name,name,ext)
        if not isfile(fn):
            continue
        if name not in results:
            results[name] = {}
        results[name][ext] = time_command([
            'timeout',
            '300',
            expanduser('~/.local/bin/ivy_check'),
            expanduser('~/repos/oopsla17-epr/examples/' + fn),
        ], 10)
        open('results.dat', 'w').write(repr(results))

print "\n\nALL DONE\n{}".format(results)
