import z3 # pip3 install z3-solver

import sys
import json
import os
import argparse
from typing import cast

sys.path.insert(0, os.path.abspath(os.path.join(
    os.path.dirname(__file__), '../mypyvy/src')))

import mypyvy
import utils
import parser
#import typechecker
import syntax

"""
def parse_program(input: str, force_rebuild: bool = False, filename: Optional[str] = None) -> Program:
    l = parser.get_lexer()
    p = parser.get_parser(forbid_rebuild=force_rebuild)
    return p.parse(input=input, lexer=l, filename=filename)


def parse_args(args: List[str]) -> utils.MypyvyArgs:
    argparser = argparse.ArgumentParser()

    subparsers = argparser.add_subparsers(title='subcommands', dest='subcommand')
    all_subparsers = []

    verify_subparser = subparsers.add_parser('verify', help='verify that the invariants are inductive')
    verify_subparser.set_defaults(main=verify)
    all_subparsers.append(verify_subparser)

    updr_subparser = subparsers.add_parser('updr', help='search for a strengthening that proves the invariant named by the --safety=NAME flag')
    updr_subparser.set_defaults(main=do_updr)
    all_subparsers.append(updr_subparser)

    bmc_subparser = subparsers.add_parser('bmc', help='bounded model check to depth given by the --depth=DEPTH flag for property given by the --safety=NAME flag')
    bmc_subparser.set_defaults(main=bmc)
    all_subparsers.append(bmc_subparser)

    theorem_subparser = subparsers.add_parser('theorem', help='check state-independent theorems about the background axioms of a model')
    theorem_subparser.set_defaults(main=theorem)
    all_subparsers.append(theorem_subparser)

    trace_subparser = subparsers.add_parser('trace', help='search for concrete executions that satisfy query described by the file\'s trace declaration')
    trace_subparser.set_defaults(main=trace)
    all_subparsers.append(trace_subparser)

    generate_parser_subparser = subparsers.add_parser('generate-parser', help='internal command used by benchmarking infrastructure to avoid certain race conditions')
    generate_parser_subparser.set_defaults(main=nop)  # parser is generated implicitly by main when it parses the program
    all_subparsers.append(generate_parser_subparser)

    typecheck_subparser = subparsers.add_parser('typecheck', help='typecheck the file, report any errors, and exit')
    typecheck_subparser.set_defaults(main=nop)  # program is always typechecked; no further action required
    all_subparsers.append(typecheck_subparser)

    relax_subparser = subparsers.add_parser('relax', help='produce a version of the file that is "relaxed", in a way that is indistinguishable for universal invariants')
    relax_subparser.set_defaults(main=relax)
    all_subparsers.append(relax_subparser)

    all_subparsers += pd.add_argparsers(subparsers)
    all_subparsers += pd_fol.add_argparsers(subparsers)

    for s in all_subparsers:
        s.add_argument('--forbid-parser-rebuild', action=utils.YesNoAction, default=False,
                       help='force loading parser from disk (helps when running mypyvy from multiple processes)')
        s.add_argument('--log', default='warning', choices=['error', 'warning', 'info', 'debug'],
                       help='logging level')
        s.add_argument('--log-time', action=utils.YesNoAction, default=False,
                       help='make each log message include current time')
        s.add_argument('--log-xml', action=utils.YesNoAction, default=False,
                       help='log in XML format')
        s.add_argument('--seed', type=int, default=0, help="value for z3's smt.random_seed")
        s.add_argument('--print-program-repr', action=utils.YesNoAction, default=False,
                       help='print a machine-readable representation of the program after parsing')
        s.add_argument('--print-program', action=utils.YesNoAction, default=False,
                       help='print the program after parsing')
        s.add_argument('--key-prefix',
                       help='additional string to use in front of names sent to z3')
        s.add_argument('--minimize-models', action=utils.YesNoAction, default=True,
                       help='search for models with minimal cardinality')
        s.add_argument('--timeout', type=int, default=None,
                       help='z3 timeout (milliseconds)')
        s.add_argument('--exit-on-error', action=utils.YesNoAction, default=False,
                       help='exit after reporting first error')
        s.add_argument('--ipython', action=utils.YesNoAction, default=False,
                       help='run IPython with s and prog at the end')
        s.add_argument('--error-filename-basename', action=utils.YesNoAction, default=False,
                       help='print only the basename of the input file in error messages')
        s.add_argument('--query-time', action=utils.YesNoAction, default=True,
                       help='report how long various z3 queries take')
        s.add_argument('--print-counterexample', action=utils.YesNoAction, default=True,
                       help='print counterexamples')
        s.add_argument('--print-cmdline', action=utils.YesNoAction, default=True,
                       help='print the command line passed to mypyvy')
        s.add_argument('--clear-cache', action=utils.YesNoAction, default=False,
                       help='do not load from cache, but dump to cache as usual (effectively clearing the cache before starting)')
        s.add_argument('--clear-cache-memo', action=utils.YesNoAction, default=False,
                       help='load only discovered states from the cache, but dump to cache as usual (effectively clearing the memoization cache before starting, while keeping discovered states and transitions)')
        s.add_argument('--cache-only', action=utils.YesNoAction, default=False,
                       help='assert that the caches already contain all the answers')
        s.add_argument('--cache-only-discovered', action=utils.YesNoAction, default=False,
                       help='assert that the discovered states already contain all the answers')
        s.add_argument('--print-exit-code', action=utils.YesNoAction, default=False,
                       help='print the exit code before exiting (good for regression testing)')

        s.add_argument('--cvc4', action='store_true',
                       help='use CVC4 as the backend solver. this is not very well supported.')

        # for diagrams:
        s.add_argument('--simplify-diagram', action=utils.YesNoAction,
                       default=(s is updr_subparser),
                       default_description='yes for updr, else no',
                       help='in diagram generation, substitute existentially quantified variables that are equal to constants')
        s.add_argument('--diagrams-subclause-complete', action=utils.YesNoAction, default=False,
                       help='in diagram generation, "complete" the diagram so that every stronger '
                            'clause is a subclause')

    updr_subparser.add_argument('--use-z3-unsat-cores', action=utils.YesNoAction, default=True,
                                help='generalize diagrams using brute force instead of unsat cores')
    updr_subparser.add_argument('--smoke-test', action=utils.YesNoAction, default=False,
                                help='(for debugging mypyvy itself) run bmc to confirm every conjunct added to a frame')
    updr_subparser.add_argument('--assert-inductive-trace', action=utils.YesNoAction, default=False,
                                help='(for debugging mypyvy itself) check that frames are always inductive')

    updr_subparser.add_argument('--sketch', action=utils.YesNoAction, default=False,
                                help='use sketched invariants as additional safety (currently only in automaton)')

    updr_subparser.add_argument('--automaton', action=utils.YesNoAction, default=False,
                                help='whether to run vanilla UPDR or phase UPDR')
    updr_subparser.add_argument('--block-may-cexs', action=utils.YesNoAction, default=False,
                                help="treat failures to push as additional proof obligations")
    updr_subparser.add_argument('--push-frame-zero', default='if_trivial', choices=['if_trivial', 'always', 'never'],
                                help="push lemmas from the initial frame: always/never/if_trivial, the latter is when there is more than one phase")

    verify_subparser.add_argument('--automaton', default='yes', choices=['yes', 'no', 'only'],
                                  help="whether to use phase automata during verification. by default ('yes'), both non-automaton "
                                  "and automaton proofs are checked. 'no' means ignore automaton proofs. "
                                  "'only' means ignore non-automaton proofs.")
    verify_subparser.add_argument('--check-transition', default=None, nargs='+',
                                  help="when verifying inductiveness, check only these transitions")
    verify_subparser.add_argument('--check-invariant', default=None, nargs='+',
                                  help="when verifying inductiveness, check only these invariants")
    verify_subparser.add_argument('--json', action='store_true',
                                  help="output machine-parseable verification results in JSON format")
    verify_subparser.add_argument('--smoke-test-solver', action=utils.YesNoAction, default=False,
                                help='(for debugging mypyvy itself) double check countermodels by evaluation')

    updr_subparser.add_argument('--checkpoint-in',
                                help='start from internal state as stored in given file')
    updr_subparser.add_argument('--checkpoint-out',
                                help='store internal state to given file') # TODO: say when


    bmc_subparser.add_argument('--safety', help='property to check')
    bmc_subparser.add_argument('--depth', type=int, default=3, metavar='N',
                               help='number of steps to check')

    argparser.add_argument('filename')

    return cast(utils.MypyvyArgs, argparser.parse_args(args))
"""


"""
def parse_program(input, filename = None):
    l = parser.get_lexer()
    p = parser.get_parser(forbid_rebuild=False)
    prog = p.parse(input=input, lexer=l, filename=filename)
    prog.input = input
    return prog

# copied from mypyvy/src/mypyvy.py
def parse_args(args):
    argparser = argparse.ArgumentParser()

    subparsers = argparser.add_subparsers(title='subcommands', dest='subcommand')
    all_subparsers = []

    verify_subparser = subparsers.add_parser('verify', help='verify that the invariants are inductive')
    #verify_subparser.set_defaults(main=verify)
    all_subparsers.append(verify_subparser)

    updr_subparser = subparsers.add_parser(
        'updr',
        help='search for a strengthening that proves the invariant named by the --safety=NAME flag')
    #updr_subparser.set_defaults(main=do_updr)
    all_subparsers.append(updr_subparser)

    bmc_subparser = subparsers.add_parser(
        'bmc',
        help='bounded model check to depth given by the --depth=DEPTH flag '
             'for property given by the --safety=NAME flag')
    #bmc_subparser.set_defaults(main=bmc)
    all_subparsers.append(bmc_subparser)

    theorem_subparser = subparsers.add_parser(
        'theorem',
        help='check state-independent theorems about the background axioms of a model')
    #theorem_subparser.set_defaults(main=theorem)
    all_subparsers.append(theorem_subparser)

    trace_subparser = subparsers.add_parser(
        'trace',
        help='search for concrete executions that satisfy query described by the file\'s trace declaration')
    #trace_subparser.set_defaults(main=trace)
    all_subparsers.append(trace_subparser)

    generate_parser_subparser = subparsers.add_parser(
        'generate-parser',
        help='internal command used by benchmarking infrastructure to avoid certain race conditions')
    # parser is generated implicitly by main when it parses the program, so we can just nop here
    #generate_parser_subparser.set_defaults(main=nop)
    all_subparsers.append(generate_parser_subparser)

    typecheck_subparser = subparsers.add_parser('typecheck', help='typecheck the file, report any errors, and exit')
    #typecheck_subparser.set_defaults(main=nop)  # program is always typechecked; no further action required
    all_subparsers.append(typecheck_subparser)

    relax_subparser = subparsers.add_parser(
        'relax',
        help='produce a version of the file that is "relaxed", '
             'in a way that is indistinguishable for universal invariants')
    #relax_subparser.set_defaults(main=relax)
    all_subparsers.append(relax_subparser)

    check_one_bounded_width_invariant_parser = subparsers.add_parser(
        'check-one-bounded-width-invariant',
        help='popl'
    )
    #check_one_bounded_width_invariant_parser.set_defaults(main=check_one_bounded_width_invariant)
    all_subparsers.append(check_one_bounded_width_invariant_parser)

    #all_subparsers += pd.add_argparsers(subparsers)

    #all_subparsers += rethink.add_argparsers(subparsers)

    #all_subparsers += sep.add_argparsers(subparsers)

    for s in all_subparsers:
        s.add_argument('--forbid-parser-rebuild', action=utils.YesNoAction, default=False,
                       help='force loading parser from disk (helps when running mypyvy from multiple processes)')
        s.add_argument('--log', default='warning', choices=['error', 'warning', 'info', 'debug'],
                       help='logging level')
        s.add_argument('--log-time', action=utils.YesNoAction, default=False,
                       help='make each log message include current time')
        s.add_argument('--log-xml', action=utils.YesNoAction, default=False,
                       help='log in XML format')
        s.add_argument('--seed', type=int, default=0, help="value for z3's smt.random_seed")
        s.add_argument('--print-program',
                       choices=['str', 'repr', 'faithful', 'without-invariants'],
                       help='print program after parsing using given strategy')
        s.add_argument('--key-prefix',
                       help='additional string to use in front of names sent to z3')
        s.add_argument('--minimize-models', action=utils.YesNoAction, default=True,
                       help='search for models with minimal cardinality')
        s.add_argument('--timeout', type=int, default=None,
                       help='z3 timeout (milliseconds)')
        s.add_argument('--exit-on-error', action=utils.YesNoAction, default=False,
                       help='exit after reporting first error')
        s.add_argument('--ipython', action=utils.YesNoAction, default=False,
                       help='run IPython with s and prog at the end')
        s.add_argument('--error-filename-basename', action=utils.YesNoAction, default=False,
                       help='print only the basename of the input file in error messages')
        s.add_argument('--query-time', action=utils.YesNoAction, default=True,
                       help='report how long various z3 queries take')
        s.add_argument('--print-counterexample', action=utils.YesNoAction, default=True,
                       help='print counterexamples')
        s.add_argument('--print-negative-tuples', action=utils.YesNoAction, default=False,
                       help='print negative counterexamples')
        s.add_argument('--print-cmdline', action=utils.YesNoAction, default=True,
                       help='print the command line passed to mypyvy')
        s.add_argument('--clear-cache', action=utils.YesNoAction, default=False,
                       help='do not load from cache, but dump to cache as usual '
                            '(effectively clearing the cache before starting)')
        s.add_argument('--clear-cache-memo', action=utils.YesNoAction, default=False,
                       help='load only discovered states from the cache, but dump to cache as usual '
                            '(effectively clearing the memoization cache before starting, '
                            'while keeping discovered states and transitions)')
        s.add_argument('--cache-only', action=utils.YesNoAction, default=False,
                       help='assert that the caches already contain all the answers')
        s.add_argument('--cache-only-discovered', action=utils.YesNoAction, default=False,
                       help='assert that the discovered states already contain all the answers')
        s.add_argument('--print-exit-code', action=utils.YesNoAction, default=False,
                       help='print the exit code before exiting (good for regression testing)')
        s.add_argument('--exit-0', action=utils.YesNoAction, default=False,
                       help='always exit with status 0 (good for testing)')

        s.add_argument('--cvc4', action='store_true',
                       help='use CVC4 as the backend solver. this is not very well supported.')

        s.add_argument('--smoke-test-solver', action=utils.YesNoAction, default=False,
                       help='(for debugging mypyvy itself) double check countermodels by evaluation')

        # for diagrams:
        s.add_argument('--simplify-diagram', action=utils.YesNoAction,
                       default=(s is updr_subparser),
                       default_description='yes for updr, else no',
                       help='in diagram generation, substitute existentially quantified variables '
                            'that are equal to constants')

    updr_subparser.add_argument('--use-z3-unsat-cores', action=utils.YesNoAction, default=True,
                                help='generalize using unsat cores rather than brute force')
    updr_subparser.add_argument('--assert-inductive-trace', action=utils.YesNoAction, default=False,
                                help='(for debugging mypyvy itself) check that frames are always inductive')

    verify_subparser.add_argument('--check-transition', default=None, nargs='+',
                                  help="when verifying inductiveness, check only these transitions")
    verify_subparser.add_argument('--check-invariant', default=None, nargs='+',
                                  help="when verifying inductiveness, check only these invariants")
    verify_subparser.add_argument('--json', action='store_true',
                                  help="output machine-parseable verification results in JSON format")

    updr_subparser.add_argument('--checkpoint-in',
                                help='start from internal state as stored in given file')
    updr_subparser.add_argument('--checkpoint-out',
                                help='store internal state to given file')  # TODO: say when

    bmc_subparser.add_argument('--safety', help='property to check')
    bmc_subparser.add_argument('--depth', type=int, default=3, metavar='N',
                               help='number of steps to check')
    bmc_subparser.add_argument('--relax', action=utils.YesNoAction, default=False,
                               help='relaxed semantics (domain can decrease)')

    argparser.add_argument('filename')

    #return cast(utils.MypyvyArgs, argparser.parse_args(args))
    return cast(utils.MypyvyArgs, argparser.parse_args(args))
"""

def binder_to_json(binder):
  d = []
  for v in binder.vs:
    d.append(["var", v.name, sort_to_json(v.sort)])
  return d

class Mods(object):
  def __init__(self, mods):
    self.mods = mods
    self.in_old = False
  def with_old(self):
    assert (self.mods != None)
    m = Mods(self.mods)
    m.in_old = True
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
    if (not m.in_old) and m.mods != None and e.callee in m.mods:
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
      if (not m.in_old) and m.mods != None and e.name in m.mods:
        return ["const", e.name + "'", fs[e.name]]
      else:
        return ["const", e.name, fs[e.name]]
  elif isinstance(e, syntax.UnaryExpr):
    if e.op == "NOT":
      return ["not", expr_to_json(fs, m, vs, e.arg)]
    elif e.op == "OLD":
      return expr_to_json(fs, m.with_old(), vs, e.arg)
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

  for f in prog.relations_constants_and_functions():
    if isinstance(f, syntax.RelationDecl):
      dom = [sort_to_json(r) for r in f.arity]
      rng = boolean_sort_json()
      funcs.append(["const", f.name, ["functionSort", dom, rng]])
    elif isinstance(f, syntax.ConstantDecl):
      funcs.append(["const", f.name, sort_to_json(f.sort)])
    elif isinstance(f, syntax.FunctionDecl):
      dom = [sort_to_json(r) for r in f.arity]
      rng = sort_to_json(f.sort)
      funcs.append(["const", f.name, ["functionSort", dom, rng]])

  return funcs

def get_fs(prog):
  fs = {}

  for f in prog.relations_constants_and_functions():
    if isinstance(f, syntax.RelationDecl):
      dom = [sort_to_json(r) for r in f.arity]
      rng = boolean_sort_json()
      fs[f.name] = ["functionSort", dom, rng]
    elif isinstance(f, syntax.ConstantDecl):
      fs[f.name] = sort_to_json(f.sort)
    elif isinstance(f, syntax.FunctionDecl):
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
    #assert (e.num_states == 2)
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

  utils.args = mypyvy.parse_args(['typecheck', filename])

  with open(filename) as f:
    contents = f.read()
  prog = mypyvy.parse_program(contents, filename)
  prog.resolve()
  #typechecker.typecheck_program(prog)

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
