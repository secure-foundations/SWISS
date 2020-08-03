#include "synth_loop.h"

#include <iostream>
#include <fstream>
#include <streambuf>

#include "lib/json11/json11.hpp"

#include "model.h"
#include "enumerator.h"
#include "benchmarking.h"
#include "bmc.h"
#include "quantifier_permutations.h"
#include "top_quantifier_desc.h"
#include "strengthen_invariant.h"
#include "filter.h"
#include "synth_enumerator.h"
#include "utils.h"
#include "solve.h"

using namespace std;
using namespace json11;

int numRetries = 0;
int numTryHardFailures = 0;

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
    shared_ptr<BackgroundContext> bgctx,
    value cur_invariant,
    value candidate,
    vector<value> conjectures,
    int j)
{
  auto indctx = shared_ptr<InductionContext>(new InductionContext(bgctx, module, j));
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
    value cur_invariant,
    value candidate,
    vector<value> conjectures)
{
  Counterexample cex;
  cex.none = false;

  ContextSolverResult csr = context_solve(
      "init-check",
      module,
      options.minimal_models ? ModelType::Min : ModelType::Any,
      Strictness::TryHard,
      candidate /* hint */,
      [module, candidate](shared_ptr<BackgroundContext> bgctx)
  {
    auto initctx = shared_ptr<InitContext>(new InitContext(bgctx, module));
    smt::solver& init_solver = initctx->ctx->solver;
    init_solver.push();
    init_solver.add(initctx->e->value2expr(v_not(candidate)));
    init_solver.set_log_info("init-check");
    return vector<shared_ptr<ModelEmbedding>>{initctx->e};
  });

  if (csr.res == smt::SolverResult::Unknown) {
    return cex;
  }

  if (csr.res == smt::SolverResult::Sat) {
    cex.is_true = csr.models[0];
    printf("counterexample type: INIT\n");
    return cex;
  }

  for (int k = 0; k < (int)conjectures.size(); k++) {
    for (int j = 0; j < (int)module->actions.size(); j++) {
      ContextSolverResult csr = context_solve(
          "inductivity-check-with-conj: " + module->action_names[j],
          module,
          options.minimal_models ? ModelType::Min : ModelType::Any,
          Strictness::TryHard,
          candidate /* hint */,
          [module, k, j, &options, cur_invariant, candidate, &conjectures](shared_ptr<BackgroundContext> bgctx)
      {
        auto indctx = get_counterexample_test_with_conjs_make_indctx(
              module, options, bgctx, cur_invariant, candidate, conjectures, j);
        smt::solver& solver = indctx->ctx->solver;

        solver.add(indctx->e2->value2expr(v_not(conjectures[k])));

        solver.set_log_info(
            "inductivity-check-with-conj: " + module->action_names[j]);
        return vector<shared_ptr<ModelEmbedding>>{indctx->e1};
      });

      if (csr.res == smt::SolverResult::Unknown) {
        return cex;
      }

      if (csr.res == smt::SolverResult::Sat) {
        cex.is_false = csr.models[0];
        printf("counterexample type: SAFETY\n");
        return cex;
      }
    }
  }

  for (int j = 0; j < (int)module->actions.size(); j++) {
    ContextSolverResult csr = context_solve(
          "inductivity-check: " + module->action_names[j],
          module,
          options.minimal_models ? ModelType::Min : ModelType::Any,
          Strictness::TryHard,
          candidate /* hint */,
          [module, &options, cur_invariant, candidate, &conjectures, j](shared_ptr<BackgroundContext> bgctx)
    {
      auto indctx = get_counterexample_test_with_conjs_make_indctx(
            module, options, bgctx, cur_invariant, candidate, conjectures, j);

      smt::solver& solver = indctx->ctx->solver;

      solver.add(indctx->e2->value2expr(v_not(candidate)));

      solver.set_log_info(
          "inductivity-check: " + module->action_names[j]);

      return vector<shared_ptr<ModelEmbedding>>{indctx->e1, indctx->e2};
    });

    if (csr.res == smt::SolverResult::Unknown) {
      return cex;
    }

    if (csr.res == smt::SolverResult::Sat) {
      cex.hypothesis = csr.models[0];
      cex.conclusion = csr.models[1];
      printf("counterexample type: INDUCTIVE\n");
      return cex;
    }
  }

  cex.none = true;
  return cex;
}

Counterexample get_counterexample_simple(
    shared_ptr<Module> module,
    Options const& options,
    BMCContext& bmc,
    bool check_implies_conj,
    vector<value> const& conjs,
    value cur_invariant,
    value candidate)
{
  Counterexample cex;
  cex.none = false;

  ContextSolverResult csr = context_solve(
      "init-check",
      module,
      options.minimal_models ? ModelType::Min : ModelType::Any,
      Strictness::TryHard,
      candidate /* hint */,
      [module, candidate](shared_ptr<BackgroundContext> bgctx)
  {
    auto initctx = shared_ptr<InitContext>(new InitContext(bgctx, module));
    smt::solver& init_solver = initctx->ctx->solver;
    init_solver.push();
    init_solver.add(initctx->e->value2expr(v_not(candidate)));
    init_solver.set_log_info("init-check");
    return vector<shared_ptr<ModelEmbedding>>{initctx->e};
  });

  if (csr.res == smt::SolverResult::Unknown) {
    return cex;
  }

  if (csr.res == smt::SolverResult::Sat) {
    cex.is_true = csr.models[0];
    printf("counterexample type: INIT\n");
    return cex;
  }

  if (check_implies_conj) {
    ContextSolverResult csr = context_solve(
      "conj-check",
      module,
      options.minimal_models ? ModelType::Min : ModelType::Any,
      Strictness::TryHard,
      candidate /* hint */,
      [module, candidate, &conjs, cur_invariant](shared_ptr<BackgroundContext> bgctx)
    {
      auto conjctx = shared_ptr<BasicContext>(new BasicContext(bgctx, module));

      smt::solver& conj_solver = conjctx->ctx->solver;
      conj_solver.add(conjctx->e->value2expr(v_not(v_and(conjs))));
      if (cur_invariant) {
        conj_solver.add(conjctx->e->value2expr(cur_invariant));
      }
      conj_solver.add(conjctx->e->value2expr(candidate));
      return vector<shared_ptr<ModelEmbedding>>{conjctx->e};
    });

    if (csr.res == smt::SolverResult::Unknown) {
      return cex;
    }

    if (csr.res == smt::SolverResult::Sat) {
      cex.is_false = csr.models[0];
      printf("counterexample type: SAFETY\n");
      return cex;
    }
  }

  if (options.pre_bmc) {
    Counterexample bmc_cex = get_bmc_counterexample(bmc, candidate, options);
    if (!bmc_cex.none) {
      return bmc_cex;
    }
  }

  for (int j = 0; j < (int)module->actions.size(); j++) {
    ContextSolverResult csr = context_solve(
      "inductivity-check: " + module->action_names[j],
      module,
      options.minimal_models ? ModelType::Min : ModelType::Any,
      Strictness::TryHard,
      candidate /* hint */,
      [module, candidate, j, cur_invariant](shared_ptr<BackgroundContext> bgctx)
    {
      auto indctx = shared_ptr<InductionContext>(new InductionContext(bgctx, module, j));
      smt::solver& solver = indctx->ctx->solver;
      solver.push();
      if (cur_invariant) {
        solver.add(indctx->e1->value2expr(cur_invariant));
      }
      solver.add(indctx->e1->value2expr(candidate));
      solver.add(indctx->e2->value2expr(v_not(candidate)));

      return vector<shared_ptr<ModelEmbedding>>{indctx->e1, indctx->e2};
    });

    if (csr.res == smt::SolverResult::Unknown) {
      return cex;
    }

    if (csr.res == smt::SolverResult::Sat) {
      cex.hypothesis = csr.models[0];
      cex.conclusion = csr.models[1];
      printf("counterexample type: INDUCTIVE\n");
      return cex;
    }
  }

  cex.none = true;

  return cex;
}

bool conjectures_inv(
    shared_ptr<Module> module,
    vector<value> const& filtered_simplified_strengthened_invs,
    vector<value> const& conjectures)
{
  return is_invariant_wrt(
      module,
      v_and(filtered_simplified_strengthened_invs),
      conjectures);
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

bool invariant_is_nonredundant(shared_ptr<Module> module, vector<value> existingInvariants, value newInvariant)
{
  ContextSolverResult csr = context_solve(
      "redundancy check",
      module,
      ModelType::Any,
      Strictness::TryHard,
      nullptr,
      [module, &existingInvariants, newInvariant](shared_ptr<BackgroundContext> bgctx)
  {
    BasicContext basic(bgctx, module);
    for (value v : existingInvariants) {
      basic.ctx->solver.add(basic.e->value2expr(v));
    }
    basic.ctx->solver.add(basic.e->value2expr(v_not(newInvariant)));
    return vector<shared_ptr<ModelEmbedding>>{};
  });

  // Assume it's nonredundant in the Unknown case.
  return csr.res != smt::SolverResult::Unsat;
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

int numEnumeratedFilteredRedundantInvariants = 0;

void dump_stats(long long progress, CexStats const& cs,
    std::chrono::time_point<std::chrono::high_resolution_clock> init,
    int num_redundant, int num_nonredundant,
    long long filtering_ms, long long num_finishers_found,
    long long addCounterexample_ms, long long addCounterexample_count,
    long long cex_process_ns,
    long long redundant_process_ns,
    long long nonredundant_process_ns,
    long long indef_count,
    long long indef_ns) {
  cout << "================= Stats =================" << endl;
  cout << "progress: " << progress << endl;
  cout << "total time running so far: " << as_ms(now() - init)
       << " ms" << endl;
  cout << "total time filtering: " << filtering_ms << " ms" << endl;
  cout << "total time addCounterexample: " << addCounterexample_ms << " ms" << endl;
  if (addCounterexample_count > 0) {
    cout << "avg time addCounterexample: " << (addCounterexample_ms/addCounterexample_count) << " ops" << endl;
  }
  cout << "total time processing counterexamples: " << cex_process_ns / 1000000 << " ms" << endl;
  cout << "total time processing redundant: " << redundant_process_ns / 1000000 << " ms" << endl;
  cout << "total time processing nonredundant: " << nonredundant_process_ns / 1000000 << " ms" << endl;
  cout << "total time processing indef: " << indef_ns / 1000000 << " ms" << endl;
  cs.dump();
  cout << "number of redundant invariants found: "
       << num_redundant << endl;
  cout << "number of non-redundant invariants found: "
       << num_nonredundant << endl;
  cout << "number of finisher invariants found: "
       << num_finishers_found << endl;
  cout << "number of retries: " << numRetries << endl;
  cout << "number of TryHard failures: " << numTryHardFailures << endl;
  cout << "number of candidates could not determine inductiveness: " << indef_count << endl;
  cout << "number of enumerated filtered redundant invariants: " << numEnumeratedFilteredRedundantInvariants << endl;
  smt::dump_smt_stats();
  cout << "=========================================" << endl;
  cout.flush();
}

extern const int TIMEOUT = 45 * 1000;

SynthesisResult synth_loop(
  shared_ptr<Module> module,
  vector<TemplateSubSlice> const& slices,
  Options const& options,
  FormulaDump const& fd)
{
  shared_ptr<CandidateSolver> cs = make_candidate_solver(
      module, slices, false);

  SynthesisResult synres;
  synres.done = false;

  auto t_init = now();

  smt::context bmcctx(smt::Backend::z3);

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

  value cur_invariant = v_and(fd.base_invs);

  //Transcript transcript;

  CexStats cexstats;

  if (options.get_space_size) {
    long long s = cs->getSpaceSize();
    cout << "space size: " << s << endl;
    exit(0);
  }

  long long filtering_ns = 0;

  long long num_finishers_found = 0;
  long long process_cex_ns = 0;

  long long process_indef_ns = 0;
  long long indef_count = 0;

  while (true) {
    num_iterations++;

    printf("\n");

    //log_smtlib(solver_sf);
    //printf("number of boolean variables: %d\n", sf.get_bool_count());
    cout << "num iterations " << num_iterations << endl;
    std::cout.flush();

    auto filtering_t1 = now();
    value candidate = cs->getNext();
    filtering_ns += as_ns(now() - filtering_t1);

    auto process_start_t = now();

    if (!candidate) {
      printf("unable to synthesize any formula\n");
      break;
    }

    cout << "candidate: " << candidate->to_string() << endl;

    Counterexample cex;
    if (options.with_conjs) {
      cex = get_counterexample_test_with_conjs(module, options, cur_invariant, candidate, fd.conjectures);
      if (!cex.is_valid()) {
        long long process_ns = as_ns(now() - process_start_t);
        process_indef_ns += process_ns;
        indef_count++;

        cout << "WARNING: COULD NOT DETERMINE INDUCTIVENESS" << endl;
        continue;
      }
      cex = simplify_cex_nosafety(module, cex, options, bmc);
    } else {
      cex = get_counterexample_simple(module, options, bmc, true, fd.conjectures, nullptr, candidate);
      if (!cex.is_valid()) {
        long long process_ns = as_ns(now() - process_start_t);
        process_indef_ns += process_ns;
        indef_count++;

        cout << "WARNING: COULD NOT DETERMINE INDUCTIVENESS" << endl;
        continue;
      }
      cex = simplify_cex(module, cex, options, bmc, antibmc);
    }

    cexstats.add(cex);

    if (cex.none) {
      num_finishers_found++;

      // Extra verification:
      // (If we get here, then it should definitely be invariant,
      // this is just a sanity check that our code is right.)
      bool is_inv;
      if (options.with_conjs) {
        vector<value> conjs = module->conjectures;
        conjs.push_back(candidate);
        vector_append(conjs, fd.base_invs);
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
      cex_stats(cex);
      cs->addCounterexample(cex);
      //transcript.entries.push_back(make_pair(cex, candidate));
    }

    long long process_ns = as_ns(now() - process_start_t);
    if (!cex.none) {
      process_cex_ns += process_ns;
    }

    dump_stats(cs->getProgress(), cexstats, t_init, 0, 0, filtering_ns/1000000, num_finishers_found, 0, 0, process_cex_ns, 0, 0, indef_count, process_indef_ns);
  }

  //cout << transcript.to_json().dump() << endl;
  dump_stats(cs->getProgress(), cexstats, t_init, 0, 0, filtering_ns/1000000, num_finishers_found, 0, 0, process_cex_ns, 0, 0, indef_count, process_indef_ns);
  cout << "complete!" << endl;

  return synres;
}

template <typename T>
vector<T> vector_concat(vector<T> const& a, vector<T> const& b)
{
  vector<T> res = a;
  for (int i = 0; i < (int)b.size(); i++) {
    res.push_back(b[i]);
  }
  return res;
}

SynthesisResult synth_loop_incremental_breadth(
    shared_ptr<Module> module,
    vector<TemplateSubSlice> const& slices,
    Options const& options,
    FormulaDump const& fd,
    bool single_round)
{
  auto t_init = now();

  smt::context bmcctx(smt::Backend::z3);

  if (fd.conjectures.size() == 0) {
    cout << "already invariant, done" << endl;
    return SynthesisResult(true, {}, {});
  }

  vector<value> all_invs = fd.all_invs;
  vector<value> new_invs = fd.new_invs;
  //vector<value> strengthened_invs = all_invs;
  //vector<value> filtered_simplified_strengthened_invs = new_invs;
  vector<value> base_invs_plus_new_invs = vector_concat(
      fd.base_invs, fd.new_invs);
  vector<value> conjs_plus_base_invs_plus_new_invs = vector_concat(
      fd.conjectures, base_invs_plus_new_invs);
  vector<value> base_invs_plus_conjs = vector_concat(
      fd.base_invs, fd.conjectures);

  int bmc_depth = 4;
  printf("bmc_depth = %d\n", bmc_depth);
  BMCContext bmc(bmcctx, module, bmc_depth);
  bmcctx.set_timeout(15000); // 15 seconds

  int num_iterations_total = 0;
  int num_iterations_outer = 0;
  int num_redundant = 0;
  int num_nonredundant = 0;

  CexStats cexstats;

  long long filtering_ns = 0;
  long long addCounterexample_ns = 0;
  long long addCounterexample_count = 0;

  long long cex_process_ns = 0;
  long long redundant_process_ns = 0;
  long long nonredundant_process_ns = 0;

  long long process_indef_ns = 0;
  long long indef_count = 0;

  while (true) {
    num_iterations_outer++;

    shared_ptr<CandidateSolver> cs = make_candidate_solver(module, slices, true);

    if (options.get_space_size) {
      long long s = cs->getSpaceSize();
      cout << "space size: " << s << endl;
      exit(0);
    }

    for (value inv : all_invs) {
      cs->addExistingInvariant(inv);
    }

    int num_iterations = 0;
    bool any_formula_synthesized_this_round = false;

    while (true) {
      num_iterations++;
      num_iterations_total++;

      cout << endl;

      auto filtering_t1 = now();
      value candidate0 = cs->getNext();
      filtering_ns += as_ns(now() - filtering_t1);

      auto process_start_t = now();

      if (!candidate0) {
        break;
      }

      cout << "total iterations: " << num_iterations_total << " (" << num_iterations_outer << " outer + " << num_iterations << ")" << endl;
      cout << "candidate: " << candidate0->to_string() << endl;

      value candidate = candidate0->reduce_quants();

      Counterexample cex = get_counterexample_simple(
                module, options, bmc, false /* check_implies_conj */, fd.conjectures,
                v_and(
                  options.non_accumulative
                    ? (options.breadth_with_conjs ? base_invs_plus_conjs : fd.base_invs)
                    : (options.breadth_with_conjs ? conjs_plus_base_invs_plus_new_invs : base_invs_plus_new_invs)
                ),
                candidate);

      if (!cex.is_valid()) {
        long long process_ns = as_ns(now() - process_start_t);
        process_indef_ns += process_ns;
        indef_count++;

        cout << "WARNING: COULD NOT DETERMINE INDUCTIVENESS" << endl;
        continue;
      }

      Benchmarking bench2;
      bench2.start("simplification");
      cex = simplify_cex_nosafety(module, cex, options, bmc);
      bench2.end();
      bench2.dump();

      assert(cex.is_false == nullptr);

      cexstats.add(cex);

      bool is_nonredundant;
      if (cex.none) {
        //Benchmarking bench_strengthen;
        //bench_strengthen.start("strengthen");
        value strengthened_inv = options.non_accumulative ? candidate0 :
            strengthen_invariant(module,
              v_and(
                (options.breadth_with_conjs ? conjs_plus_base_invs_plus_new_invs : base_invs_plus_new_invs)
              ), candidate0);
        value simplified_strengthened_inv = strengthened_inv->simplify()->reduce_quants();
        //bench_strengthen.end();
        //bench_strengthen.dump();
        cout << "strengthened " << strengthened_inv->to_string() << endl;

        is_nonredundant = invariant_is_nonredundant(module,
            (options.breadth_with_conjs ? conjs_plus_base_invs_plus_new_invs : base_invs_plus_new_invs),
            simplified_strengthened_inv);

        all_invs.push_back(strengthened_inv);

        if (is_nonredundant) {
          any_formula_synthesized_this_round = true;

          base_invs_plus_new_invs.push_back(simplified_strengthened_inv);
          conjs_plus_base_invs_plus_new_invs.push_back(simplified_strengthened_inv);
          new_invs.push_back(simplified_strengthened_inv);

          cout << "\nfound new invariant! all so far:\n";
          for (value found_inv : new_invs) {
            cout << "    " << found_inv->to_string() << endl;
          }

          //if (!options.whole_space && is_invariant_with_conjectures(module, filtered_simplified_strengthened_invs)) {
          /*if (!options.whole_space && conjectures_inv(module, filtered_simplified_strengthened_invs, conjectures)) {
            cout << "invariant implies safety condition, done!" << endl;
            dump_stats(cs->getProgress(), cexstats, t_init, num_redundant, filtering_ns/1000000, 0);
            return SynthesisResult(true, filtered_simplified_strengthened_invs, all_invs);
          }*/
          num_nonredundant++;
        } else {
          cout << "invariant is redundant" << endl;
          num_redundant++;
        }

        cs->addExistingInvariant(strengthened_inv);
      } else {
        cex_stats(cex);
        auto t1 = now();
        cs->addCounterexample(cex);
        auto t2 = now();
        addCounterexample_ns += as_ns(t2 - t1);
        addCounterexample_count++;
        cout << "addCounterexample: " << addCounterexample_ns / 1000000 << endl;
      }

      long long process_ns = as_ns(now() - process_start_t);
      if (!cex.none) {
        cex_process_ns += process_ns;
      } else if (is_nonredundant) {
        nonredundant_process_ns += process_ns;
      } else {
        redundant_process_ns += process_ns;
      }

      dump_stats(cs->getProgress(), cexstats, t_init, num_redundant, num_nonredundant, filtering_ns/1000000, 0, addCounterexample_ns / 1000000, addCounterexample_count, cex_process_ns, redundant_process_ns, nonredundant_process_ns, indef_count, process_indef_ns);
    }

    dump_stats(cs->getProgress(), cexstats, t_init, num_redundant, num_nonredundant, filtering_ns/1000000, 0, addCounterexample_ns / 1000000, addCounterexample_count, cex_process_ns, redundant_process_ns, nonredundant_process_ns, indef_count, process_indef_ns);

    if (!any_formula_synthesized_this_round) {
      cout << "unable to synthesize any formula" << endl;
      break;
    }

    //if (!options.whole_space && conjectures_inv(module, new_invs, conjectures))
    if (!options.whole_space && is_invariant_wrt(module,
            v_and(base_invs_plus_new_invs), fd.conjectures))
    {
      cout << "invariant implies safety condition, done!" << endl;
      dump_stats(cs->getProgress(), cexstats, t_init, num_redundant, num_nonredundant, filtering_ns/1000000, 0, addCounterexample_ns / 1000000, addCounterexample_count, cex_process_ns, redundant_process_ns, nonredundant_process_ns, indef_count, process_indef_ns);
      return SynthesisResult(true, new_invs, all_invs);
    }

    if (single_round) {
      cout << "terminating because of single_round mode" << endl;
      break;
    }
  }

  return SynthesisResult(false, new_invs, all_invs);
}
