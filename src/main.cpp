#include "logic.h"
#include "contexts.h"
#include "model.h"
#include "grammar.h"
#include "expr_gen_smt.h"
#include "benchmarking.h"
#include "bmc.h"
#include "enumerator.h"
#include "utils.h"
#include "synth_loop.h"
#include "sat_solver.h"
#include "wpr.h"

#include <iostream>
#include <iterator>
#include <string>
#include <cstdlib>
#include <cstdio>

using namespace std;

bool do_invariants_imply_conjecture(shared_ptr<ConjectureContext> conjctx) {
  smt::solver& solver = conjctx->ctx->solver;
  return !solver.check_sat();
}

bool is_redundant(
    shared_ptr<InvariantsContext> invctx,
    shared_ptr<Value> formula)
{
  smt::solver& solver = invctx->ctx->solver;
  solver.push();
  solver.add(invctx->e->value2expr(shared_ptr<Value>(new Not(formula))));

  bool res = !solver.check_sat();
  solver.pop();
  return res;
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

  /*cout << "list:" << endl;
  for (value conj : all_conjs) {
    for (value part : aggressively_split_into_conjuncts(conj)) {
      cout << part->to_string() << endl;
    }
  }
  cout << endl;*/

  cout << "wpr: " << w->to_string() << endl;

  if (is_itself_invariant(module, all_conjs)) {
  //if (is_wpr_itself_inductive(module, conj, count)) {
    printf("yes\n");
  } else{
    printf("no\n");
  }
}

int run_id;
extern bool enable_smt_logging;

struct Strategy {
  bool finisher;
  bool inc;
  bool breadth;
  EnumOptions enum_options;

  Strategy() {
    finisher = false;
    inc = false;
    breadth = false;

    enum_options.template_idx = 0;
    enum_options.arity = -1;
    enum_options.depth = -1;
    enum_options.conj = false;
    enum_options.conj_arity = -1;
    enum_options.disj_arity = -1;
    enum_options.impl_shape = false;
    //options.strat2 = false;
    enum_options.strat_alt = false;
  }
};

void input_chunks(string const& filename, vector<SpaceChunk>& chunks)
{
  FILE* f = fopen(filename.c_str(), "r");
  assert(f != NULL);
  int num;
  fscanf(f, "%d", &num);
  for (int i = 0; i < num; i++) {
    int major_idx, size, count;
    fscanf(f, "%d", &major_idx);
    fscanf(f, "%d", &size);
    fscanf(f, "%d", &count);
    SpaceChunk sc;
    sc.major_idx = major_idx;
    sc.size = size;
    for (int j = 0; j < count; j++) {
      int x;
      fscanf(f, "%d", &x);
      sc.nums.push_back(x);
    }
    chunks.push_back(sc);
  }
  fclose(f);
}

void output_chunks(string const& filename, vector<SpaceChunk> const& chunks)
{
  FILE* f = fopen(filename.c_str(), "w");
  fprintf(f, "%d\n", (int)chunks.size());
  for (SpaceChunk const& sc : chunks) {
    fprintf(f, "%d %d %d", sc.major_idx, sc.size, (int)sc.nums.size());
    for (int j : sc.nums) {
      fprintf(f, " %d", j);
    }
    fprintf(f, "\n");
  }
  fclose(f);
}

void output_chunks_mult(
  vector<string> const& output_chunk_files,
  vector<SpaceChunk> const& chunks)
{
  for (int i = 0; i < (int)output_chunk_files.size(); i++) {
    vector<SpaceChunk> slice;
    for (int j = i; j < (int)chunks.size(); j += output_chunk_files.size())
    {
      slice.push_back(chunks[j]);
    }
    output_chunks(output_chunk_files[i], slice);
  }
}

int main(int argc, char* argv[]) {
  std::istreambuf_iterator<char> begin(std::cin), end;
  std::string json_src(begin, end);

  shared_ptr<Module> module = parse_module(json_src);

  cout << "conjectures:" << endl;
  for (value v : module->conjectures) {
    cout << v->to_string() << endl;
  }

  srand((int)time(NULL));
  run_id = rand();

  Options options;
  options.enum_sat = false;
  options.with_conjs = false;
  options.whole_space = false;
  options.pre_bmc = false;
  options.post_bmc = false;
  options.get_space_size = false;
  options.minimal_models = false;
  //options.threads = 1;

  vector<string> output_chunk_files;
  string input_chunk_file;
  
  int seed = 1234;
  bool check_inductiveness = false;
  bool check_rel_inductiveness = false;
  bool check_implication = false;
  bool wpr = false;
  int wpr_index = 0;

  int i;
  for (i = 1; i < argc; i++) {
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
    else if (argv[i] == string("--check-rel-inductiveness")) {
      check_rel_inductiveness = true;
    }
    else if (argv[i] == string("--check-implication")) {
      check_implication = true;
    }
    else if (argv[i] == string("--finisher")) {
      break;
    }
    else if (argv[i] == string("--incremental")) {
      break;
    }
    else if (argv[i] == string("--breadth")) {
      break;
    }
    else if (argv[i] == string("--whole-space")) {
      options.whole_space = true;
    }
    else if (argv[i] == string("--enum-sat")) {
      options.enum_sat = true;
    }
    else if (argv[i] == string("--with-conjs")) {
      options.with_conjs = true;
    }
    else if (argv[i] == string("--log-smt-files")) {
      enable_smt_logging = true;
    }
    else if (argv[i] == string("--pre-bmc")) {
      options.pre_bmc = true;
    }
    else if (argv[i] == string("--post-bmc")) {
      options.post_bmc = true;
    }
    else if (argv[i] == string("--get-space-size")) {
      options.get_space_size = true;
    }
    else if (argv[i] == string("--minimal-models")) {
      options.minimal_models = true;
    }
    else if (argv[i] == string("--output-chunk-file")) {
      assert(i + 1 < argc);
      output_chunk_files.push_back(argv[i+1]);
      i++;
    }
    else if (argv[i] == string("--input-chunk-file")) {
      assert(i + 1 < argc);
      input_chunk_file = argv[i+1];
      i++;
    }
    /*else if (argv[i] == string("--threads")) {
      assert(i + 1 < argc);
      options.threads = atoi(argv[i+1]);
      i++;
    }*/
    else {
      cout << "unreocgnized argument " << argv[i] << endl;
      return 1;
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

  if (check_rel_inductiveness) {
    printf("just inductiveness of the last one...\n");
    value v = module->conjectures[module->conjectures.size() - 1];
    vector<value> others;
    for (int i = 0; i < (int)module->conjectures.size() - 1; i++) {
      others.push_back(module->conjectures[i]);
    }
    if (is_invariant_wrt(module, v_and(others), v)) {
      printf("yes\n");
    } else{
      printf("no\n");
    }
    return 0;
  }

  if (check_implication) {
    printf("just checking inductiveness...\n");
    vector<value> vs;
    for (int i = 0; i < (int)module->conjectures.size(); i++) {
      vs.push_back(i == 0 ? v_not(module->conjectures[i]) :
          module->conjectures[i]);
    }
    if (is_satisfiable(module, v_and(vs))) {
      printf("first is NOT implied by the rest\n");
    } else{
      printf("first IS implied by the rest\n");
    }
    return 0;
  }

  vector<Strategy> strats;
  while (i < argc) {
    Strategy strat;
    if (argv[i] == string("--finisher")) {
      strat.finisher = true;
    }
    else if (argv[i] == string("--incremental")) {
      strat.inc = true;
    }
    else if (argv[i] == string("--breadth")) {
      strat.breadth = true;
    }
    else {
      assert (false);
    }

    for (i++; i < argc; i++) {
      if (argv[i] == string("--finisher")) {
        break;
      }
      else if (argv[i] == string("--incremental")) {
        break;
      }
      else if (argv[i] == string("--breadth")) {
        break;
      }
      else if (argv[i] == string("--template")) {
        assert(i + 1 < argc);
        strat.enum_options.template_idx = atoi(argv[i+1]);
        assert(0 <= strat.enum_options.template_idx
            && strat.enum_options.template_idx < (int)module->templates.size());
        i++;
      }
      else if (argv[i] == string("--arity")) {
        assert(i + 1 < argc);
        strat.enum_options.arity = atoi(argv[i+1]);
        i++;
      }
      else if (argv[i] == string("--depth")) {
        assert(i + 1 < argc);
        strat.enum_options.depth = atoi(argv[i+1]);
        i++;
      }
      else if (argv[i] == string("--conj")) {
        strat.enum_options.conj = true;
      }
      else if (argv[i] == string("--conj-arity")) {
        assert(i + 1 < argc);
        strat.enum_options.conj_arity = atoi(argv[i+1]);
        i++;
      }
      else if (argv[i] == string("--disj-arity")) {
        assert(i + 1 < argc);
        strat.enum_options.disj_arity = atoi(argv[i+1]);
        i++;
      }
      else if (argv[i] == string("--impl-shape")) {
        strat.enum_options.impl_shape = true;
      }
      else if (argv[i] == string("--strat-alt")) {
        strat.enum_options.strat_alt = true;
      }
      else {
        cout << "unreocgnized enum_options argument " << argv[i] << endl;
        return 1;
      }
    }

    strats.push_back(strat);
  }

  if (strats.size() == 0) {
    cout << "No strategy given ???" << endl;
    return 1;
  }

  if (output_chunk_files.size() || input_chunk_file != "") {
    for (int i = 1; i < (int)strats.size(); i++) {
      assert (strats[0].inc == strats[i].inc);
      assert (strats[0].breadth == strats[i].breadth);
      assert (strats[0].finisher == strats[i].finisher);
    }
  }

  if (output_chunk_files.size() > 0) {
    vector<EnumOptions> enum_options;
    for (Strategy const& strat : strats) {
      enum_options.push_back(strat.enum_options);
    }
    shared_ptr<CandidateSolver> cs = make_candidate_solver(
        module, options.enum_sat, enum_options, !strats[0].finisher);
    vector<SpaceChunk> chunks;
    cs->getSpaceChunk(chunks /* output */);
    output_chunks_mult(output_chunk_files, chunks);
    return 0;
  }

  vector<SpaceChunk> chunks;
  bool use_input_chunks = false;
  if (input_chunk_file != "") {
    input_chunks(input_chunk_file, chunks);
    use_input_chunks = true;
  }

  printf("random seed = %d\n", seed);
  srand(seed);

  try {
    int idx;
    if (strats[0].inc || strats[0].breadth) {
      vector<EnumOptions> enum_options;
      int i;
      for (i = 0; i < (int)strats.size(); i++) {
        if (strats[i].inc || strats[i].breadth) {
          assert (strats[0].inc == strats[i].inc);
          assert (strats[0].breadth == strats[i].breadth);
          enum_options.push_back(strats[i].enum_options);
        } else {
          break;
        }
      }
      idx = i;

      SynthesisResult synres;
      if (strats[0].inc) {
        cout << endl;
        cout << ">>>>>>>>>>>>>> Starting incremental algorithm" << endl;
        cout << endl;
        synres = synth_loop_incremental(module, enum_options, options);
      } else {
        cout << endl;
        cout << ">>>>>>>>>>>>>> Starting breadth algorithm" << endl;
        cout << endl;
        synres = synth_loop_incremental_breadth(module, enum_options, options);
      }

      if (synres.done) {
        cout << "Synthesis success!" << endl;
        return 0;
      }

      module = module->add_conjectures(synres.new_values);
    } else {
      idx = 0;
    }

    if (idx < (int)strats.size()) {
      vector<EnumOptions> enum_options;
      for (int i = idx; i < (int)strats.size(); i++) {
        assert (strats[i].finisher);
        enum_options.push_back(strats[i].enum_options);
      }

      cout << endl;
      cout << ">>>>>>>>>>>>>> Starting finisher algorithm" << endl;
      cout << endl;
      synth_loop(module, enum_options, options,
          use_input_chunks, chunks);
    }

    return 0;
  } catch (z3::exception exc) {
    printf("got z3 exception: %s\n", exc.msg());
    throw;
  }
}
