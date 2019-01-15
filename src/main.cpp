#include "logic.h"
#include "contexts.h"
#include "model.h"

#include <iostream>
#include <iterator>
#include <string>

using namespace std;

bool try_to_add_invariant(
    shared_ptr<InitContext> initctx,
    shared_ptr<InductionContext> indctx,
    shared_ptr<ConjectureContext> conjctx,
    shared_ptr<Value> conjecture
    ) {
  shared_ptr<Value> not_conjecture = shared_ptr<Value>(new Not(conjecture));

  // Check if INIT ==> INV
  z3::solver& init_solver = initctx->ctx->solver;
  init_solver.push();
  init_solver.add(initctx->e->value2expr(not_conjecture));
  z3::check_result init_res = init_solver.check();

  assert (init_res == z3::sat || init_res == z3::unsat);

  if (init_res == z3::sat) {
    init_solver.pop();
    return false;
  } else {
    init_solver.pop();
  }

  z3::solver& solver = indctx->ctx->solver;

  solver.push();
  solver.add(indctx->e1->value2expr(conjecture));
  solver.push();
  solver.add(indctx->e2->value2expr(not_conjecture));

  z3::check_result res = solver.check();
  assert (res == z3::sat || res == z3::unsat);

  /*
  printf("'%s'\n", solver.to_smt2().c_str());

  if (res == z3::sat) {
    z3::model m = solver.get_model();
    std::cout << m << "\n";
  }
  */

  if (res == z3::unsat) {
    solver.pop();

    z3::solver& conj_solver = conjctx->ctx->solver;

    solver.add(indctx->e2->value2expr(conjecture));
    init_solver.add(initctx->e->value2expr(conjecture));
    conj_solver.add(conjctx->e->value2expr(conjecture));

    return true;
  } else {
    // NOT INVARIANT
    // pop back to last good state
    solver.pop();
    solver.pop();
    return false;
  }
}

bool do_invariants_imply_conjecture(shared_ptr<ConjectureContext> conjctx) {
  z3::solver& solver = conjctx->ctx->solver;
  z3::check_result res = solver.check();
  assert (res == z3::sat || res == z3::unsat);
  return (res == z3::unsat);
}


int main() {
  try {
    std::istreambuf_iterator<char> begin(std::cin), end;
    std::string json_src(begin, end);

    shared_ptr<Module> module = parse_module(json_src);

    z3::context ctx;

    auto indctx = shared_ptr<InductionContext>(new InductionContext(ctx, module));
    auto initctx = shared_ptr<InitContext>(new InitContext(ctx, module));
    auto conjctx = shared_ptr<ConjectureContext>(new ConjectureContext(ctx, module));

    //for (int i = module->conjectures.size() - 1; i >= 0; i--) {
    //  add_invariant(indctx, initctx, conjctx, module->conjectures[i]);
    //}

    /*
    initctx->ctx->solver.add(initctx->e->value2expr(shared_ptr<Value>(new Not(module->conjectures[0]))));
    z3::check_result res = initctx->ctx->solver.check();
    assert(res == z3::sat);
    Model m = Model::extract_model_from_z3(ctx, initctx->ctx->solver, module, *initctx->e);
    m.dump();
    */

    printf("done\n");
  } catch (z3::exception exc) {
    printf("got z3 exception: %s\n", exc.msg());
    throw;
  }
}
