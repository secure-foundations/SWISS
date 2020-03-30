import sys
from datetime import datetime
import random
import subprocess
import tempfile
import queue
import threading
import traceback
import json
import time

def run_synthesis(logfile_base, run_id, jsonfile, args, q=None):
  try:
    logfilename = logfile_base + "." + run_id

    cmd = ["./synthesis"] + args
    print("run " + run_id + ": " + " ".join(cmd) + " > " + logfilename)
    sys.stdout.flush()
    with open(logfilename, "w") as logfile:
      t1 = time.time()

      proc = subprocess.Popen(cmd,
          stdin=subprocess.PIPE,
          stdout=logfile,
          stderr=logfile)
      out, err = proc.communicate(jsonfile)
      ret = proc.wait()

      t2 = time.time()

      seconds = str(int(t2 - t1))

      if ret != 0:
        print("run " + run_id + " failed (" + seconds + " seconds)")
        sys.exit(1)
      else:
        print("complete " + run_id + " (" + seconds + " seconds)")
        sys.stdout.flush()
    if q != None:
      q.put(run_id)
  except Exception:
    traceback.print_exc()
    sys.stderr.flush()

def unpack_args(args):
  main_args = []
  iter_arg_lists = []
  li = None
  for arg in args:
    assert arg != "--incremental"
    if arg in ("--finisher", "--breadth"):
      if li != None:
        iter_arg_lists.append(li)
      li = []
      li.append(arg)
    else:
      if li == None:
        main_args.append(arg)
      else:
        li.append(arg)
  assert li != None
  iter_arg_lists.append(li)
  return (main_args, iter_arg_lists)

def do_threading(logfile, nthreads, jsonfile, args):
  main_args, iter_arg_lists = unpack_args(args)
  invfile = None
  i = 0
  for iter_arg_list in iter_arg_lists:
    i += 1
    if iter_arg_list[0] == "--finisher":
      success = do_finisher("finisher."+str(i), logfile, nthreads, jsonfile, main_args + iter_arg_list, invfile)
    elif iter_arg_list[0] == "--breadth":
      success, invfile = do_breadth("iteration."+str(i), logfile, nthreads, jsonfile, main_args + iter_arg_list, invfile)
    else:
      assert False
    if success:
      print("Invariant success!")
      break

def parse_output_file(filename):
  with open(filename) as f:
    src = f.read()
    j = json.loads(src)
    return (j["success"], len(j["formulas"]) > 0)

def do_breadth(iterkey, logfile, nthreads, jsonfile, args, invfile):
  i = 0
  while True:
    i += 1
    success, hasany, invfile = do_breadth_single(iterkey+"."+str(i),
          logfile, nthreads, jsonfile, args, invfile)
    if success:
      return True, invfile
    if not hasany:
      return False, invfile

def do_breadth_single(iterkey, logfile, nthreads, jsonfile, args, invfile):
  chunk_files = []
  chunk_file_args = []
  for i in range(nthreads):
    chunk = tempfile.mktemp()
    chunk_files.append(chunk)
    chunk_file_args.append("--output-chunk-file")
    chunk_file_args.append(chunk)

  run_synthesis(logfile, iterkey+".chunkify", jsonfile, chunk_file_args + args)

  q = queue.Queue()
  threads = [ ]
  output_files = {}
  for i in range(nthreads):
    output_file = tempfile.mktemp()
    key = iterkey+".thread."+str(i)
    output_files[key] = output_file

    if invfile != None:
      args_with_file = ["--input-formula-file", invfile] + args
    else:
      args_with_file = args

    t = threading.Thread(target=run_synthesis, args=
        (logfile, key, jsonfile,
          ["--input-chunk-file", chunk_files[i],
           "--output-formula-file", output_file] + args_with_file, q))
    t.start()
    threads.append(t)

  has_any = False
  for i in range(nthreads):
    key = q.get()
    success, this_has_any = parse_output_file(output_files[key])
    if this_has_any:
      has_any = True
    if success:
      return (True, has_any, None)

  new_output_file = tempfile.mktemp()
  coalesce_file_args = ["--coalesce", "--output-formula-file", new_output_file]
  if invfile != None:
    coalesce_file_args.append("--input-formula-file")
    coalesce_file_args.append(invfile)
  for key in output_files:
    coalesce_file_args.append("--input-formula-file")
    coalesce_file_args.append(output_files[key])
  run_synthesis(logfile, iterkey+".coalesce", jsonfile, coalesce_file_args)

  return (False, has_any, new_output_file)

def do_finisher(iterkey, logfile, nthreads, jsonfile, args, invfile):
  chunk_files = []
  chunk_file_args = []
  for i in range(nthreads):
    chunk = tempfile.mktemp()
    chunk_files.append(chunk)
    chunk_file_args.append("--output-chunk-file")
    chunk_file_args.append(chunk)

  run_synthesis(logfile, iterkey+".chunkify", jsonfile, chunk_file_args + args)

  q = queue.Queue()
  threads = [ ]
  output_files = {}
  for i in range(nthreads):
    output_file = tempfile.mktemp()
    key = iterkey+".thread."+str(i)
    output_files[key] = output_file

    if invfile != None:
      args_with_file = ["--input-formula-file", invfile] + args
    else:
      args_with_file = args

    t = threading.Thread(target=run_synthesis, args=
        (logfile, key, jsonfile,
          ["--input-chunk-file", chunk_files[i],
           "--output-formula-file", output_file] + args_with_file, q))
    t.start()
    threads.append(t)

  for i in range(nthreads):
    key = q.get()
    if parse_output_file(output_files[key]):
      return True

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
