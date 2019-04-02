#include "synth_loop.h"

#include "z3++.h"
#include "model.h"
#include "sketch.h"
#include "enumerator.h"

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

Counterexample get_counterexample(
    shared_ptr<Module> module,
    shared_ptr<InitContext> initctx,
    shared_ptr<InductionContext> indctx,
    shared_ptr<ConjectureContext> conjctx,
    shared_ptr<Value> candidate)
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
    cex.is_true->dump();
    printf("\n");
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
    printf("counterexample type: SAFETY\n");
    cex.is_false = Model::extract_model_from_z3(
        conjctx->ctx->ctx,
        conj_solver, module, *conjctx->e);
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
    printf("counterexample type: INDUCTIVE\n");
    cex.hypothesis = Model::extract_model_from_z3(
          indctx->ctx->ctx,
          solver, module, *indctx->e1);
    cex.conclusion = Model::extract_model_from_z3(
          indctx->ctx->ctx,
          solver, module, *indctx->e2);
    solver.pop();
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

void synth_loop(shared_ptr<Module> module)
{
  z3::context ctx;
  z3::solver solver(ctx);

  assert(module->templates.size() == 1);

  vector<VarDecl> quants = get_quantifiers(module->templates[0]);

  SketchFormula sf(ctx, solver, quants, module, 2, 2);

  while (true) {
    cout << solver << "\n";
    z3::check_result res = solver.check();

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

    Counterexample cex = get_counterexample(module, initctx, indctx, conjctx, candidate);
    if (cex.none) {
      printf("found invariant: %s\n", candidate->to_string().c_str());
    }

    add_counterexample(module, sf, cex);
  }
}
