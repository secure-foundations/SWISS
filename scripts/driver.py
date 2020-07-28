import sys
import os
from datetime import datetime
import random
import subprocess
import tempfile
import queue
import threading
import traceback
import json
import time
import random

import templates
import stats
from stats import Stats

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

def run_synthesis(logfile_base, run_id, json_filename, args, q=None, use_stdout=False):
  try:
    logfilename = logfile_base + "." + run_id

    cmd = ["./synthesis", "--input-module", json_filename] + args
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

      out, err = proc.communicate()
      ret = proc.wait()

      t2 = time.time()

      seconds = t2 - t1

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
  if li != None:
    iter_arg_lists.append(li)

  t = []
  for li in iter_arg_lists:
    if len(t) > 0 and t[-1][0] == li[0]:
      t[-1] = t[-1] + li
    else:
      t.append(li)

  return (main_args, t)

def do_threading(stats, ivy_filename, json_filename, logfile, nthreads, main_args, breadth_args, finisher_args, by_size):
  print(ivy_filename)
  print("json: ", json_filename)

  invfile = None
  success = False

  if not success and len(breadth_args) > 0:
    success, invfile = do_breadth("iteration", logfile, nthreads, json_filename, main_args, main_args + breadth_args, invfile, stats, by_size)

    if success:
      print("Invariant success!")
  
  if not success and len(finisher_args) > 0:
    success = do_finisher("finisher", logfile, nthreads, json_filename, main_args, main_args + finisher_args, invfile, stats)

    if success:
      print("Invariant success!")

  statfile = logfile + ".summary"
  stats.print_stats(statfile)
  print("")
  print("statfile: " + statfile)
  print("")
  print("======== Summary ========")
  print("")
  with open(statfile, "r") as f:
    t = f.read()
    t = t.split("specifics:")
    print(t[0])

def parse_output_file(filename):
  with open(filename) as f:
    src = f.read()
    j = json.loads(src)
    return (j["success"], len(j["new_invs"]) > 0)

def do_breadth(iterkey, logfile, nthreads, json_filename, main_args, args, invfile, stats, by_size):
  i = 0
  while True:
    i += 1
    success, hasany, invfile = do_breadth_single(iterkey+"."+str(i),
          logfile, nthreads, json_filename, main_args, args, invfile, i-1, stats, by_size)
    if success:
      return True, invfile
    if not hasany:
      return False, invfile

def do_breadth_single(iterkey, logfile, nthreads, json_filename, main_args, args, invfile, iteration_num, stats, by_size):
  t1 = time.time()

  chunk_files = chunkify(iterkey, logfile, nthreads, json_filename, args)
  if by_size:
    c_by_size = get_chunk_files_for_each_size(chunk_files)
    total_has_any = False
    for (sz, chunks) in c_by_size:
      success, has_any, output_invfile = breadth_run_in_parallel(
        iterkey+".size."+str(sz), logfile, json_filename, main_args,
          ["--chunk-size-to-use", str(sz)], invfile, iteration_num, stats, nthreads, chunks)
      if has_any:
        total_has_any = True
      if success:
        stats.add_inc_result(iteration_num, output_invfile, time.time() - t1)
        return success, total_has_any, output_invfile
      invfile = output_invfile
    success = False
    has_any = total_has_any
    new_output_file = invfile
  else:
    success, has_any, new_output_file = breadth_run_in_parallel(iterkey, logfile, json_filename, main_args, invfile,
        iteration_num, stats, nthreads, chunk_files)

  new_output_file = update_base_invs(new_output_file)

  stats.add_inc_result(iteration_num, new_output_file, time.time() - t1)

  return success, has_any, new_output_file

def update_base_invs(a):
  with open(a) as f:
    j = json.loads(f.read())
  j["base_invs"] = j["base_invs"] + j["new_invs"]
  j["new_invs"] = []

  c = tempfile.mktemp()
  with open(c, 'w') as f:
    f.write(json.dumps(j))
  return c

def chunkify(iterkey, logfile, nthreads, json_filename, args):
  d = tempfile.mkdtemp()
  chunk_file_args = ["--output-chunk-dir", d, "--nthreads", str(nthreads)]

  succ = run_synthesis(logfile, iterkey+".chunkify", json_filename, args_add_seed(chunk_file_args + args))

  assert succ, "chunkify failed"

  chunk_files = []
  i = 0
  while True:
    p = os.path.join(d, str(i + 1))
    if os.path.exists(p):
      i += 1
      chunk_files.append(p)
    else:
      break

  print("chunk_files:", i)

  return chunk_files

def get_chunk_files_for_each_size(chunk_files):
  sizes = []
  for c in chunk_files:
    sizes.append(sizes_mentioned_in_chunk_file(c))
  sz = 0
  res = []
  while True:
    sz += 1
    files = [chunk_files[i] for i in range(len(chunk_files)) if sz in sizes[i]]
    if len(files) == 0:
      break
    res.append((sz, files))
  return res

def sizes_mentioned_in_chunk_file(c):
  res = set()
  with open(c) as f:
    i = 0
    for l in f:
      if i > 0:
        sz = int(l.split()[1])
        res.add(sz)
      i += 1
  return res

def coalesce(logfile, json_filename, iterkey, files):
  new_output_file = tempfile.mktemp()
  coalesce_file_args = ["--coalesce", "--output-formula-file", new_output_file]
  for f in files:
    coalesce_file_args.append("--input-formula-file")
    coalesce_file_args.append(f)
  succ = run_synthesis(logfile, iterkey+".coalesce", json_filename, args_add_seed(coalesce_file_args))
  assert succ, "breadth coalesce failed"
  return new_output_file

def breadth_run_in_parallel(iterkey, logfile, json_filename, main_args, invfile, iteration_num, stats, nthreads, chunk_files):

  if invfile != None:
    args_with_file = ["--input-formula-file", invfile, "--one-breadth"]
  else:
    args_with_file = ["--one-breadth"]

  q = queue.Queue()
  threads = [ ]
  output_files = {}

  n_running = 0
  i = 0

  has_any = False
  any_success = False

  while i < len(chunk_files) or n_running > 0:
    if i < len(chunk_files) and n_running < nthreads:
      output_file = tempfile.mktemp()
      key = iterkey+".thread."+str(i)
      output_files[key] = output_file

      t = threading.Thread(target=run_synthesis, args=
          (logfile, key, json_filename, args_add_seed(
            ["--input-chunk-file", chunk_files[i],
             "--output-formula-file", output_file] + main_args + args_with_file), q))
      t.start()
      threads.append(t)

      i += 1
      n_running += 1
    else:
      synres = q.get()
      n_running -= 1
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
            kill_all_procs()
            any_success = True
            success_file = output_files[key]

  if any_success:
    return (True, has_any, success_file)

  new_output_file = coalesce(logfile, json_filename, iterkey, 
      ([] if invfile == None else [invfile]) + [output_files[key] for key in output_files])

  return (False, has_any, new_output_file)

def do_finisher(iterkey, logfile, nthreads, json_filename, main_args, args, invfile, stats):
  t1 = time.time()

  chunk_files = chunkify(iterkey, logfile, nthreads, json_filename, args)

  if invfile != None:
    args_with_file = ["--input-formula-file", invfile, "--one-finisher"]
  else:
    args_with_file = ["--one-finisher"]

  q = queue.Queue()
  threads = [ ]
  output_files = {}

  n_running = 0
  i = 0

  any_success = False

  while i < len(chunk_files) or n_running > 0:
    if i < len(chunk_files) and n_running < nthreads:
      output_file = tempfile.mktemp()
      key = iterkey+".thread."+str(i)
      output_files[key] = output_file

      t = threading.Thread(target=run_synthesis, args=
          (logfile, key, json_filename, args_add_seed(
            ["--input-chunk-file", chunk_files[i],
             "--output-formula-file", output_file] + main_args + args_with_file), q))
      t.start()
      threads.append(t)

      i += 1
      n_running += 1
    else:
      synres = q.get()
      n_running -= 1
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
            stats.add_finisher_result(output_files[key], time.time() - t1)
            kill_all_procs()

  if not any_success:
    stats.add_finisher_result(output_files[iterkey+".thread.0"], time.time() - t1)

def parse_args(ivy_filename, args):
  nthreads = None
  logfile = ""
  new_args = []
  use_stdout = False
  by_size = False
  config = None
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
    elif args[i] == "--by-size":
      by_size = True
    elif args[i] == "--config":
      config = args[i+1]
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

  (main_args, iter_args) = unpack_args(new_args)

  if config != None:
    assert len(iter_args) == 0
    suite = templates.read_suite(ivy_filename)
    bench = suite.get(config)
    b_args = bench.breadth_space.get_args("--breadth")
    f_args = bench.finisher_space.get_args("--finisher")
  else:
    assert len(iter_args) > 0
    b_args = []
    f_args = []
    for iter_arg_list in iter_args:
      if iter_arg_list[0] == "--breadth":
        b_args = b_args + iter_arg_list
      elif iter_arg_list[0] == "--finisher":
        f_args = f_args + iter_arg_list
      else:
        assert False

  return nthreads, logfile, by_size, main_args, b_args, f_args, use_stdout

def main():
  ivy_filename = sys.argv[1]
  json_filename = sys.argv[2]

  args = sys.argv[3:]
  nthreads, logfile, by_size, main_args, breadth_args, finisher_args, use_stdout = parse_args(ivy_filename, args)
  if nthreads == None:
    if "--config" in args:
      print("You must supply --threads with --config")
    else:
      all_args = main_args + breadth_args + finisher_args
      run_synthesis(logfile, "main", json_filename, all_args, use_stdout=use_stdout)
  else:
    stats = Stats(nthreads, args, ivy_filename, json_filename, logfile)
    do_threading(stats, ivy_filename, json_filename, logfile, nthreads, main_args, breadth_args, finisher_args, by_size)

if __name__ == "__main__":
  main()
