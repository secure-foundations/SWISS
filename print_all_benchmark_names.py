from __future__ import print_function
from benchmarks import BENCHMARKS
import sys

def print_names():
  t = []
  for bench in BENCHMARKS:
    t.append(bench)
  print(" ".join(t))
  
if __name__ == '__main__':
  print_names()
