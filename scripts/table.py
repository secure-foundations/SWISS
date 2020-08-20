import sys
import os

import graphs

if __name__ == '__main__':
  which = int(sys.argv[1])
  directory = sys.argv[2]
  input_directory = os.path.join("paperlogs", directory)
  if which == 0:
    graphs.make_comparison_table(input_directory, True) # ivy
  elif which == 1:
    graphs.make_comparison_table(input_directory, False)
  else:
    assert False
