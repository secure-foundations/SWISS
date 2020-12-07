import sys
import os

import graphs

if __name__ == '__main__':
  input_directory = sys.argv[1]
  median_of = int(sys.argv[2])
  graphs.misc_stats(input_directory, median_of)
