#include "synth_loop.h"

#include <iostream>
#include <fstream>
#include <streambuf>

#include "z3++.h"
#include "lib/json11/json11.hpp"

#include "model.h"
#include "sketch.h"
#include "sketch_model.h"
#include "enumerator.h"
#include "benchmarking.h"
#include "bmc.h"
#include "quantifier_permutations.h"
#include "top_quantifier_desc.h"
#include "strengthen_invariant.h"
#include "filter.h"
#include "synth_enumerator.h"

using namespace std;
using namespace json11;

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
    value cur_invariant,
    value candidate)
{
  Benchmarking bench;

  bool use_minimal = true;
  bool use_minimal_only_for_safety = true;
  bool use_bmc = true;

  Counterexample cex;
  cex.none = false;

  z3::solver& init_solver = initctx->ctx->solver;
  init_solver.push();
  init_solver.add(initctx->e->value2expr(v_not(candidate)));
  bench.start("init-check");
  z3::check_result init_res = init_solver.check();
  bench.end();

  if (init_res == z3::sat) {
    if (use_minimal) {
      bench.start("init-minimization");
      cex.is_true = Model::extract_minimal_models_from_z3(
          initctx->ctx->ctx,
          init_solver, module, {initctx->e}, /* hint */ candidate)[0];
      bench.end();
    } else {
      cex.is_true = Model::extract_model_from_z3(
          initctx->ctx->ctx,
          init_solver, module, *initctx->e);
    }

    printf("counterexample type: INIT\n");
    //cex.is_true->dump();

    init_solver.pop();
    bench.dump();
    return cex;
  } else {
    init_solver.pop();
  }

  if (conjctx != nullptr) {
    z3::solver& conj_solver = conjctx->ctx->solver;
    conj_solver.push();
    if (cur_invariant) {
      conj_solver.add(conjctx->e->value2expr(cur_invariant));
    }
    conj_solver.add(conjctx->e->value2expr(candidate));
    bench.start("conj-check");
    z3::check_result conj_res = conj_solver.check();
    bench.end();

    if (conj_res == z3::sat) {
      if (use_minimal || use_minimal_only_for_safety) {
        bench.start("conj-minimization");
        cex.is_false = Model::extract_minimal_models_from_z3(
            conjctx->ctx->ctx,
            conj_solver, module, {conjctx->e}, /* hint */ candidate)[0];
        bench.end();
      } else {
        cex.is_false = Model::extract_model_from_z3(
            conjctx->ctx->ctx,
            conj_solver, module, *conjctx->e);
      }

      printf("counterexample type: SAFETY\n");
      //cex.is_false->dump();

      conj_solver.pop();
      bench.dump();
      return cex;
    } else {
      conj_solver.pop();
    }
  }

  if (use_bmc) {
    bench.start("bmc-attempt-1");
    Counterexample bmc_cex = get_bmc_counterexample(bmc, candidate, use_minimal);
    bench.end();
    if (!bmc_cex.none) {
      bench.dump();
      return bmc_cex;
    }
  }

  z3::solver& solver = indctx->ctx->solver;
  solver.push();
  if (cur_invariant) {
    solver.add(indctx->e1->value2expr(cur_invariant));
  }
  solver.add(indctx->e1->value2expr(candidate));
  solver.add(indctx->e2->value2expr(v_not(candidate)));
  bench.start("inductivity-check");
  z3::check_result res = solver.check();
  bench.end();

  if (res == z3::sat) {
    if (use_minimal) {
      bench.start("inductivity-minimization");
      auto ms = Model::extract_minimal_models_from_z3(
          indctx->ctx->ctx, solver, module, {indctx->e1, indctx->e2}, /* hint */ candidate);
      bench.end();
      cex.hypothesis = ms[0];
      cex.conclusion = ms[1];
    } else {
      cex.hypothesis = Model::extract_model_from_z3(
          indctx->ctx->ctx, solver, module, *indctx->e1);
      cex.conclusion = Model::extract_model_from_z3(
          indctx->ctx->ctx, solver, module, *indctx->e2);
    }

    solver.pop();

    printf("counterexample type: INDUCTIVE\n");

    bench.dump();
    return cex;
  }

  cex.none = true;

  bench.dump();
  return cex;
}

Counterexample get_counterexample(
    shared_ptr<Module> module,
    z3::context& ctx,
    value candidate)
{
  Counterexample cex;
  cex.none = false;

  //cout << "starting init check ..." << endl; cout.flush();

  auto initctx = shared_ptr<InitContext>(new InitContext(ctx, module));

  z3::solver& init_solver = initctx->ctx->solver;
  init_solver.push();
  init_solver.add(initctx->e->value2expr(v_not(candidate)));
  z3::check_result init_res = init_solver.check();

  if (init_res == z3::sat) {
    //cout << "got SAT, starting minimize..." << endl; cout.flush();
    cex.is_true = Model::extract_minimal_models_from_z3(
        initctx->ctx->ctx,
        init_solver, module, {initctx->e}, /* hint */ candidate)[0];
    //cout << "done minimize..." << endl; cout.flush();

    printf("counterexample type: INIT\n");
    //cex.is_true->dump();
    return cex;
  }

  value full_conj = v_and(module->conjectures);
  value full_candidate = v_and({candidate, full_conj});

  for (int i = 0; i <= module->conjectures.size(); i++) {
    //cout << "starting inductive check " << i << endl; cout.flush();

    for (int j = 0; j < module->actions.size(); j++) {
      auto indctx = shared_ptr<InductionContext>(new InductionContext(ctx, module, j));

      z3::solver& solver = indctx->ctx->solver;
      solver.add(indctx->e1->value2expr(full_candidate));

      bool testing_candidate = (i == module->conjectures.size());

      solver.push();
      solver.add(indctx->e2->value2expr(v_not(testing_candidate ? candidate : module->conjectures[i])));
      z3::check_result res = solver.check();

      if (res == z3::sat) {
        //cout << "got SAT, starting minimize" << endl; cout.flush();
        auto m1_and_m2 = Model::extract_minimal_models_from_z3(
              indctx->ctx->ctx,
              solver, module, {indctx->e1, indctx->e2}, /* hint */ candidate);
        //cout << "done minimize" << endl; cout.flush();
        shared_ptr<Model> m1 = m1_and_m2[0];
        shared_ptr<Model> m2 = m1_and_m2[1];

        if (testing_candidate) {
          printf("counterexample type: INDUCTIVE\n");
          cex.hypothesis = m1;
          cex.conclusion = m2;
        } else {
          printf("counterexample type: SAFETY\n");
          cex.is_false = m1;
        }

        //cout << "done evaluating" << endl; cout.flush();

        return cex;
      }
    }
  }

  //cout << "returning no counterexample" << endl; cout.flush();

  cex.none = true;
  return cex;
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

Counterexample simplify_cex_nosafety(shared_ptr<Module> module, Counterexample cex,
    BMCContext& bmc) {
  if (cex.hypothesis) {
    if (bmc.is_reachable_returning_false_if_unknown(cex.conclusion) ||
        bmc.is_reachable_returning_false_if_unknown(cex.hypothesis)) {
      Counterexample res;
      res.is_true = cex.conclusion;
      printf("simplifying -> INIT\n");
      return res;
    }

    return cex;
  } else {
    return cex;
  }
}


Counterexample simplify_cex(shared_ptr<Module> module, Counterexample cex,
    BMCContext& bmc,
    BMCContext& antibmc) {
  if (cex.hypothesis) {
    if (bmc.is_reachable_returning_false_if_unknown(cex.conclusion) ||
        bmc.is_reachable_exact_steps_returning_false_if_unknown(cex.hypothesis)) {
      Counterexample res;
      res.is_true = cex.conclusion;
      printf("simplifying -> INIT\n");
      return res;
    }

    if (antibmc.is_reachable_exact_steps_returning_false_if_unknown(cex.conclusion) ||
        antibmc.is_reachable_returning_false_if_unknown(cex.hypothesis)) {
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

struct Transcript {
  vector<pair<Counterexample, value>> entries;

  Json to_json() const;
  static Transcript from_json(Json, shared_ptr<Module>);
};

Json Counterexample::to_json() const {
  if (none) {
    return Json();
  }
  else if (is_true) {
    return Json(vector<Json>{Json("is_true"), is_true->to_json()});
  }
  else if (is_false) {
    return Json(vector<Json>{Json("is_false"), is_false->to_json()});
  }
  else {
    assert(hypothesis != nullptr);
    assert(conclusion != nullptr);
    return Json({Json("ind"), hypothesis->to_json(), conclusion->to_json()});
  }
}

Counterexample Counterexample::from_json(Json j, shared_ptr<Module> module) {
  Counterexample cex;
  if (j.is_null()) {
    cex.none = true;
    return cex;
  }

  assert(j.is_array());
  assert(j.array_items().size() >= 2);
  assert(j[0].is_string());
  string type = j[0].string_value();
  if (type == "is_true") {
    assert(j.array_items().size() == 2);
    cex.is_true = Model::from_json(j[1], module);
  }
  else if (type == "is_false") {
    assert(j.array_items().size() == 2);
    cex.is_false = Model::from_json(j[1], module);
  }
  else if (type == "ind") {
    assert(j.array_items().size() == 3);
    cex.hypothesis = Model::from_json(j[1], module);
    cex.conclusion = Model::from_json(j[2], module);
  }
  else {
    assert(false);
  }

  return cex;
}

Json Transcript::to_json() const {
  vector<Json> ar;
  for (auto p : entries) {
    map<string, Json> o_ent;
    o_ent.insert(make_pair("cex", p.first.to_json()));
    o_ent.insert(make_pair("candidate", p.second->to_json()));
    ar.push_back(Json(o_ent));
  }
  return Json(ar);
}

Transcript Transcript::from_json(Json j, shared_ptr<Module> module) {
  Transcript t;
  assert(j.is_array());
  for (auto ent : j.array_items()) {
    assert(ent.is_object());
    Counterexample cex = Counterexample::from_json(ent["cex"], module);
    value v = Value::from_json(ent["candidate"]);
    t.entries.push_back(make_pair(cex, v));
  }
  return t;
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


/*
extern int run_id;

void log_smtlib(z3::solver& solver) {
  static int log_num = 1;

  string filename = "./logs/smtlib/log." + to_string(run_id) + "." + to_string(log_num) + ".z3";
  log_num++;

  ofstream myfile;
  myfile.open(filename);
  myfile << solver << endl;
  myfile.close();

  cout << "logged smtlib to " << filename << endl;
}
*/

void synth_loop(shared_ptr<Module> module, Options const& options)
{
  assert(module->templates.size() == 1);

  z3::context ctx;
  z3::context bmcctx;

  int bmc_depth = 4;
  printf("bmc_depth = %d\n", bmc_depth);
  BMCContext bmc(bmcctx, module, bmc_depth);
  BMCContext antibmc(bmcctx, module, bmc_depth, true);

  z3_set_timeout(bmcctx, 15000); // 15 seconds

  int num_iterations = 0;

  /*if (init_transcript) {
    printf("using init_transcript with %d entries\n", (int)init_transcript->entries.size());
    for (auto p : init_transcript->entries) {
      Counterexample cex = p.first;
      value candidate = p.second;
      add_counterexample(module, sf, cex, candidate);
    }
    printf("done\n");
  }*/

  Benchmarking total_bench;
  Transcript transcript;

  shared_ptr<CandidateSolver> cs = make_candidate_solver(module, options, false, Shape::SHAPE_CONJ_DISJ);

  while (true) {
    num_iterations++;

    printf("\n");

    //log_smtlib(solver_sf);
    //printf("number of boolean variables: %d\n", sf.get_bool_count());
    cout << "num iterations " << num_iterations << endl;
    std::cout.flush();

    value candidate = cs->getNext();
    if (!candidate) {
      printf("unable to synthesize any formula\n");
      break;
    }

    cout << "candidate: " << candidate->to_string() << endl;

    auto indctx = shared_ptr<InductionContext>(new InductionContext(ctx, module));
    auto initctx = shared_ptr<InitContext>(new InitContext(ctx, module));
    auto conjctx = shared_ptr<ConjectureContext>(new ConjectureContext(ctx, module));
    //auto invctx = shared_ptr<InvariantsContext>(new InvariantsContext(ctx, module));

    Counterexample cex;
    if (options.with_conjs) {
      //cout << "getting counterexample ..." << endl;
      cout.flush();
      cex = get_counterexample(module, ctx, candidate);
      //cout << "counterexample obtained" << endl;
      cout.flush();
      cex = simplify_cex_nosafety(module, cex, bmc);
      //cout << "simplify_cex done" << endl;
      cout.flush();
    } else {
      cex = get_counterexample_simple(module, bmc, initctx, indctx, conjctx, nullptr, candidate);
      cex = simplify_cex(module, cex, bmc, antibmc);
    }
    if (cex.none) {
      // Extra verification:
      if (is_complete_invariant(module, candidate)) {
        printf("found invariant: %s\n", candidate->to_string().c_str());
      } else {
        printf("ERROR: invariant is not actually invariant");
        assert(false);
      }
      break;
    }

    //cex_stats(cex);
    cs->addCounterexample(cex, candidate);
    transcript.entries.push_back(make_pair(cex, candidate));
  }

  //cout << transcript.to_json().dump() << endl;
  total_bench.dump();
}

/*void synth_loop_from_transcript(shared_ptr<Module> module, int arity, int depth)
{
  string filename = "../../ms";
  ifstream t(filename);
  string contents((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());

  cout << "read in transcript contents:" << endl;
  cout << contents << endl << endl;

  string err;
  if (err.size()) cout << err << "\n";
  Json j = Json::parse(contents, err);
  Transcript transcript = Transcript::from_json(j, module);

  Benchmarking bench;
  bench.start("synth_loop with transcript");

  synth_loop(module, arity, depth, &transcript);

  bench.end();
  bench.dump();
}*/

void synth_loop_incremental(shared_ptr<Module> module, Options const& options)
{
  z3::context ctx;

  assert(module->templates.size() >= 1);

  vector<value> all_found_invs;
  vector<value> found_invs;
  if (is_invariant_with_conjectures(module, v_true())) {
    printf("already invariant, done\n");
    return;
  }

  int bmc_depth = 4;
  printf("bmc_depth = %d\n", bmc_depth);
  BMCContext bmc(ctx, module, bmc_depth);

  int num_iterations_total = 0;

  Benchmarking total_bench;

  std::vector<std::pair<Counterexample, value>> cexes;

  while (true) {
    shared_ptr<CandidateSolver> cs = make_candidate_solver(module, options, true, Shape::SHAPE_DISJ);
    for (value inv : found_invs) {
      cs->addExistingInvariant(inv);
    }
    for (auto& p : cexes) {
      cs->addCounterexample(p.first, p.second);
    }

    Benchmarking per_outer_loop_bench;
    per_outer_loop_bench.start("time to infer this invariant");

    int num_iterations = 0;

    while (true) {
      Benchmarking per_inner_loop_bench;
      per_inner_loop_bench.start("iteration");

      num_iterations++;
      num_iterations_total++;

      cout << "num_iterations_total = " << num_iterations_total << endl;

      printf("\n");

      //log_smtlib(solver_sf);
      //printf("number of boolean variables: %d\n", sf.get_bool_count() + sm.get_bool_count());
      std::cout.flush();

      value candidate0 = cs->getNext();

      if (!candidate0) {
        printf("unable to synthesize any formula\n");
        goto done;
      }

      cout << "candidate: " << candidate0->to_string() << endl;

      value candidate = cs->getNext()->reduce_quants();

      auto indctx = shared_ptr<InductionContext>(new InductionContext(ctx, module));
      auto initctx = shared_ptr<InitContext>(new InitContext(ctx, module));
      Counterexample cex = get_counterexample_simple(
                module, bmc, initctx, indctx, nullptr /* conjctx */,
                v_and(found_invs), candidate);

      Benchmarking bench2;
      bench2.start("simplification");
      cex = simplify_cex_nosafety(module, cex, bmc);
      bench2.end();
      bench2.dump();

      assert(cex.is_false == nullptr);

      if (cex.none) {
        value simplified_inv;
        if (options.enum_sat) {
          Benchmarking bench_strengthen;
          bench_strengthen.start("strengthen");
          simplified_inv = strengthen_invariant(module, v_and(found_invs), candidate)
              ->simplify()->reduce_quants();
          bench_strengthen.end();
          bench_strengthen.dump();
        } else {
          simplified_inv = candidate;
        }

        found_invs.push_back(simplified_inv);
        all_found_invs.push_back(simplified_inv);

        cexes = filter_unneeded_cexes(cexes, v_and(found_invs));

        printf("\nfound new invariant! all so far:\n");
        for (value found_inv : found_invs) {
          printf("    %s\n", found_inv->to_string().c_str());
        }
        if (options.filter_redundant) {
          found_invs = filter_redundant_formulas(module, found_invs);
          printf("\nfiltered:\n");
          for (value found_inv : found_invs) {
            printf("    %s\n", found_inv->to_string().c_str());
          }
          printf("num found so far:   %d\n", (int)found_invs.size());
        }
        printf("TODAL found so far: %d\n", (int)all_found_invs.size());

        per_inner_loop_bench.end();
        per_inner_loop_bench.dump();

        benchmarking_dump_totals();

        break;
      } else {
        cex_stats(cex);

        Benchmarking add_cex_bench;
        add_cex_bench.start("add_counterexample");
        cs->addCounterexample(cex, candidate0);
        cexes.push_back(make_pair(cex, candidate0));
        add_cex_bench.end();
        add_cex_bench.dump();
      }

      per_inner_loop_bench.end();
      per_inner_loop_bench.dump();
    }

    assert(is_itself_invariant(module, found_invs));

    if (is_invariant_with_conjectures(module, found_invs)) {
      printf("invariant implies safety condition, done!\n");
      break;
    }

    printf("\n");
    per_outer_loop_bench.end();
    per_outer_loop_bench.dump();
    printf("\n");
  }

  done:

  printf("\nall invariants found::\n");
  for (value found_inv : found_invs) {
    printf("    %s\n", found_inv->to_string().c_str());
  }
  printf("total invariants found: %d\n", (int)found_invs.size());
  benchmarking_dump_totals();
  printf("\n");
  total_bench.dump();
  printf("\n");
}
