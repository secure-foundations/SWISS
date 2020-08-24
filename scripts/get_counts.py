import subprocess

import templates

# paperlogs/2020august/mm__leader-election__auto__seed1_t24

class CountSet(object):
  def __init__(self, presymm, postsymm):
    self.presymm = presymm
    self.postsymm = postsymm

def get_finisher_params_from_file(filename):
  with open(filename) as f:
    for line in f:
      if line.startswith('Resulting invariant parameters:'):
        params_str = line.split(':')[1]
        params = templates.parse_keyvals(params_str)
        break
  return ["--template-sorter",
      str(params["k"]),
      str(params["d"]),
      str(params["m"]),
      str(params["e"])]

def get_protocol_from_file(filename):
  with open(filename) as f:
    for line in f:
      if line.startswith('Protocol:'):
        return line.split()[1]
  assert False

def get_config_from_file(filename):
  with open(filename) as f:
    for line in f:
      if line.startswith('Args:'):
        params = line.split()
        i = params.index('--config')
        return params[i+1]
  assert False

def get_breadth_params_from_file(protocol_file, filename):
  protocol_file = get_protocol_from_file(filename)
  suite = templates.read_suite(protocol_file)
  config_name = get_config_from_file(filename)
  bench = suite.get(config_name)
  space = bench.breadth_space
  return space.get_args('breadth')

def calc_counts(protocol_file, params):
  cmd = ["./run-simple.sh", protocol_file] + params + ["--counts-only"]
  print(' '.join(cmd))
  proc = subprocess.Popen(cmd,
      stdin=subprocess.PIPE,
      stdout=subprocess.PIPE)
  out, err = proc.communicate()
  ret = proc.wait()
  assert ret == 0

  presymm = None
  postsymm = None
  for line in out.split(b'\n'):
    if line.startswith(b"Pre-symmetries:"):
      presymm = int(line.split()[1])
    if line.startswith(b"Post-symmetries:"):
      postsymm = int(line.split()[1])
  assert presymm != None
  assert postsymm != None
  print(presymm, postsymm)
  return presymm, postsymm

def get_counts(filename, alg):
  protocol_file = get_protocol_from_file(filename)
  if alg == 'breadth':
    params = get_breadth_params_from_file(protocol_file, filename)
  elif alg == 'finisher':
    params = get_finisher_params_from_file(filename)
  else:
    assert False

  presymm, postsymm = calc_counts(protocol_file, params)

  return CountSet(presymm, postsymm) 

if __name__ == '__main__':
  get_counts("paperlogs/2020august/mm__flexible_paxos__auto__seed1_t24",
      "finisher")
