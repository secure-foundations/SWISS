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
from stats import Stats
import random

all_procs = {}
killing = False

random.seed(1234)

def set_seed(seed):
  random.seed(seed)

def args_add_seed(args):
  return ["--seed", str(random.randint(1, 10**6))] + args

def kill_all_procs():
  global killing
  if not killing:
    killing = True
    keys = all_procs.keys()
    for k in keys:
      all_procs[k].kill()

class RunSynthesisResult(object):
  def __init__(self, run_id, seconds, stopped, failed, logfile):
    self.run_id = run_id
    self.seconds = seconds
    self.stopped = stopped
    self.failed = failed
    self.logfile = logfile

def run_synthesis(logfile_base, run_id, jsonfile, args, q=None, use_stdout=False):
  try:
    logfilename = logfile_base + "." + run_id

    cmd = ["./synthesis"] + args
    print("run " + run_id + ": " + " ".join(cmd) + " > " + logfilename)
    sys.stdout.flush()
    with open(logfilename, "w") as logfile:
      t1 = time.time()

      if use_stdout:
        proc = subprocess.Popen(cmd,
            stdin=subprocess.PIPE)
      else:
        proc = subprocess.Popen(cmd,
            stdin=subprocess.PIPE,
            stdout=logfile,
            stderr=logfile)

      all_procs[run_id] = proc
      if killing:
        print("run " + run_id + " stopped")
        proc.kill()
        return

      out, err = proc.communicate(jsonfile)
      ret = proc.wait()

      t2 = time.time()

      seconds = int(t2 - t1)

      if killing:
        print("stopped " + run_id + " (" + str(seconds) + " seconds)")
        sys.stdout.flush()
      else:
        if ret != 0:
          print("failed " + run_id + " (" + str(seconds) + " seconds)")
          sys.stdout.flush()
        else:
          print("complete " + run_id + " (" + str(seconds) + " seconds)")
          sys.stdout.flush()
    if q != None:
      q.put(RunSynthesisResult(run_id, seconds, killing, ret != 0, logfilename))
    
    return ret == 0
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

  t = []
  for li in iter_arg_lists:
    if len(t) > 0 and t[-1][0] == li[0]:
      t[-1] = t[-1] + li
    else:
      t.append(li)

  return (main_args, t)

def do_threading(ivy_filename, json_filename, logfile, nthreads, jsonfile, args):
  print(ivy_filename)
  print("json: ", json_filename)

  stats = Stats(nthreads, args, ivy_filename, json_filename, logfile)

  main_args, iter_arg_lists = unpack_args(args)
  invfile = None
  i = 0
  for iter_arg_list in iter_arg_lists:
    i += 1
    if iter_arg_list[0] == "--finisher":
      success = do_finisher("finisher", logfile, nthreads, jsonfile, main_args + iter_arg_list, invfile, stats)
    elif iter_arg_list[0] == "--breadth":
      success, invfile = do_breadth("iteration", logfile, nthreads, jsonfile, main_args + iter_arg_list, invfile, stats)
    else:
      assert False
    if success:
      print("Invariant success!")
      break
  
  statfile = logfile + ".summary"
  stats.print_stats(statfile)
  print("")
  print("statfile: " + statfile)
  print("")
  print("======== Summary ========")
  print("")
  with open(statfile, "r") as f:
    print(f.read())

def parse_output_file(filename):
  with open(filename) as f:
    src = f.read()
    j = json.loads(src)
    return (j["success"], len(j["formulas"]) > 0)

def do_breadth(iterkey, logfile, nthreads, jsonfile, args, invfile, stats):
  i = 0
  while True:
    i += 1
    success, hasany, invfile = do_breadth_single(iterkey+"."+str(i),
          logfile, nthreads, jsonfile, args, invfile, i-1, stats)
    if success:
      return True, invfile
    if not hasany:
      return False, invfile

def do_breadth_single(iterkey, logfile, nthreads, jsonfile, args, invfile, iteration_num, stats):
  t1 = time.time()

  chunk_files = []
  chunk_file_args = []
  for i in range(nthreads):
    chunk = tempfile.mktemp()
    chunk_files.append(chunk)
    chunk_file_args.append("--output-chunk-file")
    chunk_file_args.append(chunk)

  succ = run_synthesis(logfile, iterkey+".chunkify", jsonfile, args_add_seed(chunk_file_args + args))
  assert succ, "breadth chunkify failed"

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
        (logfile, key, jsonfile, args_add_seed(
          ["--input-chunk-file", chunk_files[i],
           "--output-formula-file", output_file] + args_with_file), q))
    t.start()
    threads.append(t)

  has_any = False
  any_success = False
  for i in range(nthreads):
    synres = q.get()
    if synres.failed and not synres.stopped:
      kill_all_procs()
      assert False, "breadth proper failed"
    else:
      stats.add_inc_log(iteration_num, synres.logfile, synres.seconds)
      if not synres.stopped and not killing:
        key = synres.run_id
        success, this_has_any = parse_output_file(output_files[key])
        if this_has_any:
          has_any = True
        if success:
          stats.add_inc_result(iteration_num, output_files[key], int(time.time() - t1))
          kill_all_procs()
          any_success = True

  if any_success:
    return (True, has_any, None)
  
  new_output_file = tempfile.mktemp()
  coalesce_file_args = ["--coalesce", "--output-formula-file", new_output_file]
  if invfile != None:
    coalesce_file_args.append("--input-formula-file")
    coalesce_file_args.append(invfile)
  for key in output_files:
    coalesce_file_args.append("--input-formula-file")
    coalesce_file_args.append(output_files[key])
  succ = run_synthesis(logfile, iterkey+".coalesce", jsonfile, args_add_seed(coalesce_file_args))
  assert succ, "breadth coalesce failed"

  stats.add_inc_result(iteration_num, new_output_file, int(time.time() - t1))

  return (False, has_any, new_output_file)

def do_finisher(iterkey, logfile, nthreads, jsonfile, args, invfile, stats):
  t1 = time.time()

  chunk_files = []
  chunk_file_args = []
  for i in range(nthreads):
    chunk = tempfile.mktemp()
    chunk_files.append(chunk)
    chunk_file_args.append("--output-chunk-file")
    chunk_file_args.append(chunk)

  succ = run_synthesis(logfile, iterkey+".chunkify", jsonfile, args_add_seed(chunk_file_args + args))
  assert succ, "finisher chunkify failed"

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
        (logfile, key, jsonfile, args_add_seed(
          ["--input-chunk-file", chunk_files[i],
           "--output-formula-file", output_file] + args_with_file), q))
    t.start()
    threads.append(t)

  any_success = False
  for i in range(nthreads):
    synres = q.get()
    if synres.failed and not synres.stopped:
      kill_all_procs()
      assert False, "finisher proper failed"
    else:
      key = synres.run_id
      stats.add_finisher_log(synres.logfile, synres.seconds)
      if not synres.stopped and not killing:
        success, this_has_any = parse_output_file(output_files[key])
        if success:
          any_success = True
          stats.add_finisher_result(output_files[key], int(time.time() - t1))
          kill_all_procs()
  if not any_success:
    stats.add_finisher_result(output_files[iterkey+".thread.0"], int(time.time() - t1))

def parse_args(args):
  nthreads = None
  logfile = ""
  new_args = []
  use_stdout = False
  i = 0
  while i < len(args):
    if args[i] == "--threads":
      nthreads = int(args[i+1])
      i += 1
    elif args[i] == "--logfile":
      logfile = args[i+1]
      i += 1
    elif args[i] == "--seed":
      seed = int(args[i+1])
      set_seed(seed)
      i += 1
    elif args[i] == "--stdout":
      use_stdout = True
    else:
      new_args.append(args[i])
    i += 1
  if logfile == "":
    rstr = ""
    for i in range(9):
      rstr += str(random.randint(0, 9))
    logfile = ("logs/log." + datetime.now().strftime("%Y-%m-%d_%H.%M.%S")
        + "-" + rstr)
  return nthreads, logfile, new_args, use_stdout

def main():
  ivy_filename = sys.argv[1]
  json_filename = sys.argv[2]

  with open(json_filename, "rb") as f:
    jsonfile = f.read()

  args = sys.argv[3:]
  nthreads, logfile, args, use_stdout = parse_args(args)
  if nthreads == None:
    run_synthesis(logfile, "main", jsonfile, args_add_seed(args), use_stdout=use_stdout)
  else:
    do_threading(ivy_filename, json_filename, logfile, nthreads, jsonfile, args)

if __name__ == "__main__":
  main()
