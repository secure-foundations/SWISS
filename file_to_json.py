import sys
import json
import os

sys.path.append(os.path.abspath(os.path.join(
    os.path.dirname(__file__), 'ivy')))

from ivy.ivy_init import ivy_init 
from ivy.ivy_compiler import ivy_load_file
from ivy.ivy_logic import AST
from ivy import ivy_module as im
from ivy import ivy_module
from ivy import logic
from ivy import ivy_actions

def main():
  with ivy_module.Module():
    with open(sys.argv[1]) as f:
      ivy_load_file(f)

    functions = []
    for func in im.module.functions:
      functions.append(logic_to_obj(func))  

    axioms = []
    for axiom in im.module.get_axioms():
      axioms.append(logic_to_obj(axiom))

    conjectures = []
    for conj in im.module.conjs:
      for fmla in conj.fmlas:
        conjectures.append(logic_to_obj(fmla))

    inits = []
    for fmla in im.module.init_cond.fmlas:
      inits.append(logic_to_obj(fmla))

    actions = {}
    for name in im.module.actions:
      action = im.module.actions[name]
      actions[name] = action_to_obj(action)

    print json.dumps({
      "sorts": im.module.sort_order,
      "functions": functions,
      "inits": inits,
      "axioms": axioms,
      "conjectures": conjectures,
      "actions": actions,
    })

def action_to_obj(l):
  if isinstance(l, ivy_actions.LocalAction):
    return [
      "localAction",
      [logic_to_obj(arg) for arg in l.args[0:-1]],
      action_to_obj(l.args[-1]),
    ]
  elif isinstance(l, ivy_actions.Sequence):
    return [
      "sequence",
      [action_to_obj(arg) for arg in l.args],
    ]
  elif isinstance(l, ivy_actions.AssumeAction):
    return [
      "assume",
      logic_to_obj(l.args[0]),
    ]
  elif isinstance(l, ivy_actions.AssignAction):
    return [
      "assign",
      logic_to_obj(l.args[0]),
      logic_to_obj(l.args[1]),
    ]
  elif isinstance(l, ivy_actions.IfAction):
    if len(l.args) >= 3:
      return [
        "ifelse",
        logic_to_obj(l.args[0]),
        action_to_obj(l.args[1]),
        action_to_obj(l.args[2]),
      ]
    else:
      return [
        "if",
        logic_to_obj(l.args[0]),
        action_to_obj(l.args[1]),
      ]
  else:
    print 'action_to_obj failed', type(l)
    assert False

def logic_to_obj(l):
  if isinstance(l, logic.ForAll):
    return ["forall", [logic_to_obj(var) for var in l.variables], logic_to_obj(l.body)]
  if isinstance(l, logic.Exists):
    return ["exists", [logic_to_obj(var) for var in l.variables], logic_to_obj(l.body)]
  elif isinstance(l, logic.Var):
    return ["var", l.name, sort_to_obj(l.sort)]
  elif isinstance(l, logic.Const):
    return ["const", l.name, sort_to_obj(l.sort)]
  elif isinstance(l, logic.Implies):
    return ["implies", logic_to_obj(l.t1), logic_to_obj(l.t2)]
  elif isinstance(l, logic.Eq):
    return ["eq", logic_to_obj(l.t1), logic_to_obj(l.t2)]
  elif isinstance(l, logic.Not):
    return ["not", logic_to_obj(l.body)]
  elif isinstance(l, logic.Apply):
    return ["apply", logic_to_obj(l.func), [logic_to_obj(term) for term in l.terms]]
  elif isinstance(l, logic.And):
    return ["and", [logic_to_obj(o) for o in l]]
  elif isinstance(l, logic.Or):
    return ["or", [logic_to_obj(o) for o in l]]
  else:
    print 'cant do', type(l)
    assert False

def sort_to_obj(s):
  if isinstance(s, logic.BooleanSort):
    return ["booleanSort"]
  elif isinstance(s, logic.UninterpretedSort):
    return ["uninterpretedSort", s.name]
  elif isinstance(s, logic.FunctionSort):
    return ["functionSort", [sort_to_obj(t) for t in s.domain], sort_to_obj(s.range)]
  else:
    print 'cant do', type(s)
    assert False

def fmla_to_obj(fmla):
  assert isinstance(fmla, AST)
  return [type(fmla).__name__] + [fmla_to_obj(arg) for arg in fmla.args]

if __name__ == "__main__":
    main()

