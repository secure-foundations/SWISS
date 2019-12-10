#include "big_disjunct_synth_enumerator.h"

#include "enumerator.h"

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
  
  init_piece_to_index();

  cur_indices = {};
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

bool is_indices_subset(vector<int> const& a, vector<int> const& b) {
  if (a.size() == 0) return true;
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
      if (i >= a.size()) return true;
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
  while (true) {
    increment();
    if (cur_indices.size() > disj_arity) {
      return nullptr;
    }

    vector<BitsetEvalResult*> bers;
    bers.resize(cur_indices.size());

    bool failed = false;
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

    for (int i = 0; i < existing_invariant_indices.size(); i++) {
      if (is_indices_subset(existing_invariant_indices[i], cur_indices)) {
        failed = true;
        break;
      }
    }

    if (failed) continue;

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

void BigDisjunctCandidateSolver::increment()
{
  int j;
  for (j = cur_indices.size() - 1; j >= 0; j--) {
    if (cur_indices[j] != pieces.size() - cur_indices.size() + j) {
      cur_indices[j]++;
      for (int k = j+1; k < cur_indices.size(); k++) {
        cur_indices[k] = cur_indices[k-1] + 1;
      }
      break;
    }
  }
  if (j == -1) {
    cur_indices.push_back(0);
    assert (cur_indices.size() <= pieces.size());
    for (int i = 0; i < cur_indices.size(); i++) {
      cur_indices[i] = i;
    }
  }
}
