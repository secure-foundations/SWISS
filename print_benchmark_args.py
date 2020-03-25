from __future__ import print_function
from benchmarks import BENCHMARKS
import sys

def print_args(benchmark):
  print(benchmark.ivy_file, " ".join(benchmark.args))

if __name__ == '__main__':
  print_args(BENCHMARKS[sys.argv[1]])
