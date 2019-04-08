#include "synth_loop.h"

#include "z3++.h"
#include "model.h"
#include "sketch.h"
#include "enumerator.h"
#include "benchmarking.h"
#include "bmc.h"

using namespace std;

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
    value candidate)
{
  shared_ptr<Model> model = bmc.get_k_invariance_violation(candidate);
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

Counterexample get_counterexample(
    shared_ptr<Module> module,
    BMCContext& bmc,
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
    cex.is_true = Model::extract_model_from_z3(
        initctx->ctx->ctx,
        init_solver, module, *initctx->e);

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
    shared_ptr<Model> m1 = Model::extract_model_from_z3(
          indctx->ctx->ctx,
          solver, module, *indctx->e1);
    shared_ptr<Model> m2 = Model::extract_model_from_z3(
          indctx->ctx->ctx,
          solver, module, *indctx->e2);
    solver.pop();

    if (!m2->eval_predicate(full_conj)) {
      printf("counterexample type: SAFETY\n");
      cex.is_false = m1;
    } else {
      Counterexample bmc_cex = get_bmc_counterexample(bmc, candidate);
      if (!bmc_cex.none) {
        return bmc_cex;
      }

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
    if (args.size() == 5 && args[0] == 0 && args[1] == 1 &&
        args[2] == 0 && args[3] == 0 && args[4] == 0) {
    }
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

z3::expr is_true(shared_ptr<Module> module, SketchFormula& sf, shared_ptr<Model> model) {
  return is_something(module, sf, model, true);
}

z3::expr is_false(shared_ptr<Module> module, SketchFormula& sf, shared_ptr<Model> model) {
  return is_something(module, sf, model, false);
  //return sf.interpret_not_forall(model);
}

void add_counterexample(shared_ptr<Module> module, SketchFormula& sf, Counterexample cex) {
  z3::context& ctx = sf.ctx;

  if (cex.is_true) {
    sf.solver.add(is_true(module, sf, cex.is_true));
  }
  else if (cex.is_false) {
    sf.solver.add(is_false(module, sf, cex.is_false));
  }
  else if (cex.hypothesis && cex.conclusion) {
    z3::expr_vector vec(ctx);
    vec.push_back(is_false(module, sf, cex.hypothesis));
    vec.push_back(is_true(module, sf, cex.conclusion));
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

void synth_loop(shared_ptr<Module> module, int arity, int depth)
{
  z3::context ctx;
  z3::solver solver(ctx);

  assert(module->templates.size() == 1);

  vector<VarDecl> quants = get_quantifiers(module->templates[0]);

  SketchFormula sf(ctx, solver, quants, module, arity, depth);

  int bmc_depth = 4;
  printf("bmc_depth = %d\n", bmc_depth);
  BMCContext bmc(ctx, module, bmc_depth);

  while (true) {
    printf("\n");

    //cout << solver << "\n";
    Benchmarking bench;
    bench.start("solver");
    z3::check_result res = solver.check();
    bench.end();
    bench.dump();

    assert(res == z3::sat || res == z3::unsat);
    if (res != z3::sat) {
      printf("unable to synthesize any formula\n");
      break;
    }

    z3::model model = solver.get_model();
    value candidate_inner = sf.to_value(model);
    value candidate = fill_holes_in_value(module->templates[0], {candidate_inner});
    printf("candidate: %s\n", candidate->to_string().c_str());

    auto indctx = shared_ptr<InductionContext>(new InductionContext(ctx, module));
    auto initctx = shared_ptr<InitContext>(new InitContext(ctx, module));
    auto conjctx = shared_ptr<ConjectureContext>(new ConjectureContext(ctx, module));
    //auto invctx = shared_ptr<InvariantsContext>(new InvariantsContext(ctx, module));

    Counterexample cex = get_counterexample(module, bmc, initctx, indctx, conjctx, candidate);
    //cex = simplify_cex(module, cex);
    if (cex.none) {
      printf("found invariant: %s\n", candidate->to_string().c_str());
      break;
    }

    cex_stats(cex);
    add_counterexample(module, sf, cex);
  }
}
