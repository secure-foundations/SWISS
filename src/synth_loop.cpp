#include "synth_loop.h"

#include <iostream>
#include <fstream>
#include <streambuf>
#include <thread>

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
#include "thread_safe_queue.h"

using namespace std;
using namespace json11;

int numRetries = 0;

Counterexample get_bmc_counterexample(
    BMCContext& bmc,
    value candidate,
    Options const& options)
{
  if (!options.pre_bmc) {
    Counterexample cex;
    cex.none = true;
    return cex;
  }

  shared_ptr<Model> model = bmc.get_k_invariance_violation_maybe(candidate, options.minimal_models);
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

shared_ptr<InductionContext> get_counterexample_test_with_conjs_make_indctx(
    shared_ptr<Module> module,
    Options const& options,
    smt::context& ctx,
    value cur_invariant,
    value candidate,
    vector<value> conjectures,
    int j)
{
  auto indctx = shared_ptr<InductionContext>(new InductionContext(ctx, module, j));
  smt::solver& solver = indctx->ctx->solver;
  if (cur_invariant) {
    solver.add(indctx->e1->value2expr(cur_invariant));
  }
  solver.add(indctx->e1->value2expr(candidate));
  for (value conj : conjectures) {
    solver.add(indctx->e1->value2expr(conj));
  }
  return indctx;
}

Counterexample get_counterexample_test_with_conjs(
    shared_ptr<Module> module,
    Options const& options,
    smt::context& ctx,
    value cur_invariant,
    value candidate,
    vector<value> conjectures)
{
  Counterexample cex;
  cex.none = false;

  int num_fails = 0;
  while (true) {
    auto initctx = shared_ptr<InitContext>(new InitContext(ctx, module));
    smt::solver& init_solver = initctx->ctx->solver;
    init_solver.push();
    init_solver.add(initctx->e->value2expr(v_not(candidate)));
    init_solver.set_log_info("init-check");
    smt::SolverResult res = init_solver.check_result();

    if (res == smt::SolverResult::Sat) {
      if (options.minimal_models) {
        cex.is_true = Model::extract_minimal_models_from_z3(
            initctx->ctx->ctx,
            init_solver, module, {initctx->e}, /* hint */ candidate)[0];
      } else {
        cex.is_true = Model::extract_model_from_z3(
            initctx->ctx->ctx,
            init_solver, module, *initctx->e);
        cex.is_true->dump_sizes();
      }

      printf("counterexample type: INIT\n");
      //cex.is_true->dump();
      return cex;
    } else if (res == smt::SolverResult::Unsat) {
      break;
    } else {
      num_fails++;
      assert(num_fails < 20);
      cout << "failure encountered, retrying" << endl;
    }
  }

  vector<shared_ptr<InductionContext>> indctxs;

  for (int j = 0; j < (int)module->actions.size(); j++) {
    indctxs.push_back(get_counterexample_test_with_conjs_make_indctx(
        module, options, ctx, cur_invariant, candidate, conjectures, j));
  }

  for (int k = 0; k < (int)conjectures.size(); k++) {
    for (int j = 0; j < (int)module->actions.size(); j++) {
      int num_fails = 0;
      while (true) {
        auto indctx = indctxs[j];
        smt::solver& solver = indctx->ctx->solver;

        solver.push();
        solver.add(indctx->e2->value2expr(v_not(conjectures[k])));

        solver.set_log_info(
            "inductivity-check-with-conj: " + module->action_names[j]);
        smt::SolverResult res = solver.check_result();

        if (res == smt::SolverResult::Sat) {
          if (options.minimal_models) {
            auto ms = Model::extract_minimal_models_from_z3(
                indctx->ctx->ctx, solver, module, {indctx->e1}, /* hint */ candidate);
            cex.is_false = ms[0];
          } else {
            cex.is_false = Model::extract_model_from_z3(
                indctx->ctx->ctx, solver, module, *indctx->e1);
            cex.is_false->dump_sizes();
          }

          printf("counterexample type: SAFETY\n");

          return cex;
        } else if (res == smt::SolverResult::Unsat) {
          solver.pop();
          break;
        } else {
          num_fails++;
          assert(num_fails < 20);
          cout << "failure encountered, retrying" << endl;

          indctxs[j] = get_counterexample_test_with_conjs_make_indctx(
              module, options, ctx, cur_invariant, candidate, conjectures, j);
        }
      }
    }
  }

  for (int j = 0; j < (int)module->actions.size(); j++) {
    int num_fails = 0;
    while (true) {
      auto indctx = indctxs[j];
      smt::solver& solver = indctx->ctx->solver;

      solver.add(indctx->e2->value2expr(v_not(candidate)));

      solver.set_log_info(
          "inductivity-check: " + module->action_names[j]);
      smt::SolverResult res = solver.check_result();

      if (res == smt::SolverResult::Sat) {
        if (options.minimal_models) {
          auto ms = Model::extract_minimal_models_from_z3(
              indctx->ctx->ctx, solver, module, {indctx->e1, indctx->e2}, /* hint */ candidate);
          cex.hypothesis = ms[0];
          cex.conclusion = ms[1];
        } else {
          cex.hypothesis = Model::extract_model_from_z3(
              indctx->ctx->ctx, solver, module, *indctx->e1);
          cex.conclusion = Model::extract_model_from_z3(
              indctx->ctx->ctx, solver, module, *indctx->e2);
          cex.hypothesis->dump_sizes();
        }

        printf("counterexample type: INDUCTIVE\n");

        return cex;
      } else if (res == smt::SolverResult::Unsat) {
        break;
      } else {
        num_fails++;
        assert(num_fails < 20);
        cout << "failure encountered, retrying" << endl;

        indctxs[j] = get_counterexample_test_with_conjs_make_indctx(
            module, options, ctx, cur_invariant, candidate, conjectures, j);
      }
    }
  }

  cex.none = true;
  return cex;
}

Counterexample get_counterexample_simple(
    shared_ptr<Module> module,
    Options const& options,
    smt::context& ctx,
    BMCContext& bmc,
    bool check_implies_conj,
    value cur_invariant,
    value candidate)
{
  Counterexample cex;
  cex.none = false;

  auto initctx = shared_ptr<InitContext>(new InitContext(ctx, module));
  smt::solver& init_solver = initctx->ctx->solver;
  init_solver.push();
  init_solver.add(initctx->e->value2expr(v_not(candidate)));
  init_solver.set_log_info("init-check");
  bool init_res = init_solver.check_sat();

  if (init_res) {
    if (options.minimal_models) {
      cex.is_true = Model::extract_minimal_models_from_z3(
          initctx->ctx->ctx,
          init_solver, module, {initctx->e}, /* hint */ candidate)[0];
    } else {
      cex.is_true = Model::extract_model_from_z3(
          initctx->ctx->ctx,
          init_solver, module, *initctx->e);
      cex.is_true->dump_sizes();
    }

    printf("counterexample type: INIT\n");
    //cex.is_true->dump();

    init_solver.pop();
    return cex;
  } else {
    init_solver.pop();
  }

  if (check_implies_conj) {
    auto conjctx = shared_ptr<ConjectureContext>(new ConjectureContext(ctx, module));

    smt::solver& conj_solver = conjctx->ctx->solver;
    conj_solver.push();
    if (cur_invariant) {
      conj_solver.add(conjctx->e->value2expr(cur_invariant));
    }
    conj_solver.add(conjctx->e->value2expr(candidate));
    conj_solver.set_log_info("conj-check");
    bool conj_res = conj_solver.check_sat();

    if (conj_res) {
      if (options.minimal_models) {
        cex.is_false = Model::extract_minimal_models_from_z3(
            conjctx->ctx->ctx,
            conj_solver, module, {conjctx->e}, /* hint */ candidate)[0];
      } else {
        cex.is_false = Model::extract_model_from_z3(
            conjctx->ctx->ctx,
            conj_solver, module, *conjctx->e);
        cex.is_false->dump_sizes();
      }

      printf("counterexample type: SAFETY\n");
      //cex.is_false->dump();

      conj_solver.pop();
      return cex;
    } else {
      conj_solver.pop();
    }
  }

  if (options.pre_bmc) {
    Counterexample bmc_cex = get_bmc_counterexample(bmc, candidate, options);
    if (!bmc_cex.none) {
      return bmc_cex;
    }
  }

  for (int j = 0; j < (int)module->actions.size(); j++) {
    auto indctx = shared_ptr<InductionContext>(new InductionContext(ctx, module, j));
    smt::solver& solver = indctx->ctx->solver;
    solver.push();
    if (cur_invariant) {
      solver.add(indctx->e1->value2expr(cur_invariant));
    }
    solver.add(indctx->e1->value2expr(candidate));
    solver.add(indctx->e2->value2expr(v_not(candidate)));

    solver.set_log_info(
        "inductivity-check: " + module->action_names[j]);
    bool res = solver.check_sat();

    if (res) {
      if (options.minimal_models) {
        auto ms = Model::extract_minimal_models_from_z3(
            indctx->ctx->ctx, solver, module, {indctx->e1, indctx->e2}, /* hint */ candidate);
        cex.hypothesis = ms[0];
        cex.conclusion = ms[1];
      } else {
        cex.hypothesis = Model::extract_model_from_z3(
            indctx->ctx->ctx, solver, module, *indctx->e1);
        cex.conclusion = Model::extract_model_from_z3(
            indctx->ctx->ctx, solver, module, *indctx->e2);
        cex.hypothesis->dump_sizes();
      }

      solver.pop();

      printf("counterexample type: INDUCTIVE\n");

      return cex;
    }
  }

  cex.none = true;

  return cex;
}

/*void cex_stats(Counterexample cex) {
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
}*/

Counterexample simplify_cex_nosafety(shared_ptr<Module> module, Counterexample cex, Options const& options,
    BMCContext& bmc) {
  if (!options.post_bmc) {
    return cex;
  }

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


Counterexample simplify_cex(shared_ptr<Module> module, Counterexample cex, Options const& options,
    BMCContext& bmc,
    BMCContext& antibmc) {
  if (!options.post_bmc) {
    return cex;
  }

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

bool invariant_is_nonredundant(shared_ptr<Module> module, smt::context& ctx, vector<value> existingInvariants, value newInvariant)
{
  BasicContext basic(ctx, module);
  for (value v : existingInvariants) {
    basic.ctx->solver.add(basic.e->value2expr(v));
  }
  basic.ctx->solver.add(basic.e->value2expr(v_not(newInvariant)));
  basic.ctx->solver.set_log_info("redundancy check");
  bool res = basic.ctx->solver.check_sat();
  return res;
}

void split_into_invariants_conjectures(
    shared_ptr<Module> module,
    vector<value>& invs,
    vector<value>& conjs)
{
  invs.clear();
  conjs = module->conjectures;
  while (true) {
    bool change = false;
    for (int i = 0; i < (int)conjs.size(); i++) {
      if (is_invariant_wrt(module, v_and(invs), conjs[i])) {
        invs.push_back(conjs[i]);
        conjs.erase(conjs.begin() + i);
        i--;
        change = true;
      }
    }
    if (!change) {
      break;
    }
  }

  for (value v : invs) {
    cout << "[invariant] " << v->to_string() << endl;
  }
  for (value v : conjs) {
    cout << "[conjecture] " << v->to_string() << endl;
  }
}

struct CexStats {
  int count_true;
  int count_false;
  int count_ind;

  CexStats() : count_true(0), count_false(0), count_ind(0) { }

  void add(Counterexample const& cex) {
    if (cex.is_true) {
      count_true++;
    } else if (cex.is_false) {
      count_false++;
    } else {
      count_ind++;
    }
  }

  void dump() const {
    cout << "Counterexamples of type TRUE:       " << count_true << endl;
    cout << "Counterexamples of type FALSE:      " << count_false << endl;
    cout << "Counterexamples of type TRANSITION: " << count_ind << endl;
  }
};

void dump_stats(long long progress, CexStats const& cs,
    std::chrono::time_point<std::chrono::high_resolution_clock> init,
    int num_redundant, long long filtering_ms, long long num_finishers_found) {
  cout << "================= Stats =================" << endl;
  cout << "progress: " << progress << endl;
  cout << "total time running so far: " << as_ms(now() - init)
       << " ms" << endl;
  cout << "total time filtering: " << filtering_ms << " ms" << endl;
  cs.dump();
  cout << "number of redundant invariants found: "
       << num_redundant << endl;
  cout << "number of finisher invariants found: "
       << num_finishers_found << endl;
  cout << "number of retries: " << numRetries << endl;
  smt::dump_smt_stats();
  cout << "=========================================" << endl;
  cout.flush();
}

SynthesisResult synth_loop_main(shared_ptr<Module> module,
  shared_ptr<CandidateSolver> cs,
  Options const& options,
  ThreadSafeQueue* tsq)
{
  SynthesisResult synres;
  synres.done = false;
  assert(false);

  auto t_init = now();

  smt::context ctx;
  if (options.smt_retries && options.with_conjs) {
    // TODO support for !options.with_conjs
    int timeout = 45 * 1000;
    cout << "using SMT timeout " << timeout
         << " for inductivity checks" << endl;
    ctx.set_timeout(45 * 1000);
  }
  smt::context bmcctx;

  int bmc_depth = 4;
  printf("bmc_depth = %d\n", bmc_depth);
  BMCContext bmc(bmcctx, module, bmc_depth);
  BMCContext antibmc(bmcctx, module, bmc_depth, true);

  bmcctx.set_timeout(15000); // 15 seconds

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

  vector<value> cur_invariants;
  vector<value> conjectures;
  split_into_invariants_conjectures(module, cur_invariants /* output */, conjectures /* output */);
  value cur_invariant = v_and(cur_invariants);

  //Transcript transcript;

  CexStats cexstats;

  if (options.get_space_size) {
    cout << "space size: " << cs->getSpaceSize() << endl;
    exit(0);
  }

  if (tsq) {
    SpaceChunk* sc = tsq->getNextSpace();
    assert (sc != NULL);
    cs->setSpaceChunk(*sc);
  }

  long long filtering_ms = 0;

  long long num_finishers_found = 0;

  while (true) {
    num_iterations++;

    printf("\n");

    //log_smtlib(solver_sf);
    //printf("number of boolean variables: %d\n", sf.get_bool_count());
    cout << "num iterations " << num_iterations << endl;
    std::cout.flush();

    auto filtering_t1 = now();
    value candidate = cs->getNext();
    filtering_ms += as_ms(now() - filtering_t1);

    if (!candidate) {
      if (tsq) {
        SpaceChunk* sc = tsq->getNextSpace();
        if (sc != NULL) {
          cs->setSpaceChunk(*sc);
          cout << "NEW CHUNK (" << tsq->i << " / " << tsq->q.size() << ") "
               << sc->to_string() << endl;
          continue;
        }
      }

      printf("unable to synthesize any formula\n");
      break;
    }

    cout << "candidate: " << candidate->to_string() << endl;

    Counterexample cex;
    if (options.with_conjs) {
      cex = get_counterexample_test_with_conjs(module, options, ctx, cur_invariant, candidate, conjectures);
      cex = simplify_cex_nosafety(module, cex, options, bmc);
    } else {
      cex = get_counterexample_simple(module, options, ctx, bmc, true, nullptr, candidate);
      cex = simplify_cex(module, cex, options, bmc, antibmc);
    }

    cexstats.add(cex);

    if (cex.none) {
      num_finishers_found++;

      if (tsq && !options.whole_space) {
        tsq->clear();
      }

      // Extra verification:
      // (If we get here, then it should definitely be invariant,
      // this is just a sanity check that our code is right.)
      bool is_inv;
      if (options.with_conjs) {
        vector<value> conjs = module->conjectures;
        conjs.push_back(candidate);
        is_inv = is_itself_invariant(module, conjs);
      } else {
        is_inv = is_complete_invariant(module, candidate);
      }
      if (is_inv) {
        printf("found invariant: %s\n", candidate->to_string().c_str());
        synres.done = true;
        synres.new_values.push_back(candidate);
      } else {
        printf("ERROR: invariant is not actually invariant");
        assert(false);
      }

      if (!options.whole_space) {
        break;
      }
    } else {
      //cex_stats(cex);
      cs->addCounterexample(cex, candidate);
      //transcript.entries.push_back(make_pair(cex, candidate));
    }

    dump_stats(cs->getProgress(), cexstats, t_init, 0, filtering_ms, num_finishers_found);
  }

  //cout << transcript.to_json().dump() << endl;
  dump_stats(cs->getProgress(), cexstats, t_init, 0, filtering_ms, num_finishers_found);
  cout << "complete!" << endl;

  return synres;
}

/*void synth_loop_thread_starter(
  shared_ptr<Module> module,
  vector<EnumOptions> enum_options,
  Options options,
  ThreadSafeQueue *tsq)
{
  shared_ptr<CandidateSolver> cs = make_candidate_solver(module, options.enum_sat, enum_options, false);
  return synth_loop_main(module, cs, options, tsq);
}*/

SynthesisResult synth_loop(
  shared_ptr<Module> module,
  vector<EnumOptions> const& enum_options,
  Options const& options,
  bool use_input_chunks,
  vector<SpaceChunk> const& chunks)
{
  shared_ptr<CandidateSolver> cs = make_candidate_solver(module, options.enum_sat, enum_options, false);
  if (use_input_chunks) {
    ThreadSafeQueue tsq;
    tsq.q = chunks;
    return synth_loop_main(module, cs, options, &tsq);
  } else {
    return synth_loop_main(module, cs, options, NULL);
  }
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

SynthesisResult synth_loop_incremental(shared_ptr<Module> module, vector<EnumOptions> const& enum_options, Options const& options)
{
  auto t_init = now();

  smt::context ctx;
  smt::context bmcctx;

  vector<value> all_found_invs;
  vector<value> found_invs;
  vector<value> all_found_invs_unsimplified;
  if (is_invariant_with_conjectures(module, v_true())) {
    printf("already invariant, done\n");
    return SynthesisResult(true, {});
  }

  int bmc_depth = 4;
  printf("bmc_depth = %d\n", bmc_depth);
  BMCContext bmc(bmcctx, module, bmc_depth);
  bmcctx.set_timeout(15000); // 15 seconds

  int num_iterations_total = 0;
  int num_redundant = 0;

  std::vector<std::pair<Counterexample, value>> cexes;

  CexStats cexstats;

  long long filtering_ms = 0;

  while (true) {
    shared_ptr<CandidateSolver> cs = make_candidate_solver(module, options.enum_sat, enum_options, true);
    if (options.enum_sat) {
      for (value inv : found_invs) {
        cs->addExistingInvariant(inv);
      }
    } else {
      for (value inv : all_found_invs_unsimplified) {
        cs->addExistingInvariant(inv);
      }
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

      auto filtering_t1 = now();
      value candidate0 = cs->getNext();
      filtering_ms += as_ms(now() - filtering_t1);

      if (!candidate0) {
        printf("unable to synthesize any formula\n");
        goto done;
      }

      cout << "candidate: " << candidate0->to_string() << endl;

      value candidate = candidate0->reduce_quants();

      //auto indctx = shared_ptr<InductionContext>(new InductionContext(ctx, module));
      //auto initctx = shared_ptr<InitContext>(new InitContext(ctx, module));
      Counterexample cex = get_counterexample_simple(
                module, options, ctx, bmc, false,
                v_and(found_invs), candidate);

      Benchmarking bench2;
      bench2.start("simplification");
      cex = simplify_cex_nosafety(module, cex, options, bmc);
      bench2.end();
      bench2.dump();

      assert(cex.is_false == nullptr);

      cexstats.add(cex);

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
        all_found_invs_unsimplified.push_back(candidate0);
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

        dump_stats(cs->getProgress(), cexstats, t_init, num_redundant, filtering_ms, 0);

        break;
      } else {
        //cex_stats(cex);

        Benchmarking add_cex_bench;
        add_cex_bench.start("add_counterexample");
        cs->addCounterexample(cex, candidate0);
        cexes.push_back(make_pair(cex, candidate0));
        add_cex_bench.end();
        add_cex_bench.dump();
      }

      per_inner_loop_bench.end();
      per_inner_loop_bench.dump();

      dump_stats(cs->getProgress(), cexstats, t_init, num_redundant, filtering_ms, 0);
    }

    assert(is_itself_invariant(module, found_invs));

    if (!options.whole_space && is_invariant_with_conjectures(module, found_invs)) {
      printf("invariant implies safety condition, done!\n");

      dump_stats(cs->getProgress(), cexstats, t_init, num_redundant, filtering_ms, 0);
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

  return SynthesisResult(false, found_invs);
}

SynthesisResult synth_loop_incremental_breadth(
    shared_ptr<Module> module,
    vector<EnumOptions> const& enum_options,
    Options const& options,
    bool use_input_chunks,
    vector<SpaceChunk> const& chunks)
{
  auto t_init = now();

  smt::context ctx;
  smt::context bmcctx;

  vector<value> starter_invariants;
  vector<value> conjectures;
  split_into_invariants_conjectures(module,
      starter_invariants /* output */,
      conjectures /* output */);

  if (conjectures.size() == 0) {
    cout << "already invariant, done" << endl;
    return SynthesisResult(true, {});
  }

  vector<value> raw_invs;
  vector<value> strengthened_invs;
  vector<value> filtered_simplified_strengthened_invs = starter_invariants;
  vector<value> new_filtered_simplified_strengthened_invs;

  int bmc_depth = 4;
  printf("bmc_depth = %d\n", bmc_depth);
  BMCContext bmc(bmcctx, module, bmc_depth);
  bmcctx.set_timeout(15000); // 15 seconds

  int num_iterations_total = 0;
  int num_iterations_outer = 0;
  int num_redundant = 0;

  CexStats cexstats;

  unique_ptr<ThreadSafeQueue> tsq;
  if (use_input_chunks) {
    tsq.reset(new ThreadSafeQueue());
    tsq->q = chunks;
  }

  long long filtering_ms = 0;

  while (true) {
    num_iterations_outer++;

    shared_ptr<CandidateSolver> cs = make_candidate_solver(module, options.enum_sat, enum_options, true);
    if (options.enum_sat) {
      for (value inv : filtered_simplified_strengthened_invs) {
        cs->addExistingInvariant(inv);
      }
    } else {
      for (value inv : strengthened_invs) {
        cs->addExistingInvariant(inv);
      }
    }

    int num_iterations = 0;
    bool any_formula_synthesized_this_round = false;

    if (tsq) {
      SpaceChunk* sc = tsq->getNextSpace();
      assert (sc != NULL);
      cs->setSpaceChunk(*sc);
    }

    while (true) {
      num_iterations++;
      num_iterations_total++;

      cout << endl;

      auto filtering_t1 = now();
      value candidate0 = cs->getNext();
      filtering_ms += as_ms(now() - filtering_t1);

      if (!candidate0) {
        if (tsq) {
          SpaceChunk* sc = tsq->getNextSpace();
          if (sc != NULL) {
            cs->setSpaceChunk(*sc);
            cout << "NEW CHUNK (" << tsq->i << " / " << tsq->q.size() << ") "
                 << sc->to_string() << endl;
            continue;
          }
        }

        break;
      }

      cout << "total iterations: " << num_iterations_total << " (" << num_iterations_outer << " outer + " << num_iterations << ")" << endl;
      cout << "candidate: " << candidate0->to_string() << endl;

      value candidate = candidate0->reduce_quants();

      Counterexample cex = get_counterexample_simple(
                module, options, ctx, bmc, false /* check_implies_conj */,
                v_and(filtered_simplified_strengthened_invs), candidate);

      Benchmarking bench2;
      bench2.start("simplification");
      cex = simplify_cex_nosafety(module, cex, options, bmc);
      bench2.end();
      bench2.dump();

      assert(cex.is_false == nullptr);

      cexstats.add(cex);

      if (cex.none) {
        //Benchmarking bench_strengthen;
        //bench_strengthen.start("strengthen");
        value strengthened_inv = strengthen_invariant(module, v_and(filtered_simplified_strengthened_invs), candidate0);
        value simplified_strengthened_inv = strengthened_inv->simplify()->reduce_quants();
        //bench_strengthen.end();
        //bench_strengthen.dump();
        cout << "strengthened " << strengthened_inv->to_string() << endl;

        bool is_nonredundant;
        if (options.enum_sat) {
          is_nonredundant = true;
        } else {
          is_nonredundant = invariant_is_nonredundant(module, ctx, filtered_simplified_strengthened_invs, simplified_strengthened_inv);
        }

        raw_invs.push_back(candidate0);
        strengthened_invs.push_back(strengthened_inv);

        if (is_nonredundant) {
          any_formula_synthesized_this_round = true;

          filtered_simplified_strengthened_invs.push_back(simplified_strengthened_inv);
          new_filtered_simplified_strengthened_invs.push_back(simplified_strengthened_inv);

          cout << "\nfound new invariant! all so far:\n";
          for (value found_inv : filtered_simplified_strengthened_invs) {
            cout << "    " << found_inv->to_string() << endl;
          }

          if (!options.whole_space && is_invariant_with_conjectures(module, filtered_simplified_strengthened_invs)) {
            cout << "invariant implies safety condition, done!" << endl;
            dump_stats(cs->getProgress(), cexstats, t_init, num_redundant, filtering_ms, 0);
            return SynthesisResult(true, new_filtered_simplified_strengthened_invs);
          }
        } else {
          cout << "invariant is redundant" << endl;
          num_redundant++;
        }

        if (options.enum_sat) {
          cs->addExistingInvariant(simplified_strengthened_inv);
        } else {
          cs->addExistingInvariant(strengthened_inv);
        }
      } else {
        //cex_stats(cex);
        cs->addCounterexample(cex, candidate0);
      }

      dump_stats(cs->getProgress(), cexstats, t_init, num_redundant, filtering_ms, 0);
    }

    dump_stats(cs->getProgress(), cexstats, t_init, num_redundant, filtering_ms, 0);

    if (!any_formula_synthesized_this_round || use_input_chunks) {
      cout << "unable to synthesize any formula" << endl;
      break;
    }
  }

  return SynthesisResult(false, new_filtered_simplified_strengthened_invs);
}
