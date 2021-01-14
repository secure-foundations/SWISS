import protocol_parsing
import os
import json
import subprocess
import sys
import tempfile
import paper_benchmarks
import traceback

def get_protocol_filename(logdir):
  summary_file = os.path.join(logdir, "summary")
  with open(summary_file, "r") as f:
    for line in f:
      if line.startswith("Protocol: "):
        return line[10:].strip()
      if line.startswith("./save.sh "):
        return line.split()[1]

  assert False, "could not find protocol filename"

def does_claim_success(i):
  if "# Success: True" in i:
    return True
  elif "# Success: False" in i:
    return False
  elif "# timed out and extracted" in i:
    return False
  elif "# Extracted from logfiles" in i: # legacy
    return False
  else:
    assert False

def did_succeed(logdir):
  with open(os.path.join(logdir, "invariants")) as f:
    inv_contents = f.read()

  return does_claim_success(inv_contents)
        
def validate_run_invariants(logdir):
  with open(os.path.join(logdir, "invariants")) as f:
    inv_contents = f.read()

  claims_success = does_claim_success(inv_contents)

  if not claims_success:
    print("invariants file does not claim invariants are correct")
  else:
    print("invariants file claims to contain correct and complete invariants")
    protocol_filename = get_protocol_filename(logdir)
    print("protocol file: " + protocol_filename)
    j, invs = protocol_parsing.parse_invs(protocol_filename, inv_contents)

    j["conjectures"] = j["conjectures"] + invs

    tmpjson = tempfile.mktemp()
    with open(tmpjson, "w") as f:
      f.write(json.dumps(j))

    proc = subprocess.Popen(["./synthesis", "--input-module", tmpjson, "--check-inductiveness"])
    ret = proc.wait()

def count_terms_of_value_list(invs):
  def count_terms(v):
    if v[0] in ('forall', 'exists'):
      return count_terms(v[2])
    elif v[0] in ('and', 'or'):
      return sum(count_terms(t) for t in v[1])
    elif v[0] == 'not':
      return count_terms(v[1])
    elif v[0] == 'implies':
      return count_terms(v[1]) + count_terms(v[2])
    elif v[0] == 'apply':
      return 1
    elif v[0] == 'const':
      return 1
    elif v[0] == 'eq':
      return 1
    else:
      print(v)
      assert False

  def get_quants(v):
    if v[0] in ('forall', 'exists'):
      sorts = [ decl[2][1] for decl in v[1] ]
      new_qs = [(v[0], s) for s in sorts]
      return new_qs + get_quants(v[2])
    elif v[0] in ('and', 'or'):
      qs = []
      for t in v[1]:
        q = get_quants(t)
        if len(q) == 0:
          assert len(qs) == 0
          qs = q
      return qs
    elif v[0] == 'not':
      return get_quants(v[1])
    elif v[0] == 'implies':
      qs = []
      for t in [v[1], v[2]]:
        q = get_quants(t)
        if len(q) == 0:
          assert len(qs) == 0
          qs = q
      return qs
    elif v[0] == 'apply':
      return []
    elif v[0] == 'const':
      return []
    elif v[0] == 'eq':
      return []
    else:
      print(v)
      assert False

  def num_exists(v):
    return len([x for x in v if x[0] == 'exists'])

  def num_alts(v):
    alt = 0
    for i in range(1, len(v)):
      if v[i][0] != v[i-1][0]:
        alt += 1
    return alt

  count = sum(count_terms(i) for i in invs)
  #print("total terms: " + str(count))
  max_terms = max(count_terms(i) for i in invs)

  quants = [get_quants(i) for i in invs]

  max_vars = max(len(q) for q in quants)
  max_exists = max(num_exists(q) for q in quants)
  max_alts = max(num_alts(q) for q in quants)

  return {"invs": len(invs), "terms": count, "max_terms": max_terms,
      "max_vars": max_vars, "max_exists": max_exists, "max_alts": max_alts}

def do_single_impl_check(module_json_file, lhs_invs, rhs_invs):
  proc = subprocess.Popen(["./synthesis", "--input-module", module_json_file,
      "--big-impl-check", lhs_invs, rhs_invs], stderr=subprocess.PIPE)
  out, err = proc.communicate()
  ret = proc.wait()

  assert ret == 0
  for line in err.split(b'\n'):
    if line.startswith(b"could prove "):
      l = line.split()
      a = int(l[2])
      assert l[3] == b"/"
      b = int(l[4])
      return (a, b)
  assert False, "did not find output line"

def impl_check(ivyname, b,
    module_json_file, module_invs, gen_invs, answer_invs):
  def write_invs_file(c):
    t = tempfile.mktemp()
    with open(t, "w") as f:
      f.write(json.dumps(c))
    return t

  module_invs_file = write_invs_file(module_invs)
  gen_invs_file = write_invs_file(gen_invs)
  answer_invs_file = write_invs_file(answer_invs)

  conj_got, conj_total = do_single_impl_check(module_json_file,
      gen_invs_file, module_invs_file)

  invs_got, invs_total = do_single_impl_check(module_json_file,
      gen_invs_file, answer_invs_file)

  return {
    "safeties_got" : conj_got,
    "safeties_total" : conj_total,
    "invs_got" : invs_got,
    "invs_total" : invs_total,
  }

def do_analysis(ivyname, b):
  if not os.path.exists(b):
    print("could not find", b, " ... skipping")
    return

  try:
    print(b)

    answers_filename = ".".join(ivyname.split(".")[:-1] + ["answers"])

    if not os.path.exists(answers_filename):
      print("file does not exist: " + answers_filename)
      answers_filename = None

    module_json_file, module_invs, gen_invs, answer_invs = (
        protocol_parsing.parse_module_invs_invs_invs(
          ivyname,
          os.path.join(b, "invariants"),
          answers_filename
        )
    )

    d_syn = count_terms_of_value_list(gen_invs)
    d = {
      "synthesized_invs": d_syn["invs"],
      "synthesized_terms": d_syn["terms"],
      "synthesized_max_terms": d_syn["max_terms"],
      "synthesized_max_vars": d_syn["max_vars"],
      "synthesized_max_exists": d_syn["max_exists"],
      "synthesized_max_alts": d_syn["max_alts"],
    }

    if answer_invs != None:
      d_hand = count_terms_of_value_list(answer_invs)
      d["handwritten_invs"] = d_hand["invs"]
      d["handwritten_terms"] = d_hand["terms"]
      d["handwritten_max_terms"] = d_hand["max_terms"]
      d["handwritten_max_vars"] = d_hand["max_vars"]
      d["handwritten_max_exists"] = d_hand["max_exists"]
      d["handwritten_max_alts"] = d_hand["max_alts"]

    was_success = did_succeed(b)
    if (not was_success) and answer_invs != None:
      d1 = impl_check(ivyname, b,
          module_json_file, module_invs, gen_invs, answer_invs)
      d = {**d, **d1}

    res_filename = os.path.join(b, "inv_analysis")

    print(d)

    with open(res_filename, "w") as f:
      f.write(json.dumps(d))
  except Exception:
    traceback.print_exc()

def run_analyses(input_directory):
  all_main_benches = paper_benchmarks.get_all_main_benchmarks()
  for b in all_main_benches:
    name = b.get_name()
    do_analysis("benchmarks/" + b.ivyname,
        os.path.join(input_directory, name))

if __name__ == '__main__':
  #validate_run_invariants(sys.argv[1])
  #count_terms_of_tmpfile(sys.argv[1])
  run_analyses(sys.argv[1])
