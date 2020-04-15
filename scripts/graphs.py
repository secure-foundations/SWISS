import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path
import sys
import os

def make_parallel_graph(ax, input_directory, name):
  ax.set_title("parallel " + name)
  make_segmented_graph(ax, input_directory, name, "_t", True)

def make_seed_graph(ax, input_directory, name):
  ax.set_title("seed " + name)
  make_segmented_graph(ax, input_directory, name, "_seed_", True)

red = '#ff4040'
lightred = '#ff8080'

blue = '#4040ff'
lightblue = '#8080ff'

def slightly_darken(color):
  if color == red:
    return '#b02020'
  if color == lightred:
    return '#b06060'
  if color == blue:
    return '#6060b0'
  if color == lightblue:
    return '#6060b0'
  assert False

def make_segmented_graph(ax, input_directory, name, suffix, include_threads=False):
  for filename in os.listdir(input_directory):
    if filename.startswith(name + suffix):
      numthreads = int(filename[len(name + suffix) : ])
      times = []
      thread_times = []
      colors = []

      odd1 = True
      odd2 = True
      with open(os.path.join(input_directory, filename)) as f:
        for line in f:
          if line.startswith("BREADTH iteration "):
            times.append(int(line.split()[4]))
            thread_times.append([int(x) for x in line.split()[12:]])
            colors.append(red if odd1 else lightred)
            odd1 = not odd1
          elif line.startswith("FINISHER time: "):
            times.append(int(line.split()[2]))
            thread_times.append([int(x) for x in line.split()[10:]])
            colors.append(blue if odd2 else lightblue)
            odd2 = not odd2

      bottom = 0
      for i in range(len(times)):
        t = times[i]
        color = 'red' if i % 2 == 0 else 'black'
        ax.bar(numthreads, t, bottom=bottom, color=colors[i])

        if include_threads:
          for j in range(len(thread_times[i])):
            thread_time = thread_times[i][j]
            m = len(thread_times[i])
            ax.bar(numthreads - 0.4 + 0.8 * (j / float(m)) + 0.4 / float(m),
                thread_time, bottom=bottom, width = 0.8 / float(m),
                color=slightly_darken(colors[i]))

        bottom += t




def main():
  directory = sys.argv[1]
  input_directory = os.path.join("paperlogs", directory)
  output_directory = os.path.join("graphs", directory)

  Path(output_directory).mkdir(parents=True, exist_ok=True)

  fig, ax = plt.subplots(nrows=3, ncols=6, figsize=[6, 8])

  make_parallel_graph(ax.flat[0], input_directory, "paxos_breadth")
  make_parallel_graph(ax.flat[1], input_directory, "paxos_implshape_finisher")
  make_seed_graph(ax.flat[2], input_directory, "learning_switch")
  make_seed_graph(ax.flat[3], input_directory, "paxos")

  #plt.savefig(os.path.join(output_directory, 'graphs.png'))
  plt.show()

if __name__ == '__main__':
  main()
