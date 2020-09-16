import subprocess

import templates

# paperlogs/2020august/mm__leader-election__auto__seed1_t24

class CountSet(object):
  def __init__(self, presymm, postsymm):
    self.presymm = presymm
    self.postsymm = postsymm

def get_finisher_params_from_file(filename):
  protocol_file = get_protocol_from_file(filename)
  suite = templates.read_suite(protocol_file)
  config_name = get_config_from_file(filename)
  bench = suite.get(config_name)
  space = bench.finisher_space
  return space.get_args('--finisher')


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
  return space.get_args('--breadth')

def calc_counts(protocol_file, params):
  cmd = ["./run-simple.sh", protocol_file] + ["--counts-only"] + params
  #print(' '.join(cmd))
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
  #print(presymm, postsymm)
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

def sorted_counts(protocol_file, params):
  cmd = ["./run-simple.sh", protocol_file] + params + ["--counts-only"]
  print(' '.join(cmd))
  proc = subprocess.Popen(cmd,
      stdin=subprocess.PIPE,
      stdout=subprocess.PIPE)
  out, err = proc.communicate()
  ret = proc.wait()
  assert ret == 0

  res = []
  for l in out.split(b'\n'):
    l = l.strip()
    if l.startswith(b"TemplateSlice["):
      res.append(l.decode('utf-8'))
  return sort_spaces(res)

def count_of_space_str(s):
  t = s.split()
  assert t[5] == 'count'
  return int(t[6])

def sort_spaces(spaces):
  t = [(count_of_space_str(space), space) for space in spaces]
  t.sort()
  return [x[1] for x in t]

def print_sorted_spaces(protocol_file, params):
  s = sorted_counts(protocol_file, params)
  for l in s:
    print(l)

if __name__ == '__main__':
  print_sorted_spaces("benchmarks/decentralized-lock.ivy", '--template-sorter 9 2 6 0'.split())
  #get_counts("paperlogs/2020august/mm__flexible_paxos__auto__seed1_t24", "finisher")
