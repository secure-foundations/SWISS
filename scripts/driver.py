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
import shutil

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

def reseed(args):
  new_args = []
  i = 0
  while i < len(args):
    if args[i] != "--seed":
      new_args.append(args[i])
      i += 1
    else:
      i += 2
  return args_add_seed(new_args)

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

def get_input_files(args):
  t = []
  for i in range(len(args) - 1):
    if args[i] in ('--input-module', "--input-chunk-file", "--input-formula-file"):
      t.append(args[i+1])
  return t

def log_inputs(logfilename, json_filename, args):
  try:
    input_files = [json_filename] + get_input_files(args)
    for i in range(len(input_files)):
      a = input_files[i]
      b = logfilename + ".error." + str(i)
      print(a + " -> " + b)
      shutil.copy(input_files[i], logfilename + ".error." + str(i))

  except Exception:
    print("failed to log_inputs")
    pass

def run_synthesis_off_thread(logfile_base, run_id, json_filename, args, q):
  res = run_synthesis_retry(logfile_base, run_id, json_filename, args)
  q.put(res)

def run_synthesis_retry(logfile_base, run_id, json_filename, args):
  nfails = 0
  all_res = []
  while True:
    extra = "" if nfails == 0 else ".retry." + str(nfails)
    res = run_synthesis(logfile_base, run_id+extra, json_filename, args)
    all_res.append(res)
    if killing:
      res.all_res = all_res
      return res
    if not res.failed:
      res.all_res = all_res
      return res
    nfails += 1
    if nfails >= 3:
      res.all_res = all_res
      return res
    print('!!!!!!!!! synthesis', run_id,
        'failed, re-seeding and trying again', '!!!!!!!!!')
    args = reseed(args)

def run_synthesis(logfile_base, run_id, json_filename, args, use_stdout=False):
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
          print("failed " + run_id + " (" + str(seconds) + " seconds) (ret " + str(ret) + ") (pid " + str(proc.pid) + ")")
          log_inputs(logfilename, json_filename, args)
          sys.stdout.flush()
        else:
          print("complete " + run_id + " (" + str(seconds) + " seconds)")
          sys.stdout.flush()

    return RunSynthesisResult(run_id, seconds, killing, ret != 0, logfilename)
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

  if by_size:
    c_by_size = chunkify_by_size(iterkey, logfile, nthreads, json_filename, args)
    total_has_any = False
    sz = 0
    for chunks in c_by_size:
      sz += 1
      success, has_any, output_invfile = breadth_run_in_parallel(
        iterkey+".size."+str(sz), logfile, json_filename, main_args,
          invfile, iteration_num, stats, nthreads, chunks)
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
    chunk_files = chunkify(iterkey, logfile, nthreads, json_filename, args)
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

  synres = run_synthesis_retry(logfile, iterkey+".chunkify", json_filename, args_add_seed(chunk_file_args + args))

  assert not synres.failed, "chunkify failed"

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

def chunkify_by_size(iterkey, logfile, nthreads, json_filename, args):
  d = tempfile.mkdtemp()
  chunk_file_args = ["--output-chunk-dir", d, "--nthreads", str(nthreads), "--by-size"]

  synres = run_synthesis_retry(logfile, iterkey+".chunkify", json_filename, args_add_seed(chunk_file_args + args))

  assert not synres.failed, "chunkify failed"

  all_chunk_files = []
  j = 0
  while True:
    chunk_files = []
    i = 0
    while True:
      p = os.path.join(d, str(j + 1) + "." + str(i + 1))
      if os.path.exists(p):
        i += 1
        chunk_files.append(p)
      else:
        break
    if len(chunk_files) > 0:
      all_chunk_files.append(chunk_files)
      j += 1
    else:
      break

  print("chunk_files by size: ", ", ".join(str(len(f)) for f in all_chunk_files))

  return all_chunk_files

def coalesce(logfile, json_filename, iterkey, files):
  new_output_file = tempfile.mktemp()
  coalesce_file_args = ["--coalesce", "--output-formula-file", new_output_file]
  for f in files:
    coalesce_file_args.append("--input-formula-file")
    coalesce_file_args.append(f)
  synres = run_synthesis_retry(logfile, iterkey+".coalesce", json_filename, args_add_seed(coalesce_file_args))
  assert not synres.failed, "breadth coalesce failed"
  return new_output_file

def remove_one(s):
  assert len(s) > 0
  t = min(s)
  s.remove(t)
  return t

def breadth_run_in_parallel(iterkey, logfile, json_filename, main_args, invfile, iteration_num, stats, nthreads, chunk_files):

  if invfile != None:
    args_with_file = ["--input-formula-file", invfile, "--one-breadth"]
  else:
    args_with_file = ["--one-breadth"]

  q = queue.Queue()
  threads = [ ]
  output_files = {}

  column_ids = {} # assign each proc a column id in [1..nthreads] for graphing purposes
  avail_column_ids = set(range(1, nthreads + 1))

  n_running = 0
  i = 0

  has_any = False
  any_success = False

  while ((not any_success) and i < len(chunk_files)) or n_running > 0:
    if (not any_success) and i < len(chunk_files) and n_running < nthreads:
      output_file = tempfile.mktemp()
      key = iterkey+".thread."+str(i)
      output_files[key] = output_file
      cid = remove_one(avail_column_ids)
      column_ids[key] = cid

      t = threading.Thread(target=run_synthesis_off_thread, args=
          (logfile, key, json_filename, args_add_seed(
            ["--input-chunk-file", chunk_files[i],
             "--output-formula-file", output_file] + main_args + args_with_file), q))
      t.start()
      threads.append(t)

      i += 1
      n_running += 1
    else:
      synres = q.get()

      key = synres.all_res[0].run_id
      cid = column_ids[key]
      avail_column_ids.add(cid)

      n_running -= 1
      if synres.failed and not synres.stopped:
        print("!!!!! WARNING PROC FAILED, SKIPPING !!!!!")
        for r in synres.all_res:
          stats.add_inc_log(iteration_num, r.logfile, r.seconds, cid, r.failed)
        output_files.pop(key)
        #kill_all_procs()
        #assert False, "breadth proper failed"
      else:
        for r in synres.all_res:
          stats.add_inc_log(iteration_num, r.logfile, r.seconds, cid, r.failed)
        if not synres.stopped and not killing:
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

  column_ids = {} # assign each proc a column id in [1..nthreads] for graphing purposes
  avail_column_ids = set(range(1, nthreads + 1))

  n_running = 0
  i = 0

  any_success = False

  while ((not any_success) and i < len(chunk_files)) or n_running > 0:
    if (not any_success) and i < len(chunk_files) and n_running < nthreads:
      output_file = tempfile.mktemp()
      key = iterkey+".thread."+str(i)
      output_files[key] = output_file
      cid = remove_one(avail_column_ids)
      column_ids[key] = cid

      t = threading.Thread(target=run_synthesis_off_thread, args=
          (logfile, key, json_filename, args_add_seed(
            ["--input-chunk-file", chunk_files[i],
             "--output-formula-file", output_file] + main_args + args_with_file), q))
      t.start()
      threads.append(t)

      i += 1
      n_running += 1
    else:
      synres = q.get()

      key = synres.all_res[0].run_id
      cid = column_ids[key]
      avail_column_ids.add(cid)

      n_running -= 1
      if synres.failed and not synres.stopped:
        print("!!!!! WARNING PROC FAILED, SKIPPING !!!!!")
        for r in synres.all_res:
          stats.add_finisher_log(r.logfile, r.seconds, cid, r.failed)
        output_files.pop(key)
        #kill_all_procs()
        #assert False, "finisher proper failed"
      else:
        for r in synres.all_res:
          stats.add_finisher_log(r.logfile, r.seconds, cid, r.failed)
        if not synres.stopped and not killing:
          success, this_has_any = parse_output_file(output_files[key])
          if success:
            any_success = True
            stats.add_finisher_result(output_files[key], time.time() - t1)
            kill_all_procs()

  some_key = None
  for some_key in output_files:
    break
  if not any_success:
    stats.add_finisher_result(output_files[some_key], time.time() - t1)

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
