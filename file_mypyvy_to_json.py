import sys
import json
import os

sys.path.insert(0, os.path.abspath(os.path.join(
    os.path.dirname(__file__), 'mypyvy/src')))

import parser
import typechecker
import syntax

def parse_program(input, filename = None):
    l = parser.get_lexer()
    p = parser.get_parser(forbid_rebuild=False)
    prog = p.parse(input=input, lexer=l, filename=filename)
    prog.input = input
    return prog

def binder_to_json(binder):
  d = []
  for v in binder.vs:
    d.append(["var", v.name, sort_to_json(v.sort)])
  return d

class Mods(object):
  def __init__(self, mods):
    self.mods = mods
    self.in_new = False
  def with_new(self):
    assert (self.mods != None)
    m = Mods(self.mods)
    m.in_new = True
    return m

def expr_to_json(fs, m, vs, e):
  if isinstance(e, syntax.QuantifierExpr):
    assert e.quant in ("FORALL", "EXISTS")
    is_forall = e.quant == "FORALL"
    decls = binder_to_json(e.binder)
    w = dict(vs)
    for v in e.binder.vs:
      w[v.name] = sort_to_json(v.sort)

    body = expr_to_json(fs, m, w, e.body)
    return ["forall" if is_forall else "exists", decls, body]
  elif isinstance(e, syntax.AppExpr):
    so = fs[e.callee]
    if m.in_new and e.callee in m.mods:
      c = ["const", e.callee + "'", so]
    else:
      c = ["const", e.callee, so]
    return ["apply",
      c,
      [expr_to_json(fs, m, vs, arg) for arg in e.args]
    ]
  elif isinstance(e, syntax.Id):
    if e.name in vs:
      return ["var", e.name, vs[e.name]]
    else:
      assert e.name in fs
      if m.in_new and e.name in m.mods:
        return ["const", e.name + "'", fs[e.name]]
      else:
        return ["const", e.name, fs[e.name]]
  elif isinstance(e, syntax.UnaryExpr):
    if e.op == "NOT":
      return ["not", expr_to_json(fs, m, vs, e.arg)]
    elif e.op == "NEW":
      return expr_to_json(fs, m.with_new(), vs, e.arg)
    else:
      print("unary", e.op)
      assert False
  elif isinstance(e, syntax.BinaryExpr):
    if e.op == "IMPLIES":
      return ["implies", expr_to_json(fs, m, vs, e.arg1), expr_to_json(fs, m, vs, e.arg2)]
    elif e.op == "EQUAL":
      return ["eq", expr_to_json(fs, m, vs, e.arg1), expr_to_json(fs, m, vs, e.arg2)]
    elif e.op == "IFF":
      return ["eq", expr_to_json(fs, m, vs, e.arg1), expr_to_json(fs, m, vs, e.arg2)]
    elif e.op == "NOTEQ":
      return ["not", ["eq", expr_to_json(fs, m, vs, e.arg1), expr_to_json(fs, m, vs, e.arg2)]]
    else:
      print("binary", e.op)
      assert False
  elif isinstance(e, syntax.NaryExpr):
    if e.op == "AND":
      return ["and", [expr_to_json(fs, m, vs, a) for a in e.args]]
    if e.op == "OR":
      return ["or", [expr_to_json(fs, m, vs, a) for a in e.args]]
    else:
      print("nary", e.op)
      assert False
  elif isinstance(e, syntax.IfThenElse):
    return ["ite",
        expr_to_json(fs, m, vs, e.branch),
        expr_to_json(fs, m, vs, e.then),
        expr_to_json(fs, m, vs, e.els)
    ]
  else:
    print(type(e))
    print(dir(e))
    assert False

def get_sorts(prog):
  return [sort.name for sort in prog.sorts()]

def sort_to_json(r):
  return ["uninterpretedSort", r.name]

def boolean_sort_json():
  return ["booleanSort"]

def get_functions(prog):
  funcs = []

  for rel in prog.relations():
    dom = [sort_to_json(r) for r in rel.arity]
    rng = boolean_sort_json()
    funcs.append(["const", rel.name, ["functionSort", dom, rng]])

  for c in prog.constants():
    funcs.append(["const", c.name, sort_to_json(c.sort)])

  for f in prog.functions():
    dom = [sort_to_json(r) for r in f.arity]
    rng = sort_to_json(f.sort)
    funcs.append(["const", f.name, ["functionSort", dom, rng]])

  return funcs

def get_fs(prog):
  fs = {}
  for rel in prog.relations():
    dom = [sort_to_json(r) for r in rel.arity]
    rng = boolean_sort_json()
    fs[rel.name] = ["functionSort", dom, rng]

  for c in prog.constants():
    fs[rel.name] = ["const", c.name, sort_to_json(c.sort)]

  for f in prog.functions():
    dom = [sort_to_json(r) for r in f.arity]
    rng = sort_to_json(f.sort)
    fs[f.name] = ["functionSort", dom, rng]

  return fs

def get_axioms(prog):
  fs = get_fs(prog)
  return [expr_to_json(fs, Mods(None), {}, e.expr) for e in prog.axioms()]

def get_inits(prog):
  fs = get_fs(prog)
  return [expr_to_json(fs, Mods(None), {}, e.expr) for e in prog.inits()]

def get_conjs(prog):
  fs = get_fs(prog)
  return [expr_to_json(fs, Mods(None), {}, e.expr) for e in prog.safeties()]

def get_actions(prog):
  fs = get_fs(prog)
  a = {}
  for e in prog.transitions():
    assert (e.num_states == 2)
    decls = binder_to_json(e.binder)
    vs = {v.name : sort_to_json(v.sort) for v in e.binder.vs}
    mod_names = [m.name for m in e.mods]
    m = Mods(mod_names)
    ex = expr_to_json(fs, m, vs, e.expr)

    if len(vs) > 0:
      ex = ["exists", decls, ex]

    a[e.name] = ["relation", mod_names, ex]
  return a

def main():
  filename = sys.argv[1]
  with open(filename) as f:
    contents = f.read()
  prog = parse_program(contents, filename)
  typechecker.typecheck_program(prog)

  actions = get_actions(prog)

  print(json.dumps({
    "sorts" : get_sorts(prog),
    "functions" : get_functions(prog),
    "axioms" : get_axioms(prog),
    "inits" : get_inits(prog),
    "conjectures" : get_conjs(prog),
    "templates" : [],
    "actions" : get_actions(prog),
  }))

  #print(prog.constants)
  #print(prog.decls)
  #print(prog.sorts)
  #print(prog.safeties)
  #print(prog.functions)
  #print(prog.relations)
  #print(prog.inits)

if __name__ == "__main__":
    main()
