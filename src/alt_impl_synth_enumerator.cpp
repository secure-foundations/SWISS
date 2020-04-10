#include "alt_impl_synth_enumerator.h"

#include "enumerator.h"
#include "var_lex_graph.h"
#include "utils.h"

using namespace std;

AltImplCandidateSolver::AltImplCandidateSolver(shared_ptr<Module> module, value templ, int disj_arity)
  : module(module)
  , arity1(disj_arity)
  , arity2(disj_arity)
  , progress(0)
  , taqd(templ)
  , start_from(-1)
  , done_cutoff(0)
  , finish_at_cutoff(false)
{
  cout << "Using AltImplCandidateSolver" << endl;
  cout << "disj_arity: " << disj_arity << endl;

  total_arity = arity1 + arity2;

  auto values = cached_get_unfiltered_values(module, templ, 1);
  values->init_simp();
  pieces = values->values;

  //cout << "Using " << pieces.size() << " terms" << endl;
  //for (value p : pieces) {
  //  cout << "piece: " << p->to_string() << endl;
  //}
  //assert(false);

  init_piece_to_index();

  //cur_indices = {0, 103, 105, 0, 1, 2};
  //cur_incides = {3, 4, 5, 11, 23, 97};
  //cur_indices = {28, 55, 61, 2, 15, 70};
  //cur_indices = {};
  done = false;

  var_index_states.resize(total_arity + 2);
  var_index_states[0] = get_var_index_init_state(templ);
  for (int i = 1; i < (int)var_index_states.size(); i++) {
    var_index_states[i] = var_index_states[0];
  }

  var_index_transitions =
      get_var_index_transitions(templ, pieces);

  assert (arity2 <= 3);
  existing_invariant_tries.resize(1 +
    pieces.size() +
    pieces.size() * pieces.size() +
    pieces.size() * pieces.size() * pieces.size()
    );

  for (int i = 0; i < (int)existing_invariant_tries.size(); i++) {
    existing_invariant_tries[i] = SubsequenceTrie(pieces.size());
  }
}

void AltImplCandidateSolver::addCounterexample(Counterexample cex, value candidate)
{
  cout << "add counterexample" << endl;
  assert (!cex.none);
  assert (cex.is_true || cex.is_false || (cex.hypothesis && cex.conclusion));

  cexes.push_back(cex);
  int i = cexes.size() - 1;

  AlternationBitsetEvaluator abe1;
  AlternationBitsetEvaluator abe2;

  if (cex.is_true) {
    abe2 = AlternationBitsetEvaluator::make_evaluator(
        cex.is_true, pieces[0]);
  }
  else if (cex.is_false) {
    abe1 = AlternationBitsetEvaluator::make_evaluator(
        cex.is_false, pieces[0]);
  }
  else {
    assert(cex.hypothesis);
    assert(cex.conclusion);
    abe1 = AlternationBitsetEvaluator::make_evaluator(
        cex.hypothesis, pieces[0]);
    abe2 = AlternationBitsetEvaluator::make_evaluator(
        cex.conclusion, pieces[0]);
  }

  abes.push_back(make_pair(move(abe1), move(abe2)));

  cex_results.push_back({});
  cex_results[i].resize(pieces.size());

  for (int j = 0; j < (int)pieces.size(); j++) {
    if (cex.is_true) {
      cex_results[i][j].second = BitsetEvalResult::eval_over_alternating_quantifiers(cex.is_true, pieces[j]);
    }
    else if (cex.is_false) {
      cex_results[i][j].first = BitsetEvalResult::eval_over_alternating_quantifiers(cex.is_false, pieces[j]);
    }
    else {
      cex_results[i][j].first = BitsetEvalResult::eval_over_alternating_quantifiers(cex.hypothesis, pieces[j]);
      cex_results[i][j].second = BitsetEvalResult::eval_over_alternating_quantifiers(cex.conclusion, pieces[j]);
    }
  }
}

void AltImplCandidateSolver::existing_invariants_append(
    std::pair<std::vector<int>, int> const& indices)
{
  //existing_invariant_indices.push_back(indices);
  existing_invariant_tries[indices.second].insert(indices.first);
}

void AltImplCandidateSolver::addExistingInvariant(value inv0)
{
  for (value inv : taqd.rename_into_all_possibilities(inv0)) {
    auto indices = get_indices_of_value(inv);
    existing_invariants_append(indices);

    value norm = inv->totally_normalize();
    existing_invariant_set.insert(ComparableValue(norm));
  }
}

inline bool is_indices_subset(vector<int> const& a, vector<int> const& b, int& upTo) {
  if (a.size() == 0) { upTo = 0; return true; }
  if (b.size() == 0) return false;
  int i = 0;
  int j = 0;
  while (true) {
    if (a[i] < b[j]) {
      return false;
    }
    else if (a[i] == b[j]) {
      i++;
      j++;
      if (i >= (int)a.size()) { upTo = j; return true; }
      if (j >= (int)b.size()) return false;
    }
    else {
      j++;
      if (j >= (int)b.size()) return false;
    }
  }
}

void AltImplCandidateSolver::init_piece_to_index() {
  for (int i = 0; i < (int)pieces.size(); i++) {
    value v = pieces[i];
    while (true) {
      if (Forall* f = dynamic_cast<Forall*>(v.get())) {
        v = f->body;
      }
      else if (Exists* f = dynamic_cast<Exists*>(v.get())) {
        v = f->body;
      }
      else {
        break;
      }
    }
    piece_to_index.insert(make_pair(ComparableValue(v), i));
  }
}

int AltImplCandidateSolver::get_index_of_piece(value p) {
  auto it = piece_to_index.find(ComparableValue(p));
  assert (it != piece_to_index.end());
  return it->second;
}

int AltImplCandidateSolver::get_index_for_and(std::vector<value> const& p)
{
  assert ((int)p.size() <= arity2);
  int c = 0;
  for (int i = 0; i < (int)p.size(); i++) {
    c = c * pieces.size() + get_index_of_piece(p[i]);
  }

  c++;
  int q = 1;
  for (int i = 1; i < (int)p.size(); i++) {
    q *= pieces.size();
    c += q;
  }

  assert (c < (int)existing_invariant_tries.size());
  return c;
}

pair<vector<int>, int> AltImplCandidateSolver::get_indices_of_value(value inv)
{
  while (true) {
    if (Forall* f = dynamic_cast<Forall*>(inv.get())) {
      inv = f->body;
    }
    else if (Exists* f = dynamic_cast<Exists*>(inv.get())) {
      inv = f->body;
    }
    else {
      break;
    }
  }
  Or* o = dynamic_cast<Or*>(inv.get());
  vector<value> args;
  if (o != NULL) {
    args = o->args;
  } else {
    args.push_back(inv);
  }

  int the_and = 0;
  vector<int> t;
  for (int i = 0; i < (int)args.size(); i++) {
    if (And* a = dynamic_cast<And*>(args[i].get())) {
      assert (the_and == 0);
      the_and = get_index_for_and(a->args);
    } else {
      t.push_back(get_index_of_piece(args[i]));
    }
  }

  return make_pair(sort_and_uniquify(t), the_and);
}

value AltImplCandidateSolver::disjunction_fuse(vector<value> values) {
  for (int i = 0; i < (int)values.size(); i++) {
    while (true) {
      if (Forall* f = dynamic_cast<Forall*>(values[i].get())) {
        values[i] = f->body;
      }
      else if (Exists* f = dynamic_cast<Exists*>(values[i].get())) {
        values[i] = f->body;
      }
      else {
        break;
      }
    }
  }

  vector<value> conj;
  for (int i = arity1; i < arity1 + arity2; i++) {
    conj.push_back(values[i]);
  }

  vector<value> disj;
  for (int i = 0; i < arity1; i++) {
    disj.push_back(values[i]);
  }
  disj.push_back(v_and(conj));

  return taqd.with_body(v_or(disj));
}

int t = 0;

value AltImplCandidateSolver::getNext() {
  while (true) {
    //increment();
    assert(cur_indices.size() == 0);
    cur_indices = {11, 19, 34, 15, 56, 63};
    progress++;
    if (done) {
      return nullptr;
    }

    t++;
    if (t == 50000) {
      cout << "incrementing... ";
      dump_cur_indices();
      t = 0;
    }

    // TODO comment this
    /*value sanity_v;
    {
      vector<value> disjs;
      for (int i = 0; i < (int)cur_indices.size(); i++) {
        disjs.push_back(pieces[cur_indices[i]]);
      }
      sanity_v = disjunction_fuse(disjs);
      //cout << "getNext: " << sanity_v->to_string() << endl;
    }*/

    bool failed = false;

    //// Check if it contains an existing invariant

    int upTo;

    vector<int> simple_indices = get_simple_indices(cur_indices);
    if (existing_invariant_tries[0].query(simple_indices, upTo /* output */)) {
      this->skipAhead(upTo);
      continue;
    }

    int ci = get_summary_index(cur_indices);
    if (existing_invariant_tries[ci].query(simple_indices, upTo /* output */)) {
      //this->skipAhead(upTo);
      continue;
    }

    //// Check if it violates a countereample

    for (int i = 0; i < (int)cexes.size(); i++) {
      if (cexes[i].is_true) {
        setup_abe2(abes[i].second, cex_results[i], cur_indices);
        bool res = abes[i].second.evaluate();
        /*if (res != cexes[i].is_true->eval_predicate(sanity_v)) {
          cexes[i].is_true->dump();
          cout << sanity_v->to_string() << endl;
          cout << "result shoudl be " << cexes[i].is_true->eval_predicate(sanity_v) << endl;
          for (int k = 0; k < arity1 + arity2; k++) {
            cex_results[i][cur_indices[k]].second.dump();
          }

          assert(false);
        }*/
        if (!res) {
          failed = true;
          break;
        }
      }
      else if (cexes[i].is_false) {
        setup_abe1(abes[i].first, cex_results[i], cur_indices);
        bool res = abes[i].first.evaluate();
        //assert (res == cexes[i].is_false->eval_predicate(sanity_v));
        if (res) {
          failed = true;
          break;
        }
      }
      else {
        setup_abe1(abes[i].first, cex_results[i], cur_indices);
        bool res = abes[i].first.evaluate();
        //assert (res == cexes[i].hypothesis->eval_predicate(sanity_v));
        if (res) {
          setup_abe2(abes[i].second, cex_results[i], cur_indices);
          bool res2 = abes[i].second.evaluate();
          //assert (res2 == cexes[i].conclusion->eval_predicate(sanity_v));
          if (!res2) {
            failed = true;
            break;
          }
        }
      }
    }

    if (failed) continue;

    //// Check if it's equivalent to an existing invariant
    //// by some normalization.

    vector<value> disjs;
    for (int i = 0; i < (int)cur_indices.size(); i++) {
      disjs.push_back(pieces[cur_indices[i]]);
    }
    value v = disjunction_fuse(disjs);

    if (existing_invariant_set.count(ComparableValue(v->totally_normalize())) > 0) {
      existing_invariants_append(make_pair(simple_indices, ci));
      continue;
    }

    dump_cur_indices();
    return v;
  }
}

void AltImplCandidateSolver::dump_cur_indices()
{
  cout << "cur_indices:";
  for (int i : cur_indices) {
    cout << " " << i;
  }
  cout << " / " << pieces.size() << endl;
}

// Skip all remaining index-sequences that match
// the first `upTo` numbers of the current index-sequence
void AltImplCandidateSolver::skipAhead(int upTo)
{
  for (int i = upTo; i < (int)cur_indices.size(); i++) {
    cur_indices[i] = pieces.size();
  }
}

void AltImplCandidateSolver::increment()
{
  int n = pieces.size();

  int t = cur_indices.size();

  if (t < total_arity) {
    cur_indices.resize(total_arity);
    t = 0;
    goto body_start;
  }

  if (start_from != -1) {
    t = start_from;
    start_from = -1;
    goto body_start;
  }

  goto body_end;

level_size_top:
  cur_indices.push_back(0);
  if ((int)cur_indices.size() > total_arity) {
    this->done = true;
    return;
  }
  t = 0;
  goto body_start;

body_start:
  if (t == (int)cur_indices.size()) {
    return;
  }

  if (t > 0) {
    var_index_do_transition(
      var_index_states[t-1],
      var_index_transitions[cur_indices[t-1]].res,
      var_index_states[t]);
  }

  cur_indices[t] = (t == 0 || t == arity1 ? 0 : cur_indices[t-1] + 1);

  goto loop_start_before_check;

loop_start:
  if (var_index_is_valid_transition(
      var_index_states[t],
      var_index_transitions[cur_indices[t]].pre)) {
    t++;
    goto body_start;
  }

call_end:
  cur_indices[t]++;
loop_start_before_check:
  if (cur_indices[t] >= n) {
    goto body_end;
  }
  goto loop_start;

body_end:
  if (t == done_cutoff) {
    if (finish_at_cutoff) {
      done = true;
      return;
    }
    goto level_size_top;
  } else {
    t--;
    goto call_end;
  }
}

std::vector<int> AltImplCandidateSolver::get_simple_indices(std::vector<int> const& v) {
  vector<int> res;
  res.resize(arity1);
  std::copy(v.begin(), v.begin() + arity1, res.begin());
  return res;
}

int AltImplCandidateSolver::get_summary_index(std::vector<int> const& v) {
  int c = 0;
  for (int i = arity1; i < arity1 + arity2; i++) {
    c = c * pieces.size() + v[i];
  }
  assert(arity2 == 3);
  int q = pieces.size() +
      pieces.size() * pieces.size();
  return c + 1 + q;
}

void AltImplCandidateSolver::setup_abe1(AlternationBitsetEvaluator& abe, 
    std::vector<std::pair<BitsetEvalResult, BitsetEvalResult>> const& cex_result,
    std::vector<int> const& cur_indices)
{
  abe.reset_for_conj();
  for (int i = arity1; i < arity1 + arity2; i++) {
    abe.add_conj(cex_result[cur_indices[i]].first);
  }
  for (int i = 0; i < arity1; i++) {
    abe.add_disj(cex_result[cur_indices[i]].first);
  }
}

void AltImplCandidateSolver::setup_abe2(AlternationBitsetEvaluator& abe, 
    std::vector<std::pair<BitsetEvalResult, BitsetEvalResult>> const& cex_result,
    std::vector<int> const& cur_indices)
{
  abe.reset_for_conj();
  for (int i = arity1; i < arity1 + arity2; i++) {
    abe.add_conj(cex_result[cur_indices[i]].second);
  }
  for (int i = 0; i < arity1; i++) {
    abe.add_disj(cex_result[cur_indices[i]].second);
  }
}

long long AltImplCandidateSolver::getSpaceSize() {
  while (true) {
    increment();
    progress++;
    if (progress % 500000 == 0) {
      cout << progress << endl;
      dump_cur_indices();
    }
    if (done) {
      return progress;
    }
  }
}

void AltImplCandidateSolver::setSpaceChunk(SpaceChunk const& sc)
{
  //cout << "chunk: " << sc.nums.size() << " / " << sc.size << endl;
  assert (sc.size > 0);
  cur_indices.resize(sc.size);
  assert ((int)sc.nums.size() <= sc.size);
  for (int i = 0; i < (int)sc.nums.size(); i++) {
    cur_indices[i] = sc.nums[i];
  }
  for (int i = 1; i <= (int)sc.nums.size(); i++) {
    var_index_do_transition(
      var_index_states[i-1],
      var_index_transitions[cur_indices[i-1]].res,
      var_index_states[i]);
  }
  start_from = sc.nums.size();
  done_cutoff = sc.nums.size();
  done = false;
  finish_at_cutoff = true;
}

static void getSpaceChunk_rec(vector<SpaceChunk>& res,
  vector<int>& indices, int i, VarIndexState const& vis,
  vector<value> const& pieces,
  vector<VarIndexTransition> const& var_index_transitions, int sz)
{
  if (i == (int)indices.size()) {
    SpaceChunk sc;
    sc.size = sz;
    sc.nums = indices;
    res.push_back(move(sc));
    return;
  }
  int t = (i == 0 ? 0 : indices[i-1] + 1);
  for (int j = t; j < (int)pieces.size(); j++) {
    if (var_index_is_valid_transition(vis, var_index_transitions[j].pre)) {
      VarIndexState next;
      var_index_do_transition(vis, var_index_transitions[j].res, next);
      indices[i] = j;
      getSpaceChunk_rec(res, indices, i+1, next,
          pieces, var_index_transitions, sz);
    }
  }
}

void AltImplCandidateSolver::getSpaceChunk(std::vector<SpaceChunk>& res)
{
  int k = arity2;
  int sz = arity1 + arity2;
  int j = k < sz ? sz - k : 0;
  VarIndexState vis = var_index_states[0];
  vector<int> indices;
  indices.resize(j);
  getSpaceChunk_rec(res, indices, 0, vis,
      pieces, var_index_transitions, sz);
}
