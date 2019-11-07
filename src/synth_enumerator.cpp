#include "synth_enumerator.h"

#include "enumerator.h"
#include "quantifier_permutations.h"
#include "benchmarking.h"
#include "sketch_model.h"

#define DO_FORALL_PRUNING true

using namespace std;

class SatCandidateSolver : public CandidateSolver {
public:
  SatCandidateSolver(std::shared_ptr<Module>, Options const&,
      bool ensure_nonredundant, Shape shape);

  value getNext();
  void addCounterexample(Counterexample cex, value candidate);
  void addExistingInvariant(value inv);

private:
  std::vector<std::pair<Counterexample, value>> cexes;
  std::vector<value> existingInvariants;

  std::shared_ptr<Module> module;
  Shape shape;
  Options options;
  bool ensure_nonredundant;

  TopQuantifierDesc tqd;
  SatSolver ss;
  SketchFormula sf;

  void init_constraints();
};

std::shared_ptr<CandidateSolver> make_sat_candidate_solver(
    std::shared_ptr<Module> module, Options const& options,
      bool ensure_nonredundant, Shape shape)
{
  return shared_ptr<CandidateSolver>(new SatCandidateSolver(module, options, ensure_nonredundant, shape));
}

sat_expr is_something(shared_ptr<Module> module, SketchFormula& sf, shared_ptr<Model> model,
    bool do_true) {
  assert(false && "not implement with NearlyForall in mind");

  vector<VarDecl> quantifiers = sf.free_vars;
  vector<size_t> domain_sizes;
  for (VarDecl const& decl : quantifiers) {
    domain_sizes.push_back(model->get_domain_size(decl.sort));
  }
  vector<sat_expr> vec;
  
  vector<object_value> args;
  args.resize(domain_sizes.size());
  while (true) {
    sat_expr e = sf.interpret(model, args, do_true);

    int i;
    for (i = 0; i < domain_sizes.size(); i++) {
      args[i]++;
      if (args[i] == domain_sizes[i]) {
        args[i] = 0;
      } else {
        break;
      }
    }
    if (i == domain_sizes.size()) {
      break;
    }
  }

  return do_true ? sat_and(vec) : sat_or(vec);
}

sat_expr assert_true_for_some_qs(
    shared_ptr<Module> module, SketchFormula& sf, shared_ptr<Model> model,
    value candidate) {
  vector<QuantifierInstantiation> qis;
  bool evals_true = get_multiqi_counterexample(model, candidate, qis);
  assert(!evals_true);
  assert(qis.size() > 0);

  vector<sat_expr> vec;

  vector<vector<object_value>> variable_values;
  for (QuantifierInstantiation& qi : qis) {
    variable_values.push_back(qi.variable_values);
  }
  vector<vector<vector<object_value>>> all_perms = get_multiqi_quantifier_permutations(
      sf.tqd, variable_values);

  //printf("using %d instantiations\n", (int)all_perms.size());

  for (vector<vector<object_value>> const& ovs_many : all_perms) {
    vector<sat_expr> one_is_true;
    for (vector<object_value> const& ovs : ovs_many) {
      one_is_true.push_back(sf.interpret(model, ovs, true));
    }
    vec.push_back(sat_or(one_is_true));
  }

  return sat_and(vec);
}


sat_expr is_true(shared_ptr<Module> module, SketchFormula& sf, shared_ptr<Model> model,
    value candidate) {
  if (DO_FORALL_PRUNING) {
    return assert_true_for_some_qs(module, sf, model, candidate);
  } else {
    return is_something(module, sf, model, true);
  }
}

sat_expr is_false(shared_ptr<Module> module, SketchFormula& sf, shared_ptr<Model> model) {
  //return is_something(module, sf, model, false);
  return sf.interpret_not(model);
}


void add_counterexample(shared_ptr<Module> module, SketchFormula& sf, Counterexample cex,
      value candidate)
{
  if (cex.is_true) {
    sf.solver.add(is_true(module, sf, cex.is_true, candidate));
  }
  else if (cex.is_false) {
    sf.solver.add(is_false(module, sf, cex.is_false));
  }
  else if (cex.hypothesis && cex.conclusion) {
    //cex.hypothesis->dump();
    //cex.conclusion->dump();
    vector<sat_expr> vec;
    vec.push_back(is_false(module, sf, cex.hypothesis));
    vec.push_back(is_true(module, sf, cex.conclusion, candidate));
    sf.solver.add(sat_or(vec));
  }
  else {
    assert(false);
  }
}

vector<pair<Counterexample, value>> filter_unneeded_cexes(
    vector<pair<Counterexample, value>> const& cexes,
    value invariant_so_far)
{
  vector<pair<Counterexample, value>> res;
  for (auto p : cexes) {
    Counterexample cex = p.first;
    if (cex.is_true ||
        (cex.hypothesis && cex.hypothesis->eval_predicate(invariant_so_far))) {
      res.push_back(p);
    }
  }
  return res;
}

SatCandidateSolver::SatCandidateSolver(shared_ptr<Module> module, Options const& options, bool ensure_nonredundant, Shape shape)
  : module(module)
  , shape(shape)
  , options(options)
  , ensure_nonredundant(ensure_nonredundant)
  , tqd(module->templates[0])
  , sf(ss, tqd, module, options.arity, options.depth)
{
  init_constraints();
}

value SatCandidateSolver::getNext()
{
  //Benchmarking bench;
  //bench.start("solver (" + to_string(num_iterations) + ")");
  bool res = ss.is_sat();
  //bench.end();
  //bench.dump();

  if (!res) {
    return nullptr;
  }

  value candidate_inner = sf.to_value();
  value candidate = fill_holes_in_value(module->templates[0], {candidate_inner});
  candidate = candidate->simplify();
  return candidate;
}

void SatCandidateSolver::addCounterexample(Counterexample cex, value candidate)
{
  cexes.push_back(make_pair(cex, candidate));
  add_counterexample(module, sf, cex, candidate);
}

void SatCandidateSolver::addExistingInvariant(value inv)
{
  existingInvariants.push_back(inv);
  cexes = filter_unneeded_cexes(cexes, v_and(existingInvariants));

  // re-initialize the SatSolver
  ss.~SatSolver();
  new (&ss) SatSolver();
  sf.~SketchFormula();
  new (&sf) SketchFormula(ss, tqd, module, options.arity, options.depth);
  init_constraints();

  printf("Starting off with %d counterexamples\n", (int)cexes.size());
  for (auto p : cexes) {
    Counterexample cex = p.first;
    value candidate = p.second;
    add_counterexample(module, sf, cex, candidate);
  }
}

void SatCandidateSolver::init_constraints()
{
  switch (shape) {
    case Shape::SHAPE_DISJ:
      sf.constrain_disj_form();
      break;
    case Shape::SHAPE_CONJ_DISJ:
      sf.constrain_conj_disj_form();
      break;
    default:
      assert(false);
  }

  if (ensure_nonredundant) {
    SketchModel sm(ss, module, 3);
    ss.add(sf.interpret_not(sm));
    for (value v : existingInvariants) {
      sm.assert_formula(v);
    }
  }
}
