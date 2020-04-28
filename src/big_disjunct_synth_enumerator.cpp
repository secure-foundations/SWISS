#include "big_disjunct_synth_enumerator.h"

#include "enumerator.h"
#include "var_lex_graph.h"
#include <algorithm>

using namespace std;

BigDisjunctCandidateSolver::BigDisjunctCandidateSolver(shared_ptr<Module> module, value templ, int disj_arity)
  : progress(0)
  , module(module)
  , disj_arity(disj_arity)
  , tqd(templ)
  , start_from(-1)
  , done_cutoff(0)
  , finish_at_cutoff(false)
{
  cout << "Using BigDisjunctCandidateSolver" << endl;
  cout << "disj_arity: " << disj_arity << endl;

  auto values = cached_get_unfiltered_values(module, templ, 1);
  values->init_simp();
  pieces = values->values;

  cout << "Using " << pieces.size() << " terms" << endl;
  //for (value p : pieces) {
  //  cout << "piece: " << p->to_string() << endl;
  //}
  //assert(false);
  
  init_piece_to_index();

  cur_indices = {};
  done = false;

  var_index_states.push_back(get_var_index_init_state(templ));
  for (int i = 1; i < disj_arity + 2; i++) {
    var_index_states.push_back(var_index_states[0]);
  }

  var_index_transitions =
      get_var_index_transitions(templ, pieces);

  existing_invariant_trie = SubsequenceTrie(pieces.size());
}

void BigDisjunctCandidateSolver::addCounterexample(Counterexample cex, value candidate)
{
  cout << "add counterexample" << endl;
  assert (!cex.none);
  assert (cex.is_true || cex.is_false || (cex.hypothesis && cex.conclusion));

  cexes.push_back(cex);
  int i = cexes.size() - 1;

  cex_results.push_back({});
  cex_results[i].resize(pieces.size());

  for (int j = 0; j < (int)pieces.size(); j++) {
    if (cex.is_true) {
      cex_results[i][j].second = BitsetEvalResult::eval_over_foralls(cex.is_true, pieces[j]);
    }
    else if (cex.is_false) {
      cex_results[i][j].first = BitsetEvalResult::eval_over_foralls(cex.is_false, pieces[j]);
    }
    else {
      cex_results[i][j].first = BitsetEvalResult::eval_over_foralls(cex.hypothesis, pieces[j]);
      cex_results[i][j].second = BitsetEvalResult::eval_over_foralls(cex.conclusion, pieces[j]);
    }
  }
}

void BigDisjunctCandidateSolver::existing_invariants_append(std::vector<int> const& indices)
{
  existing_invariant_indices.push_back(indices);
  existing_invariant_trie.insert(indices);
}

void BigDisjunctCandidateSolver::addExistingInvariant(value inv0)
{
  for (value inv : tqd.rename_into_all_possibilities(inv0)) {
    vector<int> indices = get_indices_of_value(inv);
    std::sort(indices.begin(), indices.end());
    existing_invariants_append(indices);

    value norm = inv->totally_normalize();
    existing_invariant_set.insert(ComparableValue(norm));
  }
}

bool is_indices_subset(vector<int> const& a, vector<int> const& b, int& upTo) {
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

void BigDisjunctCandidateSolver::init_piece_to_index() {
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

int BigDisjunctCandidateSolver::get_index_of_piece(value p) {
  auto it = piece_to_index.find(ComparableValue(p));
  assert (it != piece_to_index.end());
  return it->second;
}

vector<int> BigDisjunctCandidateSolver::get_indices_of_value(value inv) {
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
  if (o != NULL) {
    vector<int> t;
    t.resize(o->args.size());
    for (int i = 0; i < (int)t.size(); i++) {
      t[i] = get_index_of_piece(o->args[i]);
    }
    return t;
  } else {
    vector<int> t;
    t.resize(1);
    t[0] = get_index_of_piece(inv);
    return t;
  }
}

value BigDisjunctCandidateSolver::disjunction_fuse(vector<value> values) {
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

  return tqd.with_body(v_or(values));
}

value BigDisjunctCandidateSolver::getNext() {
  vector<BitsetEvalResult*> bers;

  while (true) {
    increment();
    progress++;
    if (done) {
      return nullptr;
    }

    /*{
    vector<value> disjs;
    for (int i = 0; i < cur_indices.size(); i++) {
      disjs.push_back(pieces[cur_indices[i]]);
    }
    value v = disjunction_fuse(disjs);
    cout << "getNext: " << v->to_string() << endl;
    }*/
    /*value sanity_v;
    {
      vector<value> disjs;
      for (int i = 0; i < (int)cur_indices.size(); i++) {
        disjs.push_back(pieces[cur_indices[i]]);
      }
      sanity_v = disjunction_fuse(disjs);
    }*/

    bool failed = false;

    //// Check if it contains an existing invariant

    int upTo;
    if (existing_invariant_trie.query(cur_indices, upTo /* output */)) {
      this->skipAhead(upTo);
      failed = true;
    }

    if (failed) continue;

    //// Check if it violates a countereample

    if (bers.size() != cur_indices.size()) {
      bers.resize(cur_indices.size());
    }

    for (int i = 0; i < (int)cexes.size(); i++) {
      if (cexes[i].is_true) {
        for (int j = 0; j < (int)cur_indices.size(); j++) {
          bers[j] = &cex_results[i][cur_indices[j]].second;
        }
        bool res = BitsetEvalResult::disj_is_true(bers);
        //assert (res == cexes[i].is_true->eval_predicate(sanity_v));
        if (!res) {
          failed = true;
          break;
        }
      }
      else if (cexes[i].is_false) {
        for (int j = 0; j < (int)cur_indices.size(); j++) {
          bers[j] = &cex_results[i][cur_indices[j]].first;
        }
        bool res = BitsetEvalResult::disj_is_true(bers);
        //assert (res == cexes[i].is_false->eval_predicate(sanity_v));
        if (res) {
          failed = true;
          break;
        }
      }
      else {
        for (int j = 0; j < (int)cur_indices.size(); j++) {
          bers[j] = &cex_results[i][cur_indices[j]].first;
        }
        bool res = BitsetEvalResult::disj_is_true(bers);
        //assert (res == cexes[i].hypothesis->eval_predicate(sanity_v));
        if (res) {
          for (int j = 0; j < (int)cur_indices.size(); j++) {
            bers[j] = &cex_results[i][cur_indices[j]].second;
          }
          bool res2 = BitsetEvalResult::disj_is_true(bers);
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
      existing_invariants_append(cur_indices);
      continue;
    }

    dump_cur_indices();
    return v;
  }
}

void BigDisjunctCandidateSolver::dump_cur_indices()
{
  cout << "cur_indices:";
  for (int i : cur_indices) {
    cout << " " << i;
  }
  cout << " / " << pieces.size() << endl;
}

// Skip all remaining index-sequences that match
// the first `upTo` numbers of the current index-sequence
void BigDisjunctCandidateSolver::skipAhead(int upTo)
{
  for (int i = upTo; i < (int)cur_indices.size(); i++) {
    cur_indices[i] = (int)pieces.size() + i - (int)cur_indices.size();
  }
}

void BigDisjunctCandidateSolver::increment()
{
  int n = pieces.size();
  int t = cur_indices.size();

  if (start_from != -1) {
    t = start_from;
    start_from = -1;
    goto body_start;
  }

  goto body_end;

level_size_top:
  cur_indices.push_back(0);
  if ((int)cur_indices.size() > disj_arity) {
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

  cur_indices[t] = (t == 0 ? 0 : cur_indices[t-1] + 1);

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

long long BigDisjunctCandidateSolver::getSpaceSize() {
  assert(false);
}

void BigDisjunctCandidateSolver::setSpaceChunk(SpaceChunk const& sc)
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

    for (int i = 0; i < (int)indices.size(); i++) {
      cout << indices[i] << " ";
    }
    for (int i = 0; i < (int)indices.size(); i++) {
      cout << pieces[indices[i]]->to_string() << " ";
    }
    cout << endl;
    return;
  }
  int t = (i == 0 ? 0 : indices[i-1] + 1);
  for (int j = t; j < (int)pieces.size(); j++) {
    if (var_index_is_valid_transition(vis, var_index_transitions[j].pre)) {
      VarIndexState next(vis.indices.size());
      var_index_do_transition(vis, var_index_transitions[j].res, next);
      indices[i] = j;

      /*if (i == 1 && indices[0] == 1 && indices[1] == 44) {
        cout << "yooo" << endl;
        for (int x : vis.indices) cout << x << " "; cout << endl;
        for (int x : var_index_transitions[j].pre.indices) cout << x << " "; cout << endl;
      }*/

      getSpaceChunk_rec(res, indices, i+1, next,
          pieces, var_index_transitions, sz);
    }
  }
}

void BigDisjunctCandidateSolver::getSpaceChunk(std::vector<SpaceChunk>& res)
{
  int k = 2;
  for (int sz = 1; sz <= disj_arity; sz++) {
    int j = k < sz ? sz - k : 0;
    VarIndexState vis = var_index_states[0];
    vector<int> indices;
    indices.resize(j);
    getSpaceChunk_rec(res, indices, 0, vis,
        pieces, var_index_transitions, sz);
  }
}
