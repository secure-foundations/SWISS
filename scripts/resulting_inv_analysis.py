import protocol_parsing
import os
import json
import subprocess
import sys
import tempfile
import paper_benchmarks

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

def count_terms_of_tmpfile(logdir):
  with open(os.path.join(logdir, "invariants")) as f:
    inv_contents = f.read()
  protocol_filename = get_protocol_filename(logdir)
  j, invs = protocol_parsing.parse_invs(protocol_filename, inv_contents)

  def count_terms(v):
    if v[0] in ('forall', 'exists'):
      return count_terms(v[2])
    elif v[0] in ('and', 'or'):
      return sum(count_terms(t) for t in v[1])
    elif v[0] == 'not':
      return count_terms(v[1])
    elif v[0] == 'apply':
      return 1
    elif v[0] == 'const':
      return 1
    elif v[0] == 'eq':
      return 1
    else:
      print(v)
      assert False

  count = sum(count_terms(i) for i in invs)
  #print("total terms: " + str(count))

  return {"invs": len(invs), "terms": count}

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

def impl_check(ivyname, b):
  answers_filename = ".".join(ivyname.split(".")[:-1] + ["answers"])
  module_json_file, module_invs, gen_invs, answer_invs = (
      protocol_parsing.parse_module_invs_invs_invs(
        ivyname,
        os.path.join(b, "invariants"),
        answers_filename
      )
  )

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
  print(b)
  d = count_terms_of_tmpfile(b)

  was_success = did_succeed(b)
  if not was_success:
    d1 = impl_check(ivyname, b)
    d = {**d, **d1}

  res_filename = os.path.join(b, "inv_analysis")

  print(d)

  with open(res_filename, "w") as f:
    f.write(json.dumps(d))

def run_analyses(input_directory):
  all_main_benches = paper_benchmarks.get_all_main_benchmarks()
  for b in all_main_benches:
    name = b.get_name()
    if "multi" in name:
      do_analysis("benchmarks/" + b.ivyname,
          os.path.join(input_directory, name))

if __name__ == '__main__':
  #validate_run_invariants(sys.argv[1])
  #count_terms_of_tmpfile(sys.argv[1])
  run_analyses(sys.argv[1])
