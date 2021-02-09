import sys
import json
import os

sys.path.insert(0, os.path.abspath(os.path.join(
    os.path.dirname(__file__), '../ivy')))

from ivy.ivy_init import ivy_init 
from ivy.ivy_compiler import ivy_load_file
from ivy.ivy_logic import AST
from ivy import ivy_module as im
from ivy import ivy_module
from ivy import logic
from ivy import ivy_actions
from ivy import ivy_logic_utils

def get_json():
  with ivy_module.Module():
    with open(sys.argv[1]) as f:
      ivy_load_file(f)

    functions = []
    for func in im.module.functions:
      functions.append(logic_to_obj(func))  

    # XXX hack
    #if sys.argv[1] == "examples/leader-election.ivy":
    #  functions.append(["const", "<=", ["functionSort", [
    #      ["uninterpretedSort", "id"],
    #      ["uninterpretedSort", "id"]],
    #      ["booleanSort"]]])

    axioms = []
    for axiom in im.module.get_axioms():
      #print axiom
      #print logic_to_obj(ivy_logic_utils.close_epr(axiom))
      axioms.append(logic_to_obj(ivy_logic_utils.close_epr(axiom)))

    conjectures = []
    for conj in im.module.conjs:
      for fmla in conj.fmlas:
        #print fmla
        #print logic_to_obj(ivy_logic_utils.close_epr(fmla))
        conjectures.append(logic_to_obj(ivy_logic_utils.close_epr(fmla)))
      #conjectures.append(logic_to_obj(conj.formula))

    templates = []
    for template in im.module.labeled_templates:
      #print template
      #print template.formula
      templates.append(logic_to_obj(template.formula))
      #template = ivy_logic_utils.formula_to_clauses(template)
      #or fmla in template.fmlas:
      # templates.append(logic_to_obj(fmla))

    inits = []
    for fmla in im.module.init_cond.fmlas:
      inits.append(logic_to_obj(ivy_logic_utils.close_epr(fmla)))

    actions = {}
    for name in im.module.actions:
      action = im.module.actions[name]
      actions[name] = action_to_obj(action)

    return json.dumps({
      "sorts": im.module.sort_order,
      "functions": functions,
      "inits": inits,
      "axioms": axioms,
      "conjectures": conjectures, #[c for c in conjectures if not has_wildcard(c)],
      "templates": templates, #[c for c in conjectures if has_wildcard(c)],
      "actions": actions,
    })

def has_wildcard(o):
  if type(o) == dict:
    for k in o:
      if has_wildcard(o[k]):
        return True
    return False
  elif type(o) == list:
    if len(o) == 1:
      return o[0] == '__wild'
    else:
      for k in xrange(0, len(o)):
        if has_wildcard(o[k]):
          return True
      return False
  else:
    return False

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
      logic_to_obj(ivy_logic_utils.close_epr(l.args[0])),
    ]
  elif isinstance(l, ivy_actions.AssumeAction):
    return [
      "assert",
      logic_to_obj(ivy_logic_utils.close_epr(l.args[0])),
    ]
  elif isinstance(l, ivy_actions.AssignAction):
    return [
      "assign",
      logic_to_obj(l.args[0]),
      logic_to_obj(l.args[1]),
    ]
  elif isinstance(l, ivy_actions.HavocAction):
    return [
      "havoc",
      logic_to_obj(l.args[0]),
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

def maybe_merge_nearlys(obj):
  if obj[0] == "nearlyforall" and obj[2][0] == "nearlyforall":
    vs1 = obj[1]
    vs2 = obj[2][1]

    name1 = vs1[0][1]
    name2 = vs2[0][1]

    parts1 = name1.split('_')
    parts2 = name2.split('_')
    assert len(parts1) >= 3
    assert len(parts2) >= 3
    assert parts1[0] == 'Nearly'
    assert parts2[0] == 'Nearly'
    if parts1[1] == parts2[1]:
      return ["nearlyforall", vs1 + vs2, obj[2][2]]
    else:
      return obj
  else:
    return obj

def sort_vars_alphabetically(vs):
  # ["var", "Q", ["uninterpretedSort", "quorum"]]
  return sorted(vs, key = lambda v : (v[2][1], v[1]))

def logic_to_obj(l):
  if isinstance(l, logic.ForAll):
    vs = [v for v in l.variables if v.name != "WILD"]
    if len(vs) > 0:
      the_vars = [logic_to_obj(var) for var in vs]
      is_nearly = the_vars[0][1].startswith("Nearly_")
      name = "nearlyforall" if is_nearly else "forall"
      the_vars = sort_vars_alphabetically(the_vars)
      return maybe_merge_nearlys([name, the_vars, logic_to_obj(l.body)])
    else:
      return logic_to_obj(l.body)
  if isinstance(l, logic.Exists):
    vars = [logic_to_obj(var) for var in l.variables]
    if len(vars) > 0 and vars[0][1] != 'FakeOutHackExists':
      the_vars = [logic_to_obj(var) for var in l.variables]
      the_vars = sort_vars_alphabetically(the_vars)
      return ["exists", the_vars, logic_to_obj(l.body)]
    else:
      return logic_to_obj(l.body)
  elif isinstance(l, logic.Var):
    if l.name == "WILD":
      return ["__wild"]
    else:
      return ["var", l.name, sort_to_obj(l.sort)]
  elif isinstance(l, logic.Const):
    return ["const", l.name, sort_to_obj(l.sort)]
  elif isinstance(l, logic.Implies):
    return ["implies", logic_to_obj(l.t1), logic_to_obj(l.t2)]
  elif isinstance(l, logic.Iff):
    return ["eq", logic_to_obj(l.t1), logic_to_obj(l.t2)]
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
    print l
    print 'logic_to_obj cant do', type(l)
    assert False

def sort_to_obj(s):
  if isinstance(s, logic.BooleanSort):
    return ["booleanSort"]
  elif isinstance(s, logic.UninterpretedSort):
    return ["uninterpretedSort", s.name]
  elif isinstance(s, logic.FunctionSort):
    return ["functionSort", [sort_to_obj(t) for t in s.domain], sort_to_obj(s.range)]
  else:
    print 'sort_to_obj cant do', type(s)
    assert False

def fmla_to_obj(fmla):
  assert isinstance(fmla, AST)
  return [type(fmla).__name__] + [fmla_to_obj(arg) for arg in fmla.args]

def main():
  print get_json()

if __name__ == "__main__":
    main()
