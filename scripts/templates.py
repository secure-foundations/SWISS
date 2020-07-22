class TemplateSpace(object):
  def __init__(self, templ, k, d):
    self.templ = templ
    self.k = k
    self.d = d

class TemplateGen(object):
  def __init__(self, k, d, mvars):
    self.k = k
    self.d = d
    self.mvars = mvars

class BigSpace(object):
  def __init__(self, spaces=None, gen=None):
    self.spaces = spaces
    self.gen = gen

  def get_args(self, alg):
    if self.gen != None:
      assert len(self.spaces) == 0
      return ["--template-sorter", alg,
          str(self.gen.k), str(self.gen.d), str(self.gen.mvars)]
    elif len(self.spaces) > 0:
      res = []
      for space in self.spaces:
        res = res + [alg, "--template-space", space.templ.replace(" ", "-"), "--disj-arity",
            str(space.k)] + (["--depth2-shape"] if space.d == 2 else [])
      return res
    else:
      return []
      

class Bench(object):
  def __init__(self, breadth_space, finisher_space):
    self.breadth_space = breadth_space
    self.finisher_space = finisher_space

class BenchSuite(object):
  def __init__(self, benches):
    self.benches = benches
  def get(self, name):
    return self.benches[name]

def parse_keyvals(line):
  d = {}
  for keyval in line.split():
    s = keyval.split('=')
    d[s[0]] = int(s[1])
  return d

def parse_line(line):
  s = line.split('#')

  t = s[0].split(' ')
  name = t[0]
  rest = ' '.join(t[1:])

  keyvals = parse_keyvals(s[1])

  if name == "template":
    assert len(keyvals) == 2
    return TemplateSpace(rest, keyvals["k"], keyvals["d"])
  elif name == "auto":
    assert len(keyvals) == 3
    return TemplateGen(keyvals["k"], keyvals["d"], keyvals["mvars"])

def parse_space(lines):
  spaces = []
  gen = None
  for line in lines:
    thing = parse_line(line)
    if isinstance(thing, TemplateSpace):
      spaces.append(thing)
    else:
      assert gen == None
      gen = thing
  assert not (gen != None and len(spaces) > 0)
  return BigSpace(spaces, gen)

def parse_bench(lines):
  breadth_lines = []
  finisher_lines = []

  doing_breadth = False
  doing_finisher = False

  for l in lines:
    if l == "[breadth]":
      doing_breadth = True
      doing_finisher = False
    elif l == "[finisher]":
      doing_breadth = False
      doing_finisher = True
    else:
      if doing_breadth:
        breadth_lines.append(l)
      elif doing_finisher:
        finisher_lines.append(l)
      else:
        assert False

  return Bench(parse_space(breadth_lines), parse_space(finisher_lines))

def get_suite_filename(ivy_filename):
  assert ivy_filename.endswith(".ivy")
  return ivy_filename[:-4] + ".spaces"

def read_suite(ivy_filename):
  suite_filename = get_suite_filename(ivy_filename)

  benches = {}

  bench_name = None
  cur_bench = None

  with open(suite_filename) as f:
    for line in f:
      line = line.strip()
      if line != "":
        if line.startswith("[bench "):
          s = line.split()
          assert len(s) == 2
          assert s[1][-1] == "]"
          new_bench_name = s[1][:-1]

          if bench_name is not None:
            benches[bench_name] = parse_bench(cur_bench)

          bench_name = new_bench_name
          cur_bench = []
        else:
          cur_bench.append(line)

  if bench_name is not None:
    benches[bench_name] = parse_bench(cur_bench)

  return BenchSuite(benches)
