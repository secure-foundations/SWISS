import os
import subprocess

def make_ivy_or_pyv_file(pfile, ffile):
  with open(pfile) as f:
    p = f.read()
  with open(ffile) as f:
    ff = f.read()

  num_conjs = len(ff.split('conjecture')) - 1

  if pfile.endswith(".pyv"):
    ff = ff.replace("conjecture", "safety").replace("~", "!")
    newname = "combo.pyv"
  else:
    newname = "combo.ivy"

  with open(newname, "w") as f:
    f.write(p)
    f.write('\n\n')
    f.write(ff)
  
  return (newname, num_conjs)

def run(newname, num_conjs):
  proc = subprocess.Popen(["./run-simple.sh", newname, "--big-impl-check", str(num_conjs)],
      stdout=subprocess.PIPE)
  out, err = proc.communicate()
  ret = proc.wait()
  assert ret == 0

  for line in out.split(b'\n'):
    if line.startswith(b"could prove"):
      return line.decode('utf-8')
  assert False

def go(ffile, pfile):
  cfile, nc = make_ivy_or_pyv_file(pfile, ffile)
  ans = run(cfile, nc)
  print(ffile, " : ", ans)

#go("partial_invs/mm_nonacc__chain__auto__seed1_t8", "answers/chain.ivy")
#go("partial_invs/mm_nonacc__chord__auto__seed1_t8", "answers/chord.ivy")
#go("partial_invs/mm_nonacc__distributed_lock__auto9__seed1_t8", "answers/distributed_lock.ivy")
#go("partial_invs/mm_nonacc__hybrid_reliable_broadcast_cisa_pyv__auto__seed1_t8", "answers/hybrid_reliable_broadcast_cisa.pyv")
#go("partial_invs/mm_nonacc__sharded_kv_no_lost_keys_pyv__auto9__seed1_t8", "answers/sharded_kv_no_lost_keys.pyv")
#go("partial_invs/mm_nonacc__ticket_pyv__auto__seed1_t8", "answers/ticket.pyv")
#go("partial_invs/mm_nonacc__multi_paxos__auto__seed1_t8", "answers/multi_paxos.ivy")
#go("partial_invs/mm_nonacc__vertical_paxos__auto__seed1_t8", "answers/vertical_paxos.ivy")
#go("partial_invs/mm_nonacc__fast_paxos__auto__seed1_t8", "answers/fast_paxos.ivy")
go("partial_invs/mm_nonacc__stoppable_paxos__auto__seed1_t8", "answers/stoppable_paxos.ivy")
