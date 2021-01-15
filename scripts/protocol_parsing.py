import subprocess
import tempfile
import json
import tempfile

### Parsing

def ivy_file_json_file(ivy_filename):
  t = tempfile.mktemp()

  with open(t, "w") as f:
    proc = subprocess.Popen(["python2", "./scripts/file_to_json.py", ivy_filename], stdout=f)
    ret = proc.wait()
    assert ret == 0, "file_to_json.py failed"

  return t

def pyv_file_json_file(pyv_filename):
  t = tempfile.mktemp()

  with open(t, "w") as f:
    proc = subprocess.Popen(["python3", "./scripts/file_mypyvy_to_json.py", pyv_filename], stdout=f)
    ret = proc.wait()
    assert ret == 0, "file_mypyvy_to_json.py failed"

  return t

def protocol_file_json_file(protocol_filename):
  assert protocol_filename.endswith(".pyv") or protocol_filename.endswith(".ivy")
  if protocol_filename.endswith(".pyv"):
    return pyv_file_json_file(protocol_filename)
  else:
    return ivy_file_json_file(protocol_filename)

# Parse and get in-memory json

def ivy_file_json(ivy_filename):
  t = tempfile.mktemp()

  proc = subprocess.Popen(["python2", "./scripts/file_to_json.py", ivy_filename], stdout=subprocess.PIPE)
  out, err = proc.communicate()
  ret = proc.wait()
  assert ret == 0, "file_to_json.py failed: " + ivy_filename

  return out

def pyv_file_json(pyv_filename):
  t = tempfile.mktemp()

  proc = subprocess.Popen(["python3", "./scripts/file_mypyvy_to_json.py", pyv_filename], stdout=subprocess.PIPE)
  out, err = proc.communicate()
  ret = proc.wait()
  assert ret == 0, "file_mypyvy_to_json.py failed"

  return out

def protocol_file_json(protocol_filename):
  assert protocol_filename.endswith(".pyv") or protocol_filename.endswith(".ivy")
  if protocol_filename.endswith(".pyv"):
    return pyv_file_json(protocol_filename)
  else:
    return ivy_file_json(protocol_filename)

def parse_invs(protocol_filename, inv_contents):
  js = protocol_file_json(protocol_filename)
  return parse_invs_from_json(js, inv_contents)

def parse_invs_from_json_filename(protocol_json_filename, inv_contents):
  with open(protocol_json_filename) as f:
    j = f.read()
  return parse_invs_from_json(j, inv_contents)

def parse_invs_from_json(protocol_json, inv_contents):
  j = json.loads(protocol_json)

  i = ["#lang ivy1.5"]

  def get_domain(so):
    if so[0] == "functionSort":
      return so[1]
    else:
      return []

  def get_range(so):
    if so[0] == "functionSort":
      return so[2]
    else:
      return so

  n = {"n": 1}
  def sort_to_name_with_id(so):
    i = n["n"]
    n["n"] += 1

    assert so[0] == "uninterpretedSort"
    return "X" + str(i) + ": " + so[1]

  def sort_to_name(so):
    assert so[0] == "uninterpretedSort"
    return so[1]

  for so in j["sorts"]:
    i.append("type " + so)
  for f in j["functions"]:
    assert f[0] in ('var', 'const')
    so = f[2]
    domain = get_domain(so)
    rng = get_range(so)
    i.append(
        ('relation ' if rng[0] == 'booleanSort' else 'function ') + f[1]
        + ('(' + ', '.join(sort_to_name_with_id(d) for d in domain) + ')' if len(domain) > 0 else '')
        + (': ' + sort_to_name(rng) if rng[0] != 'booleanSort' else '')
    )

  i = "\n".join(i) + "\n\n" + inv_contents + "\n"

  tmpivy = tempfile.mktemp()
  with open(tmpivy, "w") as f:
    f.write(i)

  t = ivy_file_json(tmpivy)
  return (j, json.loads(t)["conjectures"])

### Serializing

def value_json_to_string(inv):
  return inv_to_str(inv)

def inv_to_str(inv):
  #print(inv)
  assert type(inv) == list
  if inv[0] == 'forall' or inv[0] == 'exists':
    return '(' + inv[0] + " " + ", ".join(decl_to_str(decl) for decl in inv[1]) + " . (" + inv_to_str(inv[2]) + '))'
  elif inv[0] == 'or':
    return '(' + ' | '.join(inv_to_str(i) for i in inv[1]) + ')'
  elif inv[0] == 'and':
    return '(' + ' & '.join(inv_to_str(i) for i in inv[1]) + ')'
  elif inv[0] == 'not':
    return '(~(' + inv_to_str(inv[1]) + '))'
  elif inv[0] == 'apply':
    return const_name(inv[1]) + '(' + ', '.join(inv_to_str(i) for i in inv[2]) + ')'
  elif inv[0] == 'var':
    return inv[1]
  elif inv[0] == 'const':
    return inv[1]
  elif inv[0] == 'eq':
    return '(' + inv_to_str(inv[1]) + ' = ' + inv_to_str(inv[2]) + ')'
  elif inv[0] == 'implies':
    return '(' + inv_to_str(inv[1]) + ' -> ' + inv_to_str(inv[2]) + ')'
  else:
    print("inv_to_str")
    print(repr(inv))
    assert False

def decl_to_str(decl):
  assert decl[0] == 'var'
  assert decl[2][0] == 'uninterpretedSort'
  return decl[1] + ':' + decl[2][1]

def const_name(c):
  assert c[0] == 'const'
  return c[1]

def parse_module_invs_invs_invs(protocol_filename, inv_filename1, inv_filename2):
  module_json_filename = protocol_file_json_file(protocol_filename)
  with open(module_json_filename) as f:
    j = f.read()

  with open(inv_filename1) as f:
    inv_contents1 = f.read()

  if inv_filename2 != None:
    with open(inv_filename2) as f:
      inv_contents2 = f.read()

  i1 = parse_invs_from_json_filename(module_json_filename, inv_contents1)[1]

  if inv_filename2 != None:
    i2 = parse_invs_from_json_filename(module_json_filename, inv_contents2)[1]
  else:
    i2 = None

  j = json.loads(j)

  return (module_json_filename, j["conjectures"], i1, i2)
