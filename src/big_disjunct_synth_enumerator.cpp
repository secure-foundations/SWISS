#include "big_disjunct_synth_enumerator.h"

#include "enumerator.h"
#include "var_lex_graph.h"

using namespace std;

BigDisjunctCandidateSolver::BigDisjunctCandidateSolver(shared_ptr<Module> module, int disj_arity)
  : module(module)
  , disj_arity(disj_arity)
  , tqd(module->templates[0])
{
  cout << "Using BigDisjunctCandidateSolver" << endl;
  cout << "disj_arity: " << disj_arity << endl;

  assert(module->templates.size() == 1);

  auto values = cached_get_unfiltered_values(module, 1);
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

  var_index_states.resize(disj_arity + 2);
  var_index_states[0] = get_var_index_init_state(module->templates[0]);
  for (int i = 1; i < var_index_states.size(); i++) {
    var_index_states[i] = var_index_states[0];
  }

  var_index_transitions =
      get_var_index_transitions(module->templates[0], pieces);
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

  for (int j = 0; j < pieces.size(); j++) {
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

void BigDisjunctCandidateSolver::addExistingInvariant(value inv)
{
  vector<int> indices = get_indices_of_value(inv);
  existing_invariant_indices.push_back(indices);

  value norm = inv->totally_normalize();
  existing_invariant_set.insert(ComparableValue(norm));
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
      if (i >= a.size()) { upTo = j; return true; }
      if (j >= b.size()) return false;
    }
    else {
      j++;
      if (j >= b.size()) return false;
    }
  }
}

void BigDisjunctCandidateSolver::init_piece_to_index() {
  for (int i = 0; i < pieces.size(); i++) {
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
    for (int i = 0; i < t.size(); i++) {
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
  for (int i = 0; i < values.size(); i++) {
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

    bool failed = false;

    //// Check if it contains an existing invariant

    for (int i = 0; i < existing_invariant_indices.size(); i++) {
      int upTo;
      if (is_indices_subset(existing_invariant_indices[i], cur_indices, upTo /* output */)) {
        skipAhead(upTo);
        failed = true;
        break;
      }
    }

    if (failed) continue;

    //// Check if it violates a countereample

    if (bers.size() != cur_indices.size()) {
      bers.resize(cur_indices.size());
    }

    for (int i = 0; i < cexes.size(); i++) {
      if (cexes[i].is_true) {
        for (int j = 0; j < cur_indices.size(); j++) {
          bers[j] = &cex_results[i][cur_indices[j]].second;
        }
        bool res = BitsetEvalResult::disj_is_true(bers);
        if (!res) {
          failed = true;
          break;
        }
      }
      else if (cexes[i].is_false) {
        for (int j = 0; j < cur_indices.size(); j++) {
          bers[j] = &cex_results[i][cur_indices[j]].first;
        }
        bool res = BitsetEvalResult::disj_is_true(bers);
        if (res) {
          failed = true;
          break;
        }
      }
      else {
        for (int j = 0; j < cur_indices.size(); j++) {
          bers[j] = &cex_results[i][cur_indices[j]].first;
        }
        bool res = BitsetEvalResult::disj_is_true(bers);
        if (res) {
          for (int j = 0; j < cur_indices.size(); j++) {
            bers[j] = &cex_results[i][cur_indices[j]].second;
          }
          bool res2 = BitsetEvalResult::disj_is_true(bers);
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
    for (int i = 0; i < cur_indices.size(); i++) {
      disjs.push_back(pieces[cur_indices[i]]);
    }
    value v = disjunction_fuse(disjs);

    if (existing_invariant_set.count(ComparableValue(v->totally_normalize())) > 0) {
      existing_invariant_indices.push_back(cur_indices);
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
  for (int i = upTo; i < cur_indices.size(); i++) {
    cur_indices[i] = (int)pieces.size() + i - (int)cur_indices.size();
  }
}

void BigDisjunctCandidateSolver::increment()
{
  int n = pieces.size();

  int t = cur_indices.size();
  goto body_end;

level_size_top:
  cur_indices.push_back(0);
  if (cur_indices.size() > disj_arity) {
    this->done = true;
    return;
  }
  t = 0;
  goto body_start;

body_start:
  if (t == cur_indices.size()) {
    return;
  }

  if (t > 0) {
    var_index_do_transition(
      var_index_states[t-1],
      var_index_transitions[cur_indices[t-1]].res,
      var_index_states[t]);
  }

  cur_indices[t] = (t == 0 ? 0 : cur_indices[t-1] + 1);

loop_start:
  if (var_index_is_valid_transition(
      var_index_states[t],
      var_index_transitions[cur_indices[t]].pre)) {
    t++;
    goto body_start;
  }

call_end:
  cur_indices[t]++;
  if (cur_indices[t] >= n + t - (int)cur_indices.size() + 1) {
    goto body_end;
  }
  goto loop_start;

body_end:
  if (t == 0) {
    goto level_size_top;
  } else {
    t--;
    goto call_end;
  }
}
