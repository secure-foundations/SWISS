#include "logic.h"
#include "contexts.h"
#include "model.h"
#include "grammar.h"
#include "smt.h"
#include "benchmarking.h"
#include "bmc.h"
#include "enumerator.h"
#include "utils.h"

#include <iostream>
#include <iterator>
#include <string>

using namespace std;

bool try_to_add_invariant(
    shared_ptr<InitContext> initctx,
    shared_ptr<InductionContext> indctx,
    shared_ptr<ConjectureContext> conjctx,
    shared_ptr<InvariantsContext> invctx,
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
    z3::solver& inv_solver = invctx->ctx->solver;

    solver.add(indctx->e2->value2expr(conjecture));
    init_solver.add(initctx->e->value2expr(conjecture));
    conj_solver.add(conjctx->e->value2expr(conjecture));
    inv_solver.add(invctx->e->value2expr(conjecture));

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

bool is_redundant(
    shared_ptr<InvariantsContext> invctx,
    shared_ptr<Value> formula)
{
  z3::solver& solver = invctx->ctx->solver;
  solver.push();
  solver.add(invctx->e->value2expr(shared_ptr<Value>(new Not(formula))));

  z3::check_result res = solver.check();
  assert (res == z3::sat || res == z3::unsat);
  solver.pop();
  return (res == z3::unsat);
}

// get a Model that can be an initial state
shared_ptr<Model> get_dumb_model(
    shared_ptr<InitContext> initctx,
    shared_ptr<Module> module)
{
  z3::solver& solver = initctx->ctx->solver;

  solver.push();

  {
  shared_ptr<Sort> node_sort = shared_ptr<Sort>(new UninterpretedSort("node"));
  VarDecl decl_a = VarDecl(string_to_iden("A"), node_sort);
  VarDecl decl_b = VarDecl(string_to_iden("B"), node_sort);
  VarDecl decl_c = VarDecl(string_to_iden("C"), node_sort);

  shared_ptr<Value> var_a = shared_ptr<Value>(new Var(string_to_iden("A"), node_sort));
  shared_ptr<Value> var_b = shared_ptr<Value>(new Var(string_to_iden("B"), node_sort));
  shared_ptr<Value> var_c = shared_ptr<Value>(new Var(string_to_iden("C"), node_sort));

  solver.add(initctx->e->value2expr(shared_ptr<Value>(new Exists(
      { decl_a, decl_b, decl_c },
      shared_ptr<Value>(new And({
        shared_ptr<Value>(new Not(shared_ptr<Value>(new Eq(var_a, var_b)))),
        shared_ptr<Value>(new Not(shared_ptr<Value>(new Eq(var_b, var_c)))),
        shared_ptr<Value>(new Not(shared_ptr<Value>(new Eq(var_c, var_a))))
      }))
    ))));
  }

  z3::check_result res = solver.check();
  assert (res == z3::sat);
  
  shared_ptr<Model> result = Model::extract_model_from_z3(
      initctx->ctx->ctx,
      solver,
      module,
      *(initctx->e));

  solver.pop();

  return result;
}

void try_to_add_invariants(
    shared_ptr<Module> module,
    shared_ptr<InitContext> initctx,
    shared_ptr<InductionContext> indctx,
    shared_ptr<ConjectureContext> conjctx,
    shared_ptr<InvariantsContext> invctx,
    vector<shared_ptr<Value>> const& invariants
) {
  Benchmarking bench;

  if (try_to_add_invariant(initctx, indctx, conjctx, invctx, v_and(module->conjectures))) {
    printf("conjectures already invariant\n");
    return;
  }

  printf("have a list of %d candidate invariants\n", (int)invariants.size());

  bool solved = false;

  vector<shared_ptr<Model>> models = get_tree_of_models2(
      initctx->ctx->ctx,
      module, 5, 3);
  printf("using %d models\n", (int)models.size());
  /*
  vector<shared_ptr<Model>> bad_models = get_tree_of_models2(
      initctx->ctx->ctx,
      module, 5, true);
  printf("using %d bad models\n", (int)bad_models.size());
  */

  BMCContext bmc(initctx->ctx->ctx, module, 4);

  int count_iterations = 0;
  int count_evals = 0;
  int count_bad_evals = 0;
  int count_redundancy_checks = 0;
  int count_invariance_checks = 0;
  int count_invariants_added = 0;
  int count_bmc_checks = 0;
  int count_models_added = 0;

  vector<bool> is_good_candidate;
  is_good_candidate.resize(invariants.size());
  for (int i = 0; i < invariants.size(); i++) {
    is_good_candidate[i] = true;
  }

  vector<value> probable_candidates;

  for (int i_ = 0; i_ < 2*invariants.size(); i_++) {
    int i = i_ % invariants.size();

    if (!is_good_candidate[i]) {
      continue;
    }

    count_iterations++;

    auto invariant = invariants[i];
    if (i_ % 1000 == 0) {
      printf("doing %d\n", i_);

      /*
      printf("total iterations: %d\n", count_iterations);
      printf("total evals against good models: %d\n", count_evals);
      printf("total evals against bad models: %d\n", count_bad_evals);
      printf("total redundancy checks: %d\n", count_redundancy_checks);
      printf("total bmc checks: %d\n", count_bmc_checks);
      printf("total models added: %d\n", count_models_added);
      printf("total invariance checks: %d\n", count_invariance_checks);
      printf("total invariants added: %d\n", count_invariants_added);
      bench.dump();
      */
    }
    //printf("%s\n", invariant->to_string().c_str());
    //printf("%s\n\n", invariant->totally_normalize()->to_string().c_str());

    count_evals++;
    for (auto model : models) {
      bench.start("eval");
      bool model_eval = model->eval_predicate(invariant);
      bench.end();
      if (!model_eval) {
        is_good_candidate[i] = false;
        break;
      }
    }
    if (!is_good_candidate[i]) {
      continue;
    }

    /*
    count_bad_evals++;
    for (auto model : bad_models) {
      bench.start("eval");
      bool model_eval = model->eval_predicate(invariant);
      bench.end();
      if (model_eval) {
        is_good_candidate[i] = false;
        break;
      }
    }
    if (!is_good_candidate[i]) {
      continue;
    }
    */

    count_redundancy_checks++;
    bench.start("redundancy");
    bool isr = is_redundant(invctx, invariant);
    bench.end();
    if (isr) {
      is_good_candidate[i] = false;
      continue;
    }

    count_bmc_checks++;
    bench.start("bmc");
    shared_ptr<Model> violation_model = bmc.get_k_invariance_violation(invariant);
    bench.end();
    if (violation_model) {
      //violation_model->dump(); 
      models.push_back(violation_model);
      count_models_added++;
      is_good_candidate[i] = false;
      continue;
    }

    bool is_redun = false;
    for (value pc : probable_candidates) {
      if (is_redundant_quick(pc, invariant)) {
        is_redun = true;
        break;
      }
    }
    if (is_redun) {
      continue;
    }

    count_invariance_checks++;
    bench.start("try_to_add_invariant");

    //printf("%s\n", invariant->to_string().c_str());
    bool ttai = try_to_add_invariant(initctx, indctx, conjctx, invctx, invariant);
    bench.end();
    if (ttai) {
      is_good_candidate[i] = false;
      // We added an invariant!
      // Now check if we're done.
      printf("found invariant (%d): %s\n", i, invariant->to_string().c_str());
      count_invariants_added++;
      if (do_invariants_imply_conjecture(conjctx)) {
        solved = true;
        break;
      }
    } else {
      probable_candidates.push_back(invariant);
      //printf("\"probable\" invariant (%d): %s\n", i, invariant->to_string().c_str());
    }
  }

  printf("solved: %s\n", solved ? "yes" : "no");
  printf("total iterations: %d\n", count_iterations);
  printf("total evals against good models: %d\n", count_evals);
  printf("total evals against bad models: %d\n", count_bad_evals);
  printf("total redundancy checks: %d\n", count_redundancy_checks);
  printf("total bmc checks: %d\n", count_bmc_checks);
  printf("total models added: %d\n", count_models_added);
  printf("total invariance checks: %d\n", count_invariance_checks);
  printf("total invariants added: %d\n", count_invariants_added);

  bench.dump();

  return;
}

int main() {

  // FIXME: quick hack to control which enumeration to use
  bool just_enumeration = false;

  std::istreambuf_iterator<char> begin(std::cin), end;
  std::string json_src(begin, end);

  shared_ptr<Module> module = parse_module(json_src);

  try {
    if (!just_enumeration) {
      assert(module->templates.size() == 1);
      vector<shared_ptr<Value>> candidates = enumerate_for_template(module,
          module->templates[0]);
      for (auto can : candidates) {
       // printf("%s\n", can->to_string().c_str());
      }

      z3::context ctx;

      auto indctx = shared_ptr<InductionContext>(new InductionContext(ctx, module));
      auto initctx = shared_ptr<InitContext>(new InitContext(ctx, module));
      auto conjctx = shared_ptr<ConjectureContext>(new ConjectureContext(ctx, module));
      auto invctx = shared_ptr<InvariantsContext>(new InvariantsContext(ctx, module));

      try_to_add_invariants(module, initctx, indctx, conjctx, invctx, candidates);
      return 0;
    } else {
      assert(module->templates.size() == 1);
      vector<value> candidates = enumerate_for_template(module,
          module->templates[0]);

      int program = 1;
      for (value v : candidates) {
        std::cout << "#program= " << program << std::endl;
        program++;
        std::cout << v->to_string() << std::endl;
      }
      std::cout << "end" << std::endl;
    }

  } catch (z3::exception exc) {
    printf("got z3 exception: %s\n", exc.msg());
    throw;
  }
}
