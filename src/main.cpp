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
#include "wpr.h"
#include "filter.h"
#include "template_counter.h"

#include <iostream>
#include <iterator>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <fstream>

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
  bool breadth;
  EnumOptions enum_options;

  Strategy() {
    finisher = false;
    breadth = false;

    enum_options.template_idx = 0;
    enum_options.disj_arity = -1;
    enum_options.depth2_shape = false;
  }
};

void input_chunks(string const& filename, vector<SpaceChunk>& chunks, int chunk_size_to_use)
{
  FILE* f = fopen(filename.c_str(), "r");
  assert(f != NULL);
  int num;
  fscanf(f, "%d", &num);
  for (int i = 0; i < num; i++) {
    int major_idx, size, tree_idx, count;
    fscanf(f, "%d", &major_idx);
    fscanf(f, "%d", &size);
    fscanf(f, "%d", &tree_idx);
    fscanf(f, "%d", &count);
    SpaceChunk sc;
    sc.major_idx = major_idx;
    sc.tree_idx = tree_idx;
    sc.size = size;
    for (int j = 0; j < count; j++) {
      int x;
      fscanf(f, "%d", &x);
      sc.nums.push_back(x);
    }
    if (chunk_size_to_use == -1 || sc.size == chunk_size_to_use) {
      chunks.push_back(sc);
    }
  }
  fclose(f);
}

void output_chunks(string const& filename, vector<SpaceChunk> const& chunks)
{
  FILE* f = fopen(filename.c_str(), "w");
  fprintf(f, "%d\n", (int)chunks.size());
  for (SpaceChunk const& sc : chunks) {
    fprintf(f, "%d %d %d %d", sc.major_idx, sc.size, sc.tree_idx, (int)sc.nums.size());
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

template <typename T>
void random_sort(vector<T>& v, int a, int b)
{
  for (int i = a; i < b-1; i++) {
    int j = i + (rand() % (b - i));
    T tmp = v[i];
    v[i] = v[j];
    v[j] = tmp;
  }
}

void randomize_chunks(vector<SpaceChunk>& chunks)
{
  // Randomly sort the chunks, but make sure they stay
  // nondecreasing in size.
  int a = 0;
  while (a < (int)chunks.size()) {
    int b = a+1;
    while (b < (int)chunks.size() && chunks[b].size == chunks[a].size) {
      b++;
    }
    random_sort(chunks, a, b);
    a = b;
  }
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
}

FormulaDump read_formula_dump(string const& filename)
{
  ifstream f;
  f.open(filename);
  std::istreambuf_iterator<char> begin(f), end;
  std::string json_src(begin, end);
  FormulaDump fd = parse_formula_dump(json_src);
  return fd;
}

FormulaDump get_default_formula_dump(shared_ptr<Module> module)
{
  FormulaDump fd;
  fd.success = false;
  split_into_invariants_conjectures(
      module,
      fd.base_invs /* output */,
      fd.conjectures /* output */);
  return fd;
}

void write_formulas(string const& filename, FormulaDump const& fd)
{
  ofstream f;
  f.open(filename);
  string s = marshall_formula_dump(fd);
  f << s;
  f << endl;
}

void augment_fd(FormulaDump& fd, SynthesisResult const& synres)
{
  if (synres.done) fd.success = true;
  for (value v : synres.new_values) {
    fd.new_invs.push_back(v);
  }
  for (value v : synres.all_values) {
    fd.all_invs.push_back(v);
  }
}

shared_ptr<Module> read_module(string const& module_filename)
{
  ifstream f;
  f.open(module_filename);
  std::istreambuf_iterator<char> begin(f), end;
  std::string json_src(begin, end);
  return parse_module(json_src);
}

int main(int argc, char* argv[]) {
  for (int i = 0; i < argc; i++) {
    cout << argv[i] << " ";
  }
  cout << endl;

  srand((int)time(NULL));
  run_id = rand();

  Options options;
  options.with_conjs = false;
  options.breadth_with_conjs = false;
  options.whole_space = false;
  options.pre_bmc = false;
  options.post_bmc = false;
  options.get_space_size = false;
  options.minimal_models = false;
  options.smt_retries = true;
  options.non_accumulative = false;
  //options.threads = 1;

  vector<string> output_chunk_files;
  string input_chunk_file;

  vector<string> input_formula_files;
  string output_formula_file;

  string module_filename;
  
  int seed = 1234;
  bool check_inductiveness = false;
  bool check_rel_inductiveness = false;
  bool check_implication = false;
  bool wpr = false;
  int wpr_index = 0;

  bool coalesce = false;
  int chunk_size_to_use = -1;

  bool template_counter = false;
  int template_counter_k;
  int template_counter_d;

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
    else if (argv[i] == string("--breadth")) {
      break;
    }
    else if (argv[i] == string("--whole-space")) {
      options.whole_space = true;
    }
    else if (argv[i] == string("--no-smt-retries")) {
      options.smt_retries = false;
    }
    else if (argv[i] == string("--non-accumulative")) {
      options.non_accumulative = true;
    }
    else if (argv[i] == string("--with-conjs")) {
      options.with_conjs = true;
    }
    else if (argv[i] == string("--breadth-with-conjs")) {
      options.breadth_with_conjs = true;
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
    else if (argv[i] == string("--coalesce")) {
      coalesce = true;
    }
    else if (argv[i] == string("--minimal-models")) {
      options.minimal_models = true;
    }
    else if (argv[i] == string("--output-chunk-file")) {
      assert(i + 1 < argc);
      output_chunk_files.push_back(argv[i+1]);
      i++;
    }
    else if (argv[i] == string("--input-module")) {
      assert(i + 1 < argc);
      assert(module_filename == "");
      module_filename = argv[i+1];
      i++;
    }
    else if (argv[i] == string("--input-chunk-file")) {
      assert(i + 1 < argc);
      assert(input_chunk_file == "");
      input_chunk_file = argv[i+1];
      i++;
    }
    else if (argv[i] == string("--output-formula-file")) {
      assert(i + 1 < argc);
      assert(output_formula_file == "");
      output_formula_file = argv[i+1];
      i++;
    }
    else if (argv[i] == string("--input-formula-file")) {
      assert(i + 1 < argc);
      input_formula_files.push_back(argv[i+1]);
      i++;
    }
    else if (argv[i] == string("--chunk-size-to-use")) {
      assert(i + 1 < argc);
      chunk_size_to_use = atoi(argv[i+1]);
      i++;
    }
    else if (argv[i] == string("--template-counter")) {
      assert(i + 2 < argc);
      template_counter = true;
      template_counter_k = atoi(argv[i+1]);
      template_counter_d = atoi(argv[i+2]);
      i += 2;
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

  shared_ptr<Module> module = read_module(module_filename);

  cout << "conjectures:" << endl;
  for (value v : module->conjectures) {
    cout << v->to_string() << endl;
  }

  if (template_counter) {
    assert (template_counter_d == 1 || template_counter_d == 2);
    long long res = count_template(module,
        module->templates[0],
        template_counter_k,
        template_counter_d == 2,
        false);
    cout << "total: " << res << endl;
    return 0;
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
    } else {
      printf("first IS implied by the rest\n");
    }
    return 0;
  }

  if (coalesce) {
    FormulaDump res_fd;
    res_fd.success = false;
    for (string const& filename : input_formula_files) {
      FormulaDump fd = read_formula_dump(filename);
      vector_append(res_fd.base_invs, fd.base_invs);
      vector_append(res_fd.new_invs, fd.new_invs);
      vector_append(res_fd.all_invs, fd.all_invs);
      vector_append(res_fd.conjectures, fd.conjectures);
      if (fd.success) res_fd.success = true;
    }

    res_fd.base_invs = filter_unique_formulas(res_fd.base_invs);
    res_fd.new_invs = filter_redundant_formulas(module, res_fd.new_invs);
    res_fd.all_invs = filter_unique_formulas(res_fd.all_invs);
    res_fd.conjectures = filter_unique_formulas(res_fd.conjectures);
    assert (output_formula_file != "");
    write_formulas(output_formula_file, res_fd);
    return 0;
  }

  vector<Strategy> strats;
  while (i < argc) {
    Strategy strat;
    if (argv[i] == string("--finisher")) {
      strat.finisher = true;
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
      else if (argv[i] == string("--disj-arity")) {
        assert(i + 1 < argc);
        strat.enum_options.disj_arity = atoi(argv[i+1]);
        i++;
      }
      else if (argv[i] == string("--depth2-shape")) {
        strat.enum_options.depth2_shape = true;
      }
      else {
        cout << "unreocgnized enum_options argument " << argv[i] << endl;
        return 1;
      }
    }

    strats.push_back(strat);
  }

  printf("random seed = %d\n", seed);
  srand(seed);

  if (strats.size() == 0) {
    cout << "No strategy given ???" << endl;
    return 1;
  }

  FormulaDump input_fd;
  assert (input_formula_files.size() <= 1);
  if (input_formula_files.size() == 1) {
    cout << "Reading in formulas from " << input_formula_files[0] << endl;
    input_fd = read_formula_dump(input_formula_files[0]);
  } else {
    cout << "Retrieving base_invs / conjectures from module spec" << endl;
    input_fd = get_default_formula_dump(module);
  }

  for (value v : input_fd.base_invs) {
    cout << "[invariant] " << v->to_string() << endl;
  }
  for (value v : input_fd.conjectures) {
    cout << "[conjecture] " << v->to_string() << endl;
  }
  cout << "|all_invs| = " << input_fd.all_invs.size() << endl;
  cout << "|new_invs| = " << input_fd.new_invs.size() << endl;

  if (output_chunk_files.size() || input_chunk_file != "") {
    for (int i = 1; i < (int)strats.size(); i++) {
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
        module, enum_options, !strats[0].finisher);
    vector<SpaceChunk> chunks;
    cs->getSpaceChunk(chunks /* output */);
    randomize_chunks(chunks);
    output_chunks_mult(output_chunk_files, chunks);
    return 0;
  }

  vector<SpaceChunk> chunks;
  bool use_input_chunks = false;
  if (input_chunk_file != "") {
    input_chunks(input_chunk_file, chunks, chunk_size_to_use);
    use_input_chunks = true;
  }

  FormulaDump output_fd;
  output_fd.success = false;
  output_fd.base_invs = input_fd.base_invs;
  output_fd.conjectures = input_fd.conjectures;

  if (options.get_space_size) {
    vector<EnumOptions> enum_options_b;
    vector<EnumOptions> enum_options_f;
    int i;
    for (i = 0; i < (int)strats.size(); i++) {
      if (strats[i].breadth) {
        assert (strats[0].breadth == strats[i].breadth);
        enum_options_b.push_back(strats[i].enum_options);
      } else {
        break;
      }
    }
    for (; i < (int)strats.size(); i++) {
      if (strats[i].finisher) {
        enum_options_f.push_back(strats[i].enum_options);
      }
    }
    long long b_pre_symm = -1;
    long long b_post_symm = -1;
    long long f_pre_symm = -1;
    long long f_post_symm = -1;
    if (enum_options_b.size() > 0) {
      shared_ptr<CandidateSolver> cs =
          make_candidate_solver(module, enum_options_b, true);
      b_pre_symm = cs->getPreSymmCount();
      cout << "b_pre_symm " << b_pre_symm << endl;
      b_post_symm = cs->getSpaceSize();
      cout << "b_post_symm " << b_post_symm << endl;
    }
    if (enum_options_f.size() > 0) {
      shared_ptr<CandidateSolver> cs = make_candidate_solver(
          module, enum_options_f, false);
      f_pre_symm = cs->getPreSymmCount();
      cout << "f_pre_symm " << f_pre_symm << endl;
      f_post_symm = cs->getSpaceSize();
      cout << "f_post_symm " << f_post_symm << endl;
    }
    cout << "pre_symm counts ("
        << (b_pre_symm == -1 ? "None" : to_string(b_pre_symm))
        << ", "
        << (f_pre_symm == -1 ? "None" : to_string(f_pre_symm))
        << ")" << endl;
    cout << "post_symm counts ("
        << (b_post_symm == -1 ? "None" : to_string(b_post_symm))
        << ", "
        << (f_post_symm == -1 ? "None" : to_string(f_post_symm))
        << ")" << endl;
    return 0;
  }

  try {
    int idx;
    if (strats[0].breadth) {
      vector<EnumOptions> enum_options;
      int i;
      for (i = 0; i < (int)strats.size(); i++) {
        if (strats[i].breadth) {
          assert (strats[0].breadth == strats[i].breadth);
          enum_options.push_back(strats[i].enum_options);
        } else {
          break;
        }
      }
      idx = i;

      SynthesisResult synres;
      cout << endl;
      cout << ">>>>>>>>>>>>>> Starting breadth algorithm" << endl;
      cout << endl;
      synres = synth_loop_incremental_breadth(module, enum_options, options,
          use_input_chunks, chunks, input_fd);

      augment_fd(output_fd, synres);

      if (synres.done) {
        cout << "Synthesis success!" << endl;
        goto finish;
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
      SynthesisResult synres = synth_loop(module, enum_options, options,
          use_input_chunks, chunks, input_fd);

      augment_fd(output_fd, synres);

      if (synres.done) {
        cout << "Finisher algorithm: Synthesis success!" << endl;
      } else {
        cout << "Finisher algorithm unable to find invariant." << endl;
      }
    }

    finish:

    if (output_formula_file != "") {
      cout << "Writing result to " << output_formula_file << endl;
      write_formulas(output_formula_file, output_fd);
    }

    return 0;
  } catch (z3::exception exc) {
    printf("got z3 exception: %s\n", exc.msg());
    throw;
  }
}
