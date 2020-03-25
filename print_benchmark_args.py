from __future__ import print_function
from benchmarks import BENCHMARKS
import sys

def print_args(benchmark, extra):
  print(benchmark.ivy_file, " ".join(extra + list(benchmark.args)))

if __name__ == '__main__':
  print_args(BENCHMARKS[sys.argv[1]], sys.argv[2:])
