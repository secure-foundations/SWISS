import json

def parse_comma_sep(tokens, i):
  e1, i = parse1(tokens, i)

  es = [e1]

  while True:
    if i < len(tokens) and tokens[i] == ',':
      enext, i = parse1(tokens, i+1)
      es.append(enext)
    else:
      return es, i

def parse1(tokens, i):
  e1, i = parse2(tokens, i)

  if i < len(tokens) and tokens[i] == '->':
    e2, i = parse2(tokens, i+1)
    return ['implies', e1, e2], i
  else:
    return e1, i

def parse2(tokens, i):
  e1, i = parse3(tokens, i)

  do_or = (i < len(tokens) and tokens[i] == '|')
  do_and = (i < len(tokens) and tokens[i] == '&')

  if (not do_or) and (not do_and):
    return e1, i

  the_chr = '|' if do_or else '&'

  es = [e1]

  while True:
    if i < len(tokens) and tokens[i] == the_chr:
      enext, i = parse3(tokens, i+1)
      es.append(enext)
    else:
      return [("or" if do_or else "and"), es], i

def parse3(tokens, i):
  e1, i = parse_atom(tokens, i)

  if i < len(tokens) and (tokens[i] == '=' or tokens[i] == '~='):
    tok = tokens[i]
    e2, i = parse_atom(tokens, i+1)
    if tok == '=':
      return ['eq', e1, e2], i
    else:
      return ['not', ['eq', e1, e2]], i
  else:
    return e1, i

def is_var(c):
  return 'A' <= c[0] <= 'Z'

def is_const(c):
  return 'a' <= c[0] <= 'z'

def is_id(c):
  return is_var(c) or is_const(c)

def parse_atom(tokens, i):
  assert i < len(tokens)

  if tokens[i] == '(':
    e, i = parse1(tokens, i+1)
    assert i < len(tokens) and tokens[i] == ')'
    return e, i+1
  elif tokens[i] == '~':
    e, i = parse_atom(tokens, i+1)
    return ["not", e], i
  elif tokens[i] in ('forall', 'exists'):
    qtype = tokens[i]
    decls = []
    i += 1
    while True:
      assert is_id(tokens[i])
      id_name = tokens[i]
      i += 2
      if tokens[i-1] == ':':
        sort_name = tokens[i]
        i += 2
        assert tokens[i-1] == ',' or tokens[i-1] == '.'
        decls.append(['var', id_name, ["uninterpretedSort", sort_name]])
      else:
        decls.append(['var', id_name])
      assert tokens[i-1] == ',' or tokens[i-1] == '.'
      if tokens[i-1] == '.':
        break
    ebody, i = parse1(tokens, i)
    return [qtype, decls, ebody], i
  elif is_id(tokens[i]):
    c = tokens[i]
    if is_var(c):
      return ["var", c], i+1
    elif is_const(c):
      if i+1 < len(tokens) and tokens[i+1] == '(':
        es, i = parse_comma_sep(tokens, i + 2)
        assert tokens[i] == ')'
        return ["apply", ["const", c], es], i+1
      else:
        return ["const", c], i+1
  else:
    assert False
      
def tokenize_cont(c):
  tokens = []
  i = 0
  while i < len(c):
    if ('a' <= c[i] <= 'z') or ('A' <= c[i] <= 'Z'):
      j = i + 1
      while j < len(c) and (('a' <= c[j] <= 'z') or ('A' <= c[j] <= 'Z') or ('0' <= c[j] <= '9') or c[j] == '_'):
        j += 1
      tokens.append(c[i:j])
      i = j
    elif i+1 < len(c) and c[i] == '-' and c[i+1] == '>':
      tokens.append('->')
      i += 2
    elif i+1 < len(c) and c[i] == '~' and c[i+1] == '=':
      tokens.append('~=')
      i += 2
    else:
      tokens.append(c[i])
      i += 1
  return tokens

def tokenize(s):
  s = s.split()
  tokens = []
  for c in s:
    ts = tokenize_cont(c)
    tokens.extend(ts)
  return tokens

def parse_string(module, s):
  tokens = tokenize(s)
  e, i = parse1(tokens, 0)
  assert i == len(tokens)
  e = type_infer(module, e)
  return e

class Scope(object):
  def __init__(self, consts, vars):
    self.vars = vars
    self.consts = consts

  def add_var(self, new_var, new_type):
    assert new_var not in self.vars
    self.vars[new_var] = new_type

  def remove_var(self, var):
    if var in self.vars:
      del self.vars[var]

  def get_arg(self, c, i):
    funcsort = self.consts[c]
    assert funcsort[0] == "functionSort"
    return funcsort[1][i]

  def get_range(self, c, i):
    funcsort = self.consts[c]
    assert funcsort[0] == "functionSort"
    return funcsort[2]

def get_vars(v, decls):
  if v[0] in ('forall', 'exists'):
    return get_vars(v[2], decls + [decl[1] for decl in v[1]])
  elif v[0] in ('and', 'or'):
    t = set()
    for arg in v[1]:
      t = t.union(get_vars(arg, decls))
    return t
  elif v[0] == 'not':
    return get_vars(v[1], decls)
  elif v[0] == 'implies':
    t = set()
    t = t.union(get_vars(v[1], decls))
    t = t.union(get_vars(v[2], decls))
    return t
  elif v[0] == 'apply':
    t = set()
    for arg in v[2]:
      t = t.union(get_vars(arg, decls))
    return t
  elif v[0] == 'const':
    return set()
  elif v[0] == 'var':
    if v[1] not in decls:
      return {v[1]}
    else:
      return set()
  elif v[0] == 'eq':
    t = set()
    t = t.union(get_vars(v[1], decls))
    t = t.union(get_vars(v[2], decls))
    return t
  else:
    assert False


def has_any_uninferred(v):
  if v[0] in ('forall', 'exists'):
    r = has_any_uninferred(v[2]) or any(len(l) == 2 for l in v[1])
  elif v[0] in ('and', 'or'):
    r = any(has_any_uninferred(a) for a in v[1])
  elif v[0] == 'not':
    r = has_any_uninferred(v[1])
  elif v[0] == 'implies':
    r = has_any_uninferred(v[1]) or has_any_uninferred(v[2])
  elif v[0] == 'apply':
    r = has_any_uninferred(v[1]) or any(has_any_uninferred(a) for a in v[2])
  elif v[0] == 'const':
    r = len(v) == 2
  elif v[0] == 'var':
    r = len(v) == 2
  elif v[0] == 'eq':
    r = has_any_uninferred(v[1]) or has_any_uninferred(v[2])
  else:
    assert False

  return r

def do_infer(v, scope):
  if v[0] in ('forall', 'exists'):
    for decl in v[1]:
      if len(decl) == 3:
        scope.add_var(decl[1], decl[2])
    body = do_infer(v[2], scope)

    new_decls = []
    for decl in v[1]:
      if len(decl) == 3:
        new_decls.append(decl)
      elif decl[1] in scope.vars:
        new_decls.append(["var", decl[1], scope.vars[decl[1]]])
      else:
        new_decls.append(decl)
      scope.remove_var(decl[1])
    return [v[0], new_decls, body]
  elif v[0] in ('and', 'or'):
    return [v[0], [do_infer(arg, scope) for arg in v[1]]]
  elif v[0] == 'not':
    return ['not', do_infer(v[1], scope)]
  elif v[0] == 'implies':
    return ['implies', do_infer(v[1], scope), do_infer(v[2], scope)]
  elif v[0] == 'apply':
    c = do_infer(v[1], scope)
    args = [do_infer(arg, scope) for arg in v[2]]
    for i in range(len(args)):
      if args[i][0] == 'var' and len(args[i]) == 2:
        args[i] = ['var', args[i][1], scope.get_arg(c[1], i)]
    return ["apply", c, args]
  elif v[0] == 'const':
    if len(v) == 3:
      return v
    else:
      return ["const", v[1], scope.consts[v[1]]]
  elif v[0] == 'var':
    if len(v) == 3:
      if v[1] in scope.vars:
        assert repr(scope.vars[v[1]]) == repr(v[2])
      else:
        scope.vars[v[1]] = v[2]
      return v
    elif v[1] in scope.vars:
      return ["var", v[1], scope.vars[v[1]]]
    else:
      return v
  elif v[0] == 'eq':
    a = do_infer(v[1], scope)
    b = do_infer(v[2], scope)

    a = maybe_infer(a, b, scope)
    b = maybe_infer(b, a, scope)

    return ['eq', a, b]
  else:
    assert False

def const_types_of_module(module):
  d = {}
  for func in module["functions"]:
    d[func[1]] = func[2]
  return d

def try_infer_sort(v, scope):
  if v[0] == 'var' and len(v) == 3:
    return v[2]
  elif v[0] == 'const' and len(v) == 3:
    return v[2]
  elif v[0] == 'apply':
    return scope.get_range(v[1])
  else:
    return None

def maybe_infer(a, b, scope):
  if a[0] == 'var' and len(a) == 2:
    so = try_infer_sort(b, scope)
    if so != None:
      return [a[0], a[1], so]
    else:
      return a
  else:
    return a

def type_infer(module, e):
  vs = get_vars(e, [])
  if len(vs) > 0:
    e = ['forall', [["var", x] for x in vs], e]

  module = json.loads(module)
  const_types = const_types_of_module(module)
  while True:
    if not has_any_uninferred(e):
      return e
    e1 = do_infer(e, Scope(const_types, {}))
    if repr(e) == repr(e1):
      print(e)
      print(e1)
      assert False
    e = e1

def parse_inv_contents(invs_contents, module):
  lines = invs_contents.split('\n')
  lines = [l.split('#')[0] for l in lines]
  al = '\n'.join(lines)
  cs = al.split('conjecture')
  assert cs[0].strip() == ''
  cs = cs[1:]

  return [parse_string(module, c) for c in cs]
