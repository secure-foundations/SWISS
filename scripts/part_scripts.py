import sys

paperlog_bucket = sys.argv[1]
nparts = int(sys.argv[2])

print("paperlogs/" + paperlog_bucket)
print("num parts: " + str(nparts))

queue_script = """#!/bin/bash

"""

for partition in range(1, nparts+1):
  queue_script += "sbatch part" + str(partition) + "\n"

  s = """#!/bin/bash
#SBATCH -N 1
#SBATCH -p RM
#SBATCH -t 72:00:00 # set duration of the job
#SBATCH -o paper_benchmarks.o%j # set output name of log file

#echo commands to stdout
set -x

make
python3 scripts/paper_benchmarks.py paperlogs/""" + str(paperlog_bucket) + """ -j 24 -p """ + str(partition) + """
"""
  with open("part" + str(partition), "w") as f:
    f.write(s)

with open("queue.sh", "w") as f:
  f.write(queue_script)
