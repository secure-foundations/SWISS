import sys
from datetime import datetime
import random
import subprocess
import tempfile
import queue
import threading
import traceback

def run_synthesis(logfile_base, run_id, jsonfile, args, q=None):
  try:
    logfilename = logfile_base + "." + run_id

    cmd = ["./synthesis"] + args
    print("run " + run_id + ": " + " ".join(cmd) + " > " + logfilename)
    sys.stdout.flush()
    with open(logfilename, "w") as logfile:
      proc = subprocess.Popen(cmd,
          stdin=subprocess.PIPE,
          stdout=logfile,
          stderr=logfile)
      out, err = proc.communicate(jsonfile)
      ret = proc.wait()
      if ret != 0:
        print("run " + run_id + " failed")
        sys.exit(1)
      else:
        print("run " + run_id + " complete")
        sys.stdout.flush()
    if q != None:
      q.put(run_id)
  except Exception:
    traceback.print_exc()
    sys.stderr.flush()

def do_threading(logfile, nthreads, jsonfile, args):
  chunk_files = []
  chunk_file_args = []
  for i in range(nthreads):
    chunk = tempfile.mktemp()
    chunk_files.append(chunk)
    chunk_file_args.append("--output-chunk-file")
    chunk_file_args.append(chunk)

  run_synthesis(logfile, "chunkify", jsonfile, chunk_file_args + args)

  q = queue.Queue()
  threads = [ ]
  for i in range(nthreads):
    t = threading.Thread(target=run_synthesis, args=
        (logfile, str(i), jsonfile, ["--input-chunk-file", chunk_files[i]] + args, q))
    t.start()
    threads.append(t)

  #for i in range(nthreads):
  #  q.get()

def parse_args(args):
  nthreads = None
  logfile = ""
  new_args = []
  i = 0
  while i < len(args):
    if args[i] == "--threads":
      nthreads = int(args[i+1])
      i += 1
    elif args[i] == "--logfile":
      logfile = args[i+1]
      i += 1
    else:
      new_args.append(args[i])
    i += 1
  if logfile == "":
    rstr = ""
    for i in range(9):
      rstr += str(random.randint(0, 9))
    logfile = ("logs/log." + datetime.now().strftime("%Y-%m-%d_%H.%M.%S")
        + "-" + rstr)
  return nthreads, logfile, new_args

def main():
  filename = sys.argv[1]

  with open(filename, "rb") as f:
    jsonfile = f.read()

  args = sys.argv[2:]
  nthreads, logfile, args = parse_args(args)
  if nthreads == None:
    run_synthesis(logfile, "main", jsonfile, args)
  else:
    do_threading(logfile, nthreads, jsonfile, args)

if __name__ == "__main__":
  main()
