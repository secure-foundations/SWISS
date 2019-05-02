#include "bmc.h"

using namespace std;

FixedBMCContext::FixedBMCContext(z3::context& z3ctx, shared_ptr<Module> module, int k,
    bool from_safety)
    : module(module), ctx(new BackgroundContext(z3ctx, module)), from_safety(from_safety)
{
  this->e1 = ModelEmbedding::makeEmbedding(ctx, module);

  this->e2 = this->e1;
  for (int i = 0; i < k; i++) {
    shared_ptr<Action> action = shared_ptr<Action>(new ChoiceAction(module->actions));
    ActionResult res = applyAction(this->e2, action, {});
    this->e2 = res.e;
    // Add the relation between the two states
    ctx->solver.add(res.constraint);
  }

  // Add the axioms
  for (shared_ptr<Value> axiom : module->axioms) {
    ctx->solver.add(this->e1->value2expr(axiom, {}));
  }

  // Add the inits
  if (!from_safety) {
    for (shared_ptr<Value> init : module->inits) {
      ctx->solver.add(this->e1->value2expr(init));
    }
  } else {
    ctx->solver.add(this->e2->value2expr(v_not(v_and(module->conjectures))));
  }
}

bool FixedBMCContext::is_exactly_k_invariant(value v) {
  z3::solver& solver = ctx->solver;
  solver.push();
  if (!from_safety) {
    solver.add(this->e2->value2expr(v_not(v)));
  } else{
    solver.add(this->e1->value2expr(v_not(v)));
  }
  z3::check_result res = solver.check();
  assert(res == z3::sat || res == z3::unsat);
  solver.pop();
  return res == z3::unsat;
}

shared_ptr<Model> FixedBMCContext::get_k_invariance_violation(value v, bool get_minimal) {
  z3::solver& solver = ctx->solver;
  solver.push();
  if (!from_safety) {
    solver.add(this->e2->value2expr(v_not(v)));
  } else {
    solver.add(this->e1->value2expr(v_not(v)));
  }
  z3::check_result res = solver.check();
  assert(res == z3::sat || res == z3::unsat);

  shared_ptr<Model> ans;
  if (res == z3::sat) {
    if (get_minimal) {
      ans = Model::extract_minimal_models_from_z3(ctx->ctx, solver, module, {e2})[0];
    } else {
      ans = Model::extract_model_from_z3(ctx->ctx, solver, module, *e2);
    }
  }

  solver.pop();
  return ans;
}

bool FixedBMCContext::is_reachable(shared_ptr<Model> model) {
  z3::solver& solver = ctx->solver;
  solver.push();
  if (!from_safety) {
    model->assert_model_is(this->e2);
  } else {
    model->assert_model_is(this->e1);
  }

  z3::check_result res = solver.check();
  assert(res == z3::sat || res == z3::unsat);
  solver.pop();
  return res == z3::sat;
}

BMCContext::BMCContext(z3::context& ctx, shared_ptr<Module> module, int k, bool from_safety) {
  for (int i = 1; i <= k; i++) {
    bmcs.push_back(shared_ptr<FixedBMCContext>(new FixedBMCContext(ctx, module, i, from_safety)));
  }
}

bool BMCContext::is_k_invariant(value v) {
  for (auto bmc : bmcs) {
    if (!bmc->is_exactly_k_invariant(v)) {
      return false;
    }
  }
  return true;
}

shared_ptr<Model> BMCContext::get_k_invariance_violation(value v, bool get_minimal) {
  for (auto bmc : bmcs) {
    shared_ptr<Model> mod = bmc->get_k_invariance_violation(v, get_minimal);
    if (mod) {
      return mod;
    }
  }
  return nullptr;
}

bool BMCContext::is_reachable(std::shared_ptr<Model> model) {
  for (auto bmc : bmcs) {
    if (bmc->is_reachable(model)) {
      return true;
    }
  }
  return false;
}

bool BMCContext::is_reachable_exact_steps(std::shared_ptr<Model> model) {
  return bmcs[bmcs.size() - 1]->is_reachable(model);
}
