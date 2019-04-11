#include "synth_loop.h"

#include <iostream>

#include "z3++.h"
#include "model.h"
#include "sketch.h"
#include "enumerator.h"
#include "benchmarking.h"
#include "bmc.h"
#include "quantifier_permutations.h"

using namespace std;

#define DO_FORALL_PRUNING true

struct Counterexample {
  // is_true
  shared_ptr<Model> is_true;

  // not (is_false)
  shared_ptr<Model> is_false;

  // hypothesis ==> conclusion
  shared_ptr<Model> hypothesis;
  shared_ptr<Model> conclusion;

  bool none;
};

Counterexample get_bmc_counterexample(
    BMCContext& bmc,
    value candidate,
    bool minimal)
{
  shared_ptr<Model> model = bmc.get_k_invariance_violation(candidate, minimal);
  if (model) {
    printf("counterexample type: INIT (after some steps)\n");
    Counterexample cex;
    cex.is_true = model;
    return cex;
  } else {
    Counterexample cex;
    cex.none = true;
    return cex;
  }
}

Counterexample get_counterexample_simple(
    shared_ptr<Module> module,
    BMCContext& bmc,
    shared_ptr<InitContext> initctx,
    shared_ptr<InductionContext> indctx,
    shared_ptr<ConjectureContext> conjctx,
    value candidate)
{
  bool use_minimal = true;
  bool use_minimal_only_for_safety = true;
  bool use_bmc = true;

  Counterexample cex;
  cex.none = false;

  z3::solver& init_solver = initctx->ctx->solver;
  init_solver.push();
  init_solver.add(initctx->e->value2expr(v_not(candidate)));
  z3::check_result init_res = init_solver.check();

  if (init_res == z3::sat) {
    if (use_minimal) {
      cex.is_true = Model::extract_minimal_models_from_z3(
          initctx->ctx->ctx,
          init_solver, module, {initctx->e})[0];
    } else {
      cex.is_true = Model::extract_model_from_z3(
          initctx->ctx->ctx,
          init_solver, module, *initctx->e);
    }

    printf("counterexample type: INIT\n");
    //cex.is_true->dump();

    init_solver.pop();
    return cex;
  } else {
    init_solver.pop();
  }

  z3::solver& conj_solver = conjctx->ctx->solver;
  conj_solver.push();
  conj_solver.add(conjctx->e->value2expr(candidate));
  z3::check_result conj_res = conj_solver.check();

  if (conj_res == z3::sat) {
    if (use_minimal || use_minimal_only_for_safety) {
      cex.is_false = Model::extract_minimal_models_from_z3(
          conjctx->ctx->ctx,
          conj_solver, module, {conjctx->e})[0];
    } else {
      cex.is_false = Model::extract_model_from_z3(
          conjctx->ctx->ctx,
          conj_solver, module, *conjctx->e);
    }

    printf("counterexample type: SAFETY\n");
    //cex.is_false->dump();

    conj_solver.pop();
    return cex;
  } else {
    conj_solver.pop();
  }

  z3::solver& solver = indctx->ctx->solver;
  solver.push();
  solver.add(indctx->e1->value2expr(candidate));
  solver.add(indctx->e2->value2expr(v_not(candidate)));
  z3::check_result res = solver.check();

  if (res == z3::sat) {
    if (use_minimal) {
      auto ms = Model::extract_minimal_models_from_z3(
          indctx->ctx->ctx, solver, module, {indctx->e1, indctx->e2});
      cex.hypothesis = ms[0];
      cex.conclusion = ms[1];
    } else {
      cex.hypothesis = Model::extract_model_from_z3(
          indctx->ctx->ctx, solver, module, *indctx->e1);
      cex.conclusion = Model::extract_model_from_z3(
          indctx->ctx->ctx, solver, module, *indctx->e2);
    }

    solver.pop();

    if (use_bmc) {
      Counterexample bmc_cex = get_bmc_counterexample(bmc, candidate, use_minimal);
      if (!bmc_cex.none) {
        return bmc_cex;
      }
    }

    printf("counterexample type: INDUCTIVE\n");

    return cex;
  }

  cex.none = true;
  return cex;
}

Counterexample get_counterexample(
    shared_ptr<Module> module,
    shared_ptr<InitContext> initctx,
    shared_ptr<InductionContext> indctx,
    shared_ptr<ConjectureContext> conjctx,
    value candidate)
{
  Counterexample cex;
  cex.none = false;

  z3::solver& init_solver = initctx->ctx->solver;
  init_solver.push();
  init_solver.add(initctx->e->value2expr(v_not(candidate)));
  z3::check_result init_res = init_solver.check();

  if (init_res == z3::sat) {
    cex.is_true = Model::extract_minimal_models_from_z3(
        initctx->ctx->ctx,
        init_solver, module, {initctx->e})[0];

    printf("counterexample type: INIT\n");
    //cex.is_true->dump();

    init_solver.pop();
    return cex;
  } else {
    init_solver.pop();
  }

  /*
  z3::solver& conj_solver = conjctx->ctx->solver;
  conj_solver.push();
  conj_solver.add(conjctx->e->value2expr(candidate));
  z3::check_result conj_res = conj_solver.check();

  if (conj_res == z3::sat) {
    cex.is_false = Model::extract_model_from_z3(
        conjctx->ctx->ctx,
        conj_solver, module, *conjctx->e);

    printf("counterexample type: SAFETY\n");
    //cex.is_false->dump();

    conj_solver.pop();
    return cex;
  } else {
    conj_solver.pop();
  }
  */

  value full_conj = v_and(module->conjectures);
  value full_candidate = v_and({candidate, full_conj});

  z3::solver& solver = indctx->ctx->solver;
  solver.push();
  solver.add(indctx->e1->value2expr(full_candidate));
  solver.add(indctx->e2->value2expr(v_not(full_candidate)));
  z3::check_result res = solver.check();

  if (res == z3::sat) {
    auto m1_and_m2 = Model::extract_minimal_models_from_z3(
          indctx->ctx->ctx,
          solver, module, {indctx->e1, indctx->e2});
    shared_ptr<Model> m1 = m1_and_m2[0];
    shared_ptr<Model> m2 = m1_and_m2[1];
    solver.pop();

    if (!m2->eval_predicate(full_conj)) {
      printf("counterexample type: SAFETY\n");
      cex.is_false = m1;
    } else {
      /*
      Counterexample bmc_cex = get_bmc_counterexample(bmc, candidate);
      if (!bmc_cex.none) {
        return bmc_cex;
      }
      */

      printf("counterexample type: INDUCTIVE\n");
      cex.hypothesis = m1;
      cex.conclusion = m2;
    }

    return cex;
  }

  cex.none = true;
  return cex;
}

vector<VarDecl> get_quantifiers(value v) {
  vector<VarDecl> res;
  while (true) {
    if (Forall* f = dynamic_cast<Forall*>(v.get())) {
      for (VarDecl d : f->decls) {
        res.push_back(d);
      }
      v = f->body;
    } else {
      break;
    }
  }
  return res;
}

z3::expr is_something(shared_ptr<Module> module, SketchFormula& sf, shared_ptr<Model> model,
    bool do_true) {
  z3::context& ctx = sf.ctx;
  vector<VarDecl> quantifiers = get_quantifiers(module->templates[0]);
  vector<size_t> domain_sizes;
  for (VarDecl const& decl : quantifiers) {
    domain_sizes.push_back(model->get_domain_size(decl.sort));
  }
  z3::expr_vector vec(ctx);
  
  vector<object_value> args;
  args.resize(domain_sizes.size());
  while (true) {
    z3::expr e = sf.interpret(model, args);
    vec.push_back(do_true ? e : !e);

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

  return do_true ? z3::mk_and(vec) : z3::mk_or(vec);
}

z3::expr assert_true_for_some_qs(
    shared_ptr<Module> module, SketchFormula& sf, shared_ptr<Model> model,
    value candidate) {
  vector<VarDecl> quantifiers = get_quantifiers(module->templates[0]);
  QuantifierInstantiation qi = get_counterexample(model, candidate);
  assert(qi.non_null);
  assert(qi.decls.size() == quantifiers.size());

  z3::context& ctx = sf.ctx;
  z3::expr_vector vec(ctx);

  vector<vector<object_value>> all_perms = get_quantifier_permutations(
      quantifiers, qi.variable_values);

  printf("using %d instantiations\n", (int)all_perms.size());

  for (vector<object_value> const& ovs : all_perms) {
    z3::expr e = sf.interpret(model, ovs);
    vec.push_back(e);
  }

  return z3::mk_and(vec);
}


z3::expr is_true(shared_ptr<Module> module, SketchFormula& sf, shared_ptr<Model> model,
    value candidate) {
  if (DO_FORALL_PRUNING) {
    return assert_true_for_some_qs(module, sf, model, candidate);
  } else {
    return is_something(module, sf, model, true);
  }
}

z3::expr is_false(shared_ptr<Module> module, SketchFormula& sf, shared_ptr<Model> model) {
  //return is_something(module, sf, model, false);
  return sf.interpret_not_forall(model);
}

void add_counterexample(shared_ptr<Module> module, SketchFormula& sf, Counterexample cex,
      value candidate)
{
  z3::context& ctx = sf.ctx;

  if (cex.is_true) {
    sf.solver.add(is_true(module, sf, cex.is_true, candidate));
  }
  else if (cex.is_false) {
    sf.solver.add(is_false(module, sf, cex.is_false));
  }
  else if (cex.hypothesis && cex.conclusion) {
    z3::expr_vector vec(ctx);
    vec.push_back(is_false(module, sf, cex.hypothesis));
    vec.push_back(is_true(module, sf, cex.conclusion, candidate));
    sf.solver.add(z3::mk_or(vec));
  }
  else {
    assert(false);
  }
}

void cex_stats(Counterexample cex) {
  shared_ptr<Model> model;
  if (cex.is_true) {
    model = cex.is_true;
  }
  else if (cex.is_false) {
    model = cex.is_false;
  }
  else if (cex.hypothesis) {
    model = cex.hypothesis;
  }
  else {
    return;
  }

  model->dump_sizes();
}

Counterexample simplify_cex(shared_ptr<Module> module, Counterexample cex,
    BMCContext& bmc,
    BMCContext& antibmc) {
  if (cex.hypothesis) {
    if (bmc.is_reachable(cex.conclusion) ||
        bmc.is_reachable(cex.hypothesis)) {
      Counterexample res;
      res.is_true = cex.conclusion;
      printf("simplifying -> INIT\n");
      return res;
    }

    if (antibmc.is_reachable(cex.conclusion) ||
        antibmc.is_reachable(cex.hypothesis)) {
      Counterexample res;
      res.is_false = cex.hypothesis;
      printf("simplifying -> SAFETY\n");
      return res;
    }

    return cex;
  } else {
    return cex;
  }
}

void synth_loop(shared_ptr<Module> module, int arity, int depth)
{
  z3::context ctx;

  assert(module->templates.size() == 1);

  vector<VarDecl> quants = get_quantifiers(module->templates[0]);

  z3::context ctx_sf;
  z3::solver solver_sf(ctx_sf);
  SketchFormula sf(ctx_sf, solver_sf, quants, module, arity, depth);

  int bmc_depth = 4;
  printf("bmc_depth = %d\n", bmc_depth);
  BMCContext bmc(ctx, module, bmc_depth);
  BMCContext antibmc(ctx, module, bmc_depth, true);

  int num_iterations = 0;

  Benchmarking total_bench;

  while (true) {
    num_iterations++;

    printf("\n");

    printf("number of boolean variables: %d\n", sf.get_bool_count());
    std::cout.flush();

    //cout << solver << "\n";
    Benchmarking bench;
    bench.start("solver (" + to_string(num_iterations) + ")");
    total_bench.start("total solver time");
    z3::check_result res = solver_sf.check();
    bench.end();
    total_bench.end();
    bench.dump();

    assert(res == z3::sat || res == z3::unsat);
    if (res != z3::sat) {
      printf("unable to synthesize any formula\n");
      break;
    }

    z3::model model = solver_sf.get_model();
    value candidate_inner = sf.to_value(model);
    value candidate = fill_holes_in_value(module->templates[0], {candidate_inner});
    printf("candidate: %s\n", candidate->to_string().c_str());

    auto indctx = shared_ptr<InductionContext>(new InductionContext(ctx, module));
    auto initctx = shared_ptr<InitContext>(new InitContext(ctx, module));
    auto conjctx = shared_ptr<ConjectureContext>(new ConjectureContext(ctx, module));
    //auto invctx = shared_ptr<InvariantsContext>(new InvariantsContext(ctx, module));

    //Counterexample cex = get_counterexample(module, initctx, indctx, conjctx, candidate);
    Counterexample cex = get_counterexample_simple(module, bmc, initctx, indctx, conjctx, candidate);
    cex = simplify_cex(module, cex, bmc, antibmc);
    if (cex.none) {
      printf("found invariant: %s\n", candidate->to_string().c_str());
      break;
    }

    cex_stats(cex);
    add_counterexample(module, sf, cex, candidate);
  }

  total_bench.dump();
}
