import sys
import os

import graphs

if __name__ == '__main__':
  input_directory = sys.argv[1]
  graphs.make_comparison_table(input_directory)
