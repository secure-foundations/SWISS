import sys
import os

import graphs

if __name__ == '__main__':
  which = int(sys.argv[1])
  input_directory = sys.argv[2]
  if which == 0:
    graphs.make_comparison_table(input_directory, True) # ivy
  elif which == 1:
    graphs.make_comparison_table(input_directory, False)
  else:
    assert False
