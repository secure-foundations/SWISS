import sys
import os

import graphs

if __name__ == '__main__':
  which = int(sys.argv[1])
  directory = sys.argv[2]
  input_directory = os.path.join("paperlogs", directory)
  if which == 2:
    graphs.make_optimization_step_table(input_directory)
  else:
    graphs.make_table(input_directory, which)
