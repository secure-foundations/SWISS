import sys
import os

import graphs

if __name__ == '__main__':
  input_directory = sys.argv[1]
  which = int(sys.argv[2])
  graphs.make_comparison_table(input_directory, which)
