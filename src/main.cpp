#include "logic.h"
#include "contexts.h"
#include "model.h"
#include "benchmarking.h"
#include "bmc.h"
#include "enumerator.h"
#include "utils.h"
#include "synth_loop.h"
#include "wpr.h"
#include "filter.h"
#include "template_counter.h"
#include "template_priority.h"
#include "z3++.h"
#include "stats.h"

#include <iostream>
#include <iterator>
#include <string>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <fstream>

using namespace std;

Stats global_stats;

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

struct EnumOptions {
  int template_idx;
  string template_str;

  // Naive solving
  int disj_arity;
  bool depth2_shape;
};

struct Strategy {
  bool finisher;
  bool breadth;
  TemplateSpace tspace;

  Strategy() {
    finisher = false;
    breadth = false;
  }
};

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

vector<TemplateSubSlice> read_template_sub_slice_file(
    shared_ptr<Module> module,
    string const& filename)
{
  ifstream f;
  f.open(filename);
  int sz;
  f >> sz;
  int sorts_sz;
  f >> sorts_sz;
  for (int i = 0; i < sorts_sz; i++) {
    string so;
    f >> so;
    assert (so == module->sorts[i] && "template file uses wrong sort order");
  }
  vector<TemplateSubSlice> tds;
  for (int i = 0; i < sz; i++) {
    TemplateSubSlice td;
    f >> td;
    tds.push_back(td);
  }
  return tds;
}

void write_template_sub_slice_file(
    shared_ptr<Module> module,
    string const& filename,
    vector<TemplateSubSlice> const& tds) {
  ofstream f;
  f.open(filename);
  f << tds.size() << endl;
  f << module->sorts.size();
  for (string so : module->sorts) {
    f << " " << so;
  }
  f << endl;
  for (TemplateSubSlice const& td : tds) {
    f << td << endl;
  }
}

void output_sub_slices_mult(
  shared_ptr<Module> module,
  string const& dir,
  vector<vector<vector<TemplateSubSlice>>> const& sub_slices,
  bool by_size)
{
  assert (dir != "");
  string d = (dir[dir.size() - 1] == '/' ? dir : dir + "/");

  if (by_size) {
    for (int i = 0; i < (int)sub_slices.size(); i++) {
      assert (sub_slices[i].size() > 0); // driver code won't read the files right if this isn't true
      for (int j = 0; j < (int)sub_slices[i].size(); j++) {
        write_template_sub_slice_file(module,
            d + to_string(i+1) + "." + to_string(j+1), sub_slices[i][j]);
      }
    }
  } else {
    assert (sub_slices.size() == 1);
    for (int i = 0; i < (int)sub_slices[0].size(); i++) {
      write_template_sub_slice_file(module, d + to_string(i+1), sub_slices[0][i]);
    }
  }
}

value parse_templ(string const& s) {
  vector<char> new_s;
  for (int i = 0; i < (int)s.size(); i++) {
    if (s[i] == '-') new_s.push_back(' ');
    else new_s.push_back(s[i]);
  }

  string t(new_s.begin(), new_s.end());

  std::stringstream ss;
  ss << t;

  vector<pair<bool, vector<VarDecl>>> all_decls;
  bool is_forall = false;
  bool is_exists = false;
  vector<VarDecl> decls;
  int varname = 0;
  while (true) {
    string s;
    ss >> s;

    if (s == "") {
      break;
    }

    if (!is_forall && !is_exists) {
      assert (s == "forall" || s == "exists");
      if (s == "forall") {
        is_forall = true;
      } else {
        is_exists = true;
      }
    } else if (s == ".") {
      assert (decls.size() > 0);
      assert (is_forall || is_exists);
      all_decls.push_back(make_pair(is_exists, decls));
      is_forall = false;
      is_exists = false;
      decls.clear();
    } else {
      lsort so = s_uninterp(s);

      string name = to_string(varname);
      varname++;
      assert (name.size() <= 3);
      while (name.size() < 3) {
        name = "0" + name;
      }
      name = "A" + name;

      decls.push_back(VarDecl(string_to_iden(name), so));
    }
  }

  if (decls.size() > 0) {
    assert (is_forall || is_exists);
    all_decls.push_back(make_pair(is_exists, decls));
  }

  value templ = v_template_hole();
  for (int i = (int)all_decls.size() - 1; i >= 0; i--) {
    if (all_decls[i].first) {
      templ = v_exists(all_decls[i].second, templ);
    } else {
      templ = v_forall(all_decls[i].second, templ);
    }
  }

  cout << "parsed template " << templ->to_string() << endl;
  return templ;
}

TemplateSpace template_space_from_enum_options(
    shared_ptr<Module> module,
    EnumOptions const& options)
{
  TemplateSpace ts;

  value templ;
  if (options.template_str != "") {
    templ = parse_templ(options.template_str);
  } else {
    int idx = options.template_idx == -1 ? 0 : options.template_idx;
    assert (0 <= idx && idx < (int)module->templates.size());
    templ = module->templates[idx];
  }
  TopAlternatingQuantifierDesc taqd(templ);

  for (int i = 0; i < (int)module->sorts.size(); i++) {
    ts.vars.push_back(0);
    ts.quantifiers.push_back(Quantifier::Forall);
  }

  for (Alternation const& alter : taqd.alternations()) {
    for (VarDecl const& decl : alter.decls) {
      int idx = -1;
      for (int i = 0; i < (int)module->sorts.size(); i++) {
        if (sorts_eq(s_uninterp(module->sorts[i]), decl.sort)) {
          idx = i;
          break;
        }
      }
      assert (idx != -1);
      Quantifier q = (alter.altType == AltType::Forall ? Quantifier::Forall : Quantifier::Exists);
      if (ts.vars[idx] > 0) {
        assert (ts.quantifiers[idx] == q);
      }

      ts.quantifiers[idx] = q;
      ts.vars[idx]++;
    }
  }

  ts.depth = (options.depth2_shape ? 2 : 1);
  ts.k = options.disj_arity;

  return ts;
}

long long pre_symm_count_templ(shared_ptr<Module> module, int kmax, int d, int e, vector<int> const& t)
{
  assert (d == 1 || d == 2);

  TemplateSpace ts;
  ts.vars = t;
  for (int i = 0; i < (int)t.size(); i++) {
    ts.quantifiers.push_back(Quantifier::Forall);
  }
  ts.depth = d;
  ts.k = kmax;
  value templ = ts.make_templ(module);

  EnumInfo ei(module, templ);
  int numClauses = ei.clauses.size();

  long long total = 0;
  for (int k = 1; k <= kmax; k++) {
    long long prod = 1;
    for (int i = 0; i < k; i++) {
      prod *= (long long)numClauses;
    }
    if (d == 2) {
      prod *= 2 * (long long)((1 << (k - 1)) - 1);
    }

    total += prod;
  }

  cout << "total " << total << endl;

  int nsorts = t.size();
  long long numCombos = 0;
  for (int j = 0; j < (1 << nsorts); j++) {
    int num_e = 0;
    bool okay = true;
    for (int k = 0; k < nsorts; k++) {
      if ((j >> k) & 1) {
        num_e += t[k];
        if (t[k] == 0) {
          okay = false;
          break;
        }
      }
    }
    if (okay && num_e <= e) {
      numCombos++;
    }
  }

  return total * numCombos;
}

long long pre_symm_count(shared_ptr<Module> module, int k, int d, int m, int e)
{
  int nsorts = module->sorts.size();

  long long total = 0;
  vector<int> t;
  t.resize(nsorts);
  while (true) {
    int sum = 0;
    for (int i = 0; i < (int)t.size(); i++) {
      sum += t[i];
    }
    if (sum == m) {
      total += pre_symm_count_templ(module, k, d, e, t);
    }
    int i;
    for (i = 0; i < (int)t.size(); i++) {
      t[i]++;
      if (t[i] == m+1) {
        t[i] = 0;
      } else {
        break;
      }
    }
    if (i == (int)t.size()) {
      break;
    }
  }
  return total;
}

void do_counts(shared_ptr<Module> module, vector<TemplateSlice> const& slices, int k, int d, int m, int e)
{
  long long pre_count = pre_symm_count(module, k, d, m, e);
  cout << "Pre-symmetries: " << pre_count << endl;

  long long total = 0;
  for (TemplateSlice const& ts : slices) {
    total += ts.count;
  }
  cout << "Post-symmetries: " << total << endl;
  cout << "Num template slices: " << slices.size() << endl;

  /*int i = 0;
  for (TemplateSlice const& ts : slices) {
    if (ts.vars[0] == 2
      && ts.vars[1] == 2
      && ts.vars[2] == 1
      && ts.vars[3] == 1)
    {
      cout << i+4 << endl;
      cout << ts.count << endl;
      return;
    }

    i++;
    for (int j = 0; j < 4; j++) {
      if (ts.vars[j] == 1) {
        i++;
      }
    }
  }*/
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
  options.non_accumulative = false;
  //options.threads = 1;

  string output_chunk_dir;
  string input_chunk_file;
  int nthreads = -1;

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

  bool counts_only = false;

  bool template_sorter = false;
  int template_sorter_k;
  int template_sorter_d;
  int template_sorter_mvars;
  int template_sorter_e;
  string template_outfile;

  bool one_breadth = false;
  bool one_finisher = false;

  bool by_size = false;

  int big_impl_check = -1;

  string stats_filename;

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
    else if (argv[i] == string("--one-finisher")) {
      one_finisher = true;
    }
    else if (argv[i] == string("--one-breadth")) {
      one_breadth = true;
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
    else if (argv[i] == string("--non-accumulative")) {
      options.non_accumulative = true;
    }
    else if (argv[i] == string("--by-size")) {
      by_size = true;
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
    else if (argv[i] == string("--output-chunk-dir")) {
      assert(i + 1 < argc);
      assert (output_chunk_dir == "");
      output_chunk_dir = argv[i+1];
      i++;
    }
    else if (argv[i] == string("--nthreads")) {
      assert(i + 1 < argc);
      assert (nthreads == -1);
      nthreads = atoi(argv[i+1]);
      i++;
    }
    else if (argv[i] == string("--input-module")) {
      assert(i + 1 < argc);
      assert(module_filename == "");
      module_filename = argv[i+1];
      i++;
    }
    else if (argv[i] == string("--stats-file")) {
      assert(i + 1 < argc);
      assert(stats_filename == "");
      stats_filename = argv[i+1];
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
      assert(false && "TODO implement");
      i++;
    }
    else if (argv[i] == string("--template-counter")) {
      assert(i + 2 < argc);
      template_counter = true;
      template_counter_k = atoi(argv[i+1]);
      template_counter_d = atoi(argv[i+2]);
      i += 2;
    }
    else if (argv[i] == string("--template-sorter")) {
      assert(i + 4 < argc);
      template_sorter = true;
      template_sorter_k = atoi(argv[i+1]);
      template_sorter_d = atoi(argv[i+2]);
      template_sorter_mvars = atoi(argv[i+3]);
      template_sorter_e = atoi(argv[i+4]);
      i += 4;
    }
    else if (argv[i] == string("--counts-only")) {
      counts_only = true;
    }
    else if (argv[i] == string("--big-impl-check")) {
      assert (i + 1 < argc);
      big_impl_check = atoi(argv[i+1]);
      i += 1;
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

  if (big_impl_check != -1) {
    vector<value> a;
    vector<value> b;
    for (int i = 0; i < (int)module->conjectures.size() - big_impl_check; i++) {
      a.push_back(module->conjectures[i]);
    }
    for (int i = (int)module->conjectures.size() - big_impl_check; i < (int)module->conjectures.size(); i++) {
      b.push_back(module->conjectures[i]);
    }
    //assert (is_itself_invariant(module, a));
    //cout << "is inv" << endl;

    int total = 0;
    for (int i = 0; i < (int)a.size(); i++) {
      cout << i << endl;
      vector<value> vs;
      for (value v : b) {
        vs.push_back(v);
      }
      vs.push_back(v_not(a[i]));

      if (!is_satisfiable(module, v_and(vs))) {
        total++;
      }
    }

    cout << "could prove " << total << " / " << a.size() << endl;
    return 0;
  }

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

  if (template_sorter) {
    assert (template_sorter_d == 1 || template_sorter_d == 2);
    auto forall_slices = count_many_templates(module,
        template_sorter_k,
        template_sorter_d == 2,
        template_sorter_mvars);
    auto slices = quantifier_combos(module, forall_slices, template_sorter_e);
    if (counts_only) {
      do_counts(module, slices,
          template_sorter_k,
          template_sorter_d,
          template_sorter_mvars,
          template_sorter_e);
      return 0;
    }
    if (output_chunk_dir != "") {
      assert (nthreads != -1);
      assert (one_breadth ^ one_finisher);
      auto sub_slices = prioritize_sub_slices(module, slices, nthreads, one_breadth, by_size, false);
      output_sub_slices_mult(module, output_chunk_dir, sub_slices, by_size);
    }
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

    EnumOptions enum_options;
    enum_options.template_idx = -1;
    enum_options.disj_arity = -1;
    enum_options.depth2_shape = false;

    for (i++; i < argc; i++) {
      if (argv[i] == string("--finisher")) {
        break;
      }
      else if (argv[i] == string("--breadth")) {
        break;
      }
      else if (argv[i] == string("--template-space")) {
        assert(i + 1 < argc);
        enum_options.template_str = string(argv[i+1]);
        i++;
      }
      else if (argv[i] == string("--template")) {
        assert(i + 1 < argc);
        enum_options.template_idx = atoi(argv[i+1]);
        assert(0 <= enum_options.template_idx
            && enum_options.template_idx < (int)module->templates.size());
        i++;
      }
      else if (argv[i] == string("--disj-arity")) {
        assert(i + 1 < argc);
        enum_options.disj_arity = atoi(argv[i+1]);
        i++;
      }
      else if (argv[i] == string("--depth2-shape")) {
        enum_options.depth2_shape = true;
      }
      else {
        cout << "unreocgnized enum_options argument " << argv[i] << endl;
        return 1;
      }
    }

    strat.tspace = template_space_from_enum_options(module, enum_options);

    strats.push_back(strat);
  }

  printf("random seed = %d\n", seed);
  srand(seed);

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

  if (output_chunk_dir != "" || input_chunk_file != "") {
    for (int i = 1; i < (int)strats.size(); i++) {
      assert (strats[0].breadth == strats[i].breadth);
      assert (strats[0].finisher == strats[i].finisher);
    }
  }

  vector<TemplateSubSlice> sub_slices_breadth;
  vector<TemplateSubSlice> sub_slices_finisher;

  if (input_chunk_file != "") {
    assert (strats.size() == 0);
    assert (output_chunk_dir == "");
    auto ss = read_template_sub_slice_file(module, input_chunk_file);
    assert (one_breadth || one_finisher);
    assert (!(one_breadth && one_finisher));
    if (one_breadth) sub_slices_breadth = ss;
    else if (one_finisher) sub_slices_finisher = ss;
  } else {
    if (strats.size() == 0) {
      cout << "No strategy given ???" << endl;
      return 1;
    }

    if (output_chunk_dir != "") {
      vector<TemplateSlice> slices;
      for (Strategy const& strat : strats) {
        vector_append(slices, break_into_slices(module, strat.tspace));
      }
      assert (nthreads != -1);
      auto sub_slices = prioritize_sub_slices(module, slices, nthreads, strats[0].breadth, by_size, true);
      output_sub_slices_mult(module, output_chunk_dir, sub_slices, by_size);
      return 0;
    } else {
      vector<TemplateSlice> slices_finisher;
      vector<TemplateSlice> slices_breadth;
      for (int i = 0; i < (int)strats.size(); i++) {
        if (strats[i].breadth) {
          vector_append(slices_breadth, break_into_slices(module, strats[i].tspace));
        } else if (strats[i].finisher) {
          vector_append(slices_finisher, break_into_slices(module, strats[i].tspace));
        } else {
          assert (false);
        }
      }
      assert (false); // TODO not implemented because the size of the return vector isn't guaranteed
      //assert (!by_size);
      //sub_slices_breadth = prioritize_sub_slices(module, slices_breadth, 1, true)[0];
      //sub_slices_finisher = prioritize_sub_slices(module, slices_finisher, 1, false)[0];
    }
  }

  FormulaDump output_fd;
  output_fd.success = false;
  output_fd.base_invs = input_fd.base_invs;
  output_fd.conjectures = input_fd.conjectures;

  if (options.get_space_size) {
    long long b_pre_symm = -1;
    long long b_post_symm = -1;
    long long f_pre_symm = -1;
    long long f_post_symm = -1;
    if (sub_slices_breadth.size() > 0) {
      shared_ptr<CandidateSolver> cs =
          make_candidate_solver(module, sub_slices_breadth, true);
      b_pre_symm = cs->getPreSymmCount();
      cout << "b_pre_symm " << b_pre_symm << endl;
      b_post_symm = cs->getSpaceSize();
      cout << "b_post_symm " << b_post_symm << endl;
    }
    if (sub_slices_finisher.size() > 0) {
      shared_ptr<CandidateSolver> cs = make_candidate_solver(
          module, sub_slices_finisher, false);
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
    bool single_round = (one_breadth || one_finisher);

    if (sub_slices_breadth.size()) {
      SynthesisResult synres;
      cout << endl;
      cout << ">>>>>>>>>>>>>> Starting breadth algorithm" << endl;
      cout << endl;
      synres = synth_loop_incremental_breadth(module, sub_slices_breadth, options,
          input_fd, single_round);

      augment_fd(output_fd, synres);

      if (synres.done) {
        cout << "Synthesis success!" << endl;
        goto finish;
      }

      module = module->add_conjectures(synres.new_values);
    }

    if (sub_slices_finisher.size()) {
      cout << endl;
      cout << ">>>>>>>>>>>>>> Starting finisher algorithm" << endl;
      cout << endl;
      SynthesisResult synres = synth_loop(module, sub_slices_finisher, options,
          input_fd);

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

    if (stats_filename != "") {
      global_stats.dump(stats_filename);
    }

    return 0;
  } catch (z3::exception exc) {
    printf("got z3 exception: %s\n", exc.msg());
    throw;
  }
}
