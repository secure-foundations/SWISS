#include "logic.h"
#include "contexts.h"

#include <iostream>
#include <iterator>
#include <string>

using namespace std;

void add_invariant(
    shared_ptr<InductionContext> indctx,
    shared_ptr<Value> conjecture
    ) {
  shared_ptr<Value> not_conjecture = shared_ptr<Value>(new Not(conjecture));

  z3::solver& solver = indctx->ctx->solver;

  solver.push();
  solver.add(indctx->e1->value2expr(conjecture));
  solver.push();
  solver.add(indctx->e2->value2expr(not_conjecture));

  z3::check_result res = solver.check();
  assert(res == z3::unsat);

  solver.pop();
  solver.add(indctx->e2->value2expr(conjecture));
}

int main() {
  std::istreambuf_iterator<char> begin(std::cin), end;
  std::string json_src(begin, end);

  shared_ptr<Module> module = parse_module(json_src);

  z3::context ctx;

  auto bgctx = shared_ptr<BackgroundContext>(new BackgroundContext(ctx, module));
  auto indctx = shared_ptr<InductionContext>(new InductionContext(bgctx, module));

  for (int i = module->conjectures.size(); i >= 0; i--) {
    add_invariant(indctx, module->conjectures[i]);
  }

  printf("'%s'\n", bgctx->solver.to_smt2().c_str());
}
