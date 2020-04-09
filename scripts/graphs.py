import matplotlib.pyplot as plt
import numpy as np
import os

def make_breadth_parallel_graph(directory, name):
  fig, ax = plt.subplots()  # Create a figure containing a single axes.

  for filename in os.listdir(directory):
    if filename.startswith(name + "_t"):
      print(filename)
      numthreads = int(filename[len(name + "_t") : ])
      times = []
      with open(os.path.join(directory,filename)) as f:
        for line in f:
          if line.startswith("BREADTH iteration "):
            times.append(int(line.split()[4]))
          #if line.startswith("FINISHER time: "):
          #  times.append(int(line.split()[2]))

      bottom = 0
      for i in range(len(times)):
        t = times[i]
        color = 'red' if i % 2 == 0 else 'black'
        ax.bar(numthreads, bottom + t, bottom=bottom, color=color)
        bottom += t

  plt.savefig('test2.png')

make_breadth_parallel_graph("paperlogs/2", "paxos_breadth")
#make_breadth_parallel_graph("paperlogs/2", "paxos_implshape_finisher")
