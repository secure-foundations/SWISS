#include "bmc.h"

using namespace std;

FixedBMCContext::FixedBMCContext(z3::context& z3ctx, shared_ptr<Module> module, int k)
    : ctx(new BackgroundContext(z3ctx, module))
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
  for (shared_ptr<Value> init : module->inits) {
    ctx->solver.add(this->e1->value2expr(init));
  }
}

bool FixedBMCContext::is_exactly_k_invariant(value v) {
  z3::solver& solver = ctx->solver;
  solver.push();
  solver.add(this->e2->value2expr(v_not(v)));
  z3::check_result res = solver.check();
  assert(res == z3::sat || res == z3::unsat);
  solver.pop();
  return res == z3::unsat;
}

BMCContext::BMCContext(z3::context& ctx, shared_ptr<Module> module, int k) {
  for (int i = 1; i <= k; i++) {
    bmcs.push_back(shared_ptr<FixedBMCContext>(new FixedBMCContext(ctx, module, i)));
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
