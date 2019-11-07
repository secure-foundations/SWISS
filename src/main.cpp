#include "logic.h"
#include "contexts.h"
#include "model.h"
#include "grammar.h"
#include "smt.h"
#include "benchmarking.h"
#include "bmc.h"
#include "enumerator.h"
#include "utils.h"
#include "progress_bar.h"
#include "synth_loop.h"
#include "sat_solver.h"
#include "wpr.h"

#include <iostream>
#include <iterator>
#include <string>
#include <cstdlib>

using namespace std;

bool try_to_add_invariant(
    shared_ptr<Module> module,
    shared_ptr<InitContext> initctx,
    shared_ptr<InductionContext> indctx,
    shared_ptr<ConjectureContext> conjctx,
    shared_ptr<InvariantsContext> invctx,
    shared_ptr<Value> conjecture,
    shared_ptr<Model>* first_state_model, // if non-NULL, then return a model
    shared_ptr<Model>* second_state_model // if non-NULL, then return a model
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

    if (first_state_model) {
      *first_state_model = Model::extract_model_from_z3(
          indctx->ctx->ctx,
          solver, module, *indctx->e1);
    }
    
    if (second_state_model) {
      *second_state_model = Model::extract_model_from_z3(
          indctx->ctx->ctx,
          solver, module, *indctx->e2);
    }

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

value augment_invariant(value a, value b) {
  if (Forall* f = dynamic_cast<Forall*>(a.get())) {
    return v_forall(f->decls, augment_invariant(f->body, b));
  }
  else if (Or* o = dynamic_cast<Or*>(a.get())) {
    vector<value> args = o->args;
    args.push_back(b);
    return v_or(args);
  }
  else {
    return v_or({a, b});
  }
}

void enumerate_next_level(
    vector<value> const& fills,
    vector<value>& next_level,
    value invariant,
    QuantifierInstantiation const& qi)
{
  for (value fill : fills) {
    if (eval_qi(qi, fill)) {
      value newv = augment_invariant(invariant, fill);
      next_level.push_back(newv);
    }
  }
}

void guided_incremental(
    shared_ptr<Module> module,
    shared_ptr<InitContext> initctx,
    shared_ptr<InductionContext> indctx,
    shared_ptr<ConjectureContext> conjctx,
    shared_ptr<InvariantsContext> invctx
) {
  Benchmarking bench;

  printf("checking...\n");
  if (try_to_add_invariant(module, initctx, indctx, conjctx, invctx, v_and(module->conjectures), NULL, NULL)) {
    printf("conjectures already invariant\n");
    return;
  }

  printf("conjectures not invariant\n");
  printf("enumerating ...\n");

  bench.start("enum");

  vector<value> level1 = enumerate_fills_for_template(module, module->templates[0]);
  vector<value> fills;
  for (value v : level1) {
    while (Forall* f = dynamic_cast<Forall*>(v.get())) {
      v = f->body;
    }
    fills.push_back(v);
  }
  bench.end();

  printf("%d fills\n", (int)fills.size());
  for (value fill : fills) {
    //printf("fill: %s\n", fill->to_string().c_str()); 
  }

  bench.start("model gen");
  vector<shared_ptr<Model>> models = get_tree_of_models2(
      initctx->ctx->ctx,
      module, 5, 3);
  bench.end();
  printf("using %d models\n", (int)models.size());

  BMCContext bmc(initctx->ctx->ctx, module, 4);

  vector<vector<value>> levels;

  while (true) {
    levels.clear();
    levels.push_back({});
    levels.push_back(level1);

    vector<pair<shared_ptr<Model>, shared_ptr<Model>>> double_models;

    for (int i = 1; true; i++) {
      bench.start("loop " + to_string(i));
      vector<value> filtered = remove_equiv2(levels[i]);
      printf("level %d: %d candidates, (filtered down from %d)\n", i, (int)filtered.size(), (int)levels[i].size());
      levels[i] = move(filtered);
      vector<value> next_level;
      for (int j = 0; j < levels[i].size(); j++) {
        value invariant = levels[i][j];
        //printf("trying invariant %s\n", invariant->to_string().c_str());

        // Model-checking
        bool model_failed = false;
        for (auto model : models) {
          QuantifierInstantiation qi = get_counterexample(model, invariant);
          if (qi.non_null) {
            enumerate_next_level(fills, next_level, invariant, qi);
            model_failed = true;
            break;
          }
        }
        if (model_failed) {
          continue;
        }

        //printf("passed model checking\n");

        // Double-model-checking
        for (auto p : double_models) {
          if (p.first->eval_predicate(invariant)) {
            QuantifierInstantiation qi = get_counterexample(p.second, invariant);
            if (qi.non_null) {
              enumerate_next_level(fills, next_level, invariant, qi);
              //printf("failed double-model checking\n");
              //p.first->dump();
              //p.second->dump();
              model_failed = true;
              break;
            }
          }
        }
        if (model_failed) {
          continue;
        }

        //printf("passed double-model checking\n");

        // Redundancy check
        bool isr = is_redundant(invctx, invariant);
        if (isr) {
          continue;
        }

        /*
        shared_ptr<Model> violation_model = bmc.get_k_invariance_violation(invariant);
        if (violation_model) {
          models.push_back(violation_model);
          QuantifierInstantiation qi = get_counterexample(violation_model, invariant);
          enumerate_next_level(fills, next_level, invariant, qi);
          continue;
        }
        */

        shared_ptr<Model> model1;
        shared_ptr<Model> model2;
        bool ttai = try_to_add_invariant(
            module, initctx, indctx, conjctx, invctx, invariant, &model1, &model2);
        if (ttai) {
          printf("found invariant (%d): %s\n", i, invariant->to_string().c_str());
          bench.end();
          goto big_loop_end;
        } else {
          double_models.push_back(make_pair(model1, model2));
          QuantifierInstantiation qi = get_counterexample(model2, invariant);
          enumerate_next_level(fills, next_level, invariant, qi);
        }
      }

      levels.push_back(move(next_level));

      bench.end();
      bench.dump();
    }

    big_loop_end:

    if (try_to_add_invariant(module, initctx, indctx, conjctx, invctx, v_and(module->conjectures), NULL, NULL)) {
      printf("conjectures now invariant\n");
      break;
    }
  }
  bench.dump();
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

  if (try_to_add_invariant(module, initctx, indctx, conjctx, invctx, v_and(module->conjectures), NULL, NULL)) {
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

  int N = invariants.size() * 2;
  ProgressBar pb(N);

  for (int i_ = 0; i_ < N; i_++) {
    int i = i_ % invariants.size();
    pb.update(i_);

    if (!is_good_candidate[i]) {
      continue;
    }

    count_iterations++;

    auto invariant = invariants[i];

    if (i_ % 1000 == 0) {
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
    bool ttai = try_to_add_invariant(module, initctx, indctx, conjctx, invctx, invariant, NULL, NULL);
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

  pb.finish();

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

void print_wpr(shared_ptr<Module> module, int count)
{
  shared_ptr<Action> action = shared_ptr<Action>(new ChoiceAction(module->actions));
  value conj = v_and(module->conjectures);
  cout << "conjecture: " << conj->to_string() << endl;

  vector<value> all_conjs;

  value w = conj;
  all_conjs.push_back(w);
  for (int i = 0; i < count; i++) {
    w = wpr(w, action)->simplify();
    all_conjs.push_back(w);
  }

  cout << "list:" << endl;
  for (value conj : all_conjs) {
    for (value part : aggressively_split_into_conjuncts(conj)) {
      cout << part->to_string() << endl;
    }
  }
  cout << endl;

  cout << "wpr: " << w->to_string() << endl;

  if (is_itself_invariant(module, all_conjs)) {
  //if (is_wpr_itself_inductive(module, conj, count)) {
    printf("yes\n");
  } else{
    printf("no\n");
  }
}

int run_id;

int main(int argc, char* argv[]) {
  //test_sat(); return 0;

  // FIXME: quick hack to control which enumeration to use
  bool just_enumeration = false;

  std::istreambuf_iterator<char> begin(std::cin), end;
  std::string json_src(begin, end);

  shared_ptr<Module> module = parse_module(json_src);

  /*z3::context ctx;
  BasicContext basic(ctx, module);
  z3::solver& solver = basic.ctx->solver; 
  for (int i = 0; i < module->conjectures.size(); i++) {
    solver.add(basic.e->value2expr(i == 0 ?
        v_not(module->conjectures[i]) : module->conjectures[i]));
  }
  z3::check_result res = solver.check();
  assert (res == z3::sat || res == z3::unsat);
  if (res == z3::sat) {
    printf("nope\n");
  } else {
    printf("yep\n");
  }
  return 0;*/

  cout << "conjectures:" << endl;
  for (value v : module->conjectures) {
    cout << v->to_string() << endl;
  }

  srand((int)time(NULL));
  run_id = rand();

  Options options;
  options.arity = -1;
  options.depth = -1;
  options.conj_arity = -1;
  options.disj_arity = -1;

  int seed = 1234;
  bool check_inductiveness = false;
  bool incremental = false;
  bool wpr = false;
  int wpr_index = 0;
  for (int i = 1; i < argc; i++) {
    if (argv[i] == string("--random")) {
      seed = (int)time(NULL);
    }
    else if (argv[i] == string("--seed")) {
      assert(i + 1 < argc);
      seed = atoi(argv[i+1]);
      i++;
    }
    else if (argv[i] == string("--wpr")) {
      assert(i + 1 < argc);
      wpr_index = atoi(argv[i+1]);
      wpr = true;
      i++;
    }
    else if (argv[i] == string("--check-inductiveness")) {
      check_inductiveness = true;
    }
    else if (argv[i] == string("--incremental")) {
      incremental = true;
    }
    else if (argv[i] == string("--arity")) {
      assert(i + 1 < argc);
      options.arity = atoi(argv[i+1]);
      i++;
    }
    else if (argv[i] == string("--depth")) {
      assert(i + 1 < argc);
      options.depth = atoi(argv[i+1]);
      i++;
    }
    else if (argv[i] == string("--conj-arity")) {
      assert(i + 1 < argc);
      options.conj_arity = atoi(argv[i+1]);
      i++;
    }
    else if (argv[i] == string("--disj-arity")) {
      assert(i + 1 < argc);
      options.disj_arity = atoi(argv[i+1]);
      i++;
    }
  }

  if (wpr) {
    print_wpr(module, wpr_index);
    return 0;
  }

  if (check_inductiveness) {
    printf("just checking inductiveness...\n");
    if (is_itself_invariant(module, module->conjectures)) {
      printf("yes\n");
    } else{
      printf("no\n");
    }
    return 0;
  }

  printf("random seed = %d\n", seed);
  srand(seed);

  if (incremental) {
    printf("doing incremental\n");
  } else {
    printf("doing non-incremental\n");
  }

  try {
    if (!just_enumeration) {
      //assert(module->templates.size() == 1);
      /*
      vector<shared_ptr<Value>> candidates = enumerate_for_template(module,
          module->templates[0]);
      for (auto can : candidates) {
        printf("%s\n", can->to_string().c_str());
      }
      */

      z3::context ctx;

      auto indctx = shared_ptr<InductionContext>(new InductionContext(ctx, module));
      auto initctx = shared_ptr<InitContext>(new InitContext(ctx, module));
      auto conjctx = shared_ptr<ConjectureContext>(new ConjectureContext(ctx, module));
      auto invctx = shared_ptr<InvariantsContext>(new InvariantsContext(ctx, module));

      //try_to_add_invariants(module, initctx, indctx, conjctx, invctx, candidates);
      //guided_incremental(module, initctx, indctx, conjctx, invctx);
      assert(argc >= 3);

      if (incremental) {
        synth_loop_incremental(module, options);
      } else {
        synth_loop(module, options);
      }
      //synth_loop_from_transcript(module, arity, depth);

      /*
      z3::solver solver = z3::solver(ctx);
      SketchFormula sf(ctx, solver, {}, module, 2, 2);
      z3::check_result res = solver.check();
      assert (res == z3::sat);
      z3::model model = solver.get_model();
      value v = sf.to_value(model);
      printf("value = %s\n", v->to_string().c_str());
      */

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
