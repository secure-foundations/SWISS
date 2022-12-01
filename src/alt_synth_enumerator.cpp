#include "alt_synth_enumerator.h"

#include <algorithm>

#include "enumerator.h"
#include "var_lex_graph.h"
#include "auto_redundancy_filters.h"

using namespace std;

AltDisjunctCandidateSolver::AltDisjunctCandidateSolver(
      shared_ptr<Module> module,
      TemplateSpace const& tspace)
  : progress(0)
  , module(module)
  , disj_arity(tspace.k)
  , tspace(tspace)
  , taqd(v_template_hole())
  , start_from(-1)
  , done_cutoff(0)
  , finish_at_cutoff(false)
{
  cout << "Using AltDisjunctCandidateSolver" << endl;
  cout << "disj_arity: " << disj_arity << endl;

  assert (tspace.depth == 1);

  value templ = tspace.make_templ(module);
  taqd = TopAlternatingQuantifierDesc(templ);
  EnumInfo ei(module, templ);

  pieces = ei.clauses;

  //cout << "Using " << pieces.size() << " terms" << endl;
  //for (value p : pieces) {
  //  cout << "piece: " << p->to_string() << endl;
  //}
  //assert(false);
  
  init_piece_to_index();

  cur_indices = {};
  done = false;

  var_index_states.resize(disj_arity + 2);

  ts = build_transition_system(
      get_var_index_init_state(module, templ),
      ei.var_index_transitions, -1);

  existing_invariant_trie = SubsequenceTrie(pieces.size());

  for (vector<int> const& indices : get_auto_redundancy_filters(pieces)) {
    // These are things we want to filter out. They aren't necessarily invariant,
    // though! Only call existing_invariants_append, not addExistingInvariant.
    existing_invariants_append(indices);
  }
}

void AltDisjunctCandidateSolver::addCounterexample(Counterexample cex)
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

void AltDisjunctCandidateSolver::existing_invariants_append(std::vector<int> const& indices)
{
  existing_invariant_indices.push_back(indices);
  existing_invariant_trie.insert(indices);
}

void AltDisjunctCandidateSolver::addExistingInvariant(value inv0)
{
  assert(false);
  /*for (value inv : taqd.rename_into_all_possibilities(inv0)) {
    vector<int> indices = get_indices_of_value(inv);
    sort(indices.begin(), indices.end());
    existing_invariants_append(indices);

    value norm = inv->totally_normalize();
    existing_invariant_set.insert(ComparableValue(norm));
  }*/
}

void AltDisjunctCandidateSolver::addRedundantDesc(std::vector<int> const& indices)
{
  //cout << "INDICES: ";
  //for (int i : indices) {
  //  cout << i << " ";
  //}
  //cout << endl;
  existing_invariants_append(indices);
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

void AltDisjunctCandidateSolver::init_piece_to_index() {
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

int AltDisjunctCandidateSolver::get_index_of_piece(value p) {
  auto it = piece_to_index.find(ComparableValue(p));
  assert (it != piece_to_index.end());
  return it->second;
}

vector<int> AltDisjunctCandidateSolver::get_indices_of_value(value inv) {
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

value AltDisjunctCandidateSolver::disjunction_fuse(vector<value> values) {
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

  return taqd.with_body(v_or(values));
}

value AltDisjunctCandidateSolver::getNext() {
  while (true) {
    while (true) {
      //cout << "start increment" << endl;
      increment();
      //cout << "hi" << endl;
      if (done) {
        //cout << "return nullptr" << endl;
        return nullptr;
      }
      //cout << var_index_states[cur_indices.size()] << " "
      //    << target_state << endl;
      if (sub_ts.next(
          var_index_states[cur_indices_sub.size()-1],
          cur_indices_sub[cur_indices_sub.size()-1]) == target_state) {
        break;
      }
      //cout << "bro" << endl;
    }
    progress++;
    //cout << "attempting" << endl;

    for (int i = 0; i < (int)cur_indices.size(); i++) {
      cur_indices[i] = slice_index_map[cur_indices_sub[i]];
    }

    // TODO comment this
    /*value sanity_v;
    {
      vector<value> disjs;
      for (int i = 0; i < (int)cur_indices.size(); i++) {
        disjs.push_back(pieces[cur_indices[i]]);
      }
      sanity_v = disjunction_fuse(disjs);
      cout << "getNext: " << sanity_v->to_string() << endl;
    }*/

    bool failed = false;

    //// Check if it contains an existing invariant

    int upTo;
    if (existing_invariant_trie.query(cur_indices, upTo /* output */)) {
      numEnumeratedFilteredRedundantInvariants++;
      this->skipAhead(upTo);
      failed = true;
    }

    if (failed) continue;

    //// Check if it violates a countereample

    for (int i = 0; i < (int)cexes.size(); i++) {
      if (cexes[i].is_true) {
        abes[i].second.reset_for_disj();
        for (int j = 0; j < (int)cur_indices.size(); j++) {
          abes[i].second.add_disj(cex_results[i][cur_indices[j]].second);
        }
        bool res = abes[i].second.evaluate();
        //assert (res == cexes[i].is_true->eval_predicate(sanity_v));
        if (!res) {
          failed = true;
          break;
        }
      }
      else if (cexes[i].is_false) {
        abes[i].first.reset_for_disj();
        for (int j = 0; j < (int)cur_indices.size(); j++) {
          abes[i].first.add_disj(cex_results[i][cur_indices[j]].first);
        }
        bool res = abes[i].first.evaluate();
        //assert (res == cexes[i].is_false->eval_predicate(sanity_v));
        if (res) {
          failed = true;
          break;
        }
      }
      else {
        abes[i].first.reset_for_disj();
        for (int j = 0; j < (int)cur_indices.size(); j++) {
          abes[i].first.add_disj(cex_results[i][cur_indices[j]].first);
        }
        bool res = abes[i].first.evaluate();
        //assert (res == cexes[i].hypothesis->eval_predicate(sanity_v));
        if (res) {
          abes[i].second.reset_for_disj();
          for (int j = 0; j < (int)cur_indices.size(); j++) {
            abes[i].second.add_disj(cex_results[i][cur_indices[j]].second);
          }
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

    /*if (existing_invariant_set.count(ComparableValue(v->totally_normalize())) > 0) {
      existing_invariants_append(cur_indices);
      continue;
    }*/

    dump_cur_indices();
    return v;
  }
}

void AltDisjunctCandidateSolver::dump_cur_indices()
{
  cout << "cur_indices_sub:";
  for (int i : cur_indices_sub) {
    cout << " " << i;
  }
  cout << " / " << slice_index_map.size() << endl;
}

// Skip all remaining index-sequences that match
// the first `upTo` numbers of the current index-sequence
void AltDisjunctCandidateSolver::skipAhead(int upTo)
{
  for (int i = upTo; i < (int)cur_indices_sub.size(); i++) {
    cur_indices_sub[i] = (int)pieces.size() + i - (int)cur_indices_sub.size();
  }
}

void AltDisjunctCandidateSolver::increment()
{
  int n = slice_index_map.size();

  int t = cur_indices_sub.size();

  if (start_from != -1) {
    t = start_from;
    start_from = -1;
    goto body_start;
  }

  goto body_end;

level_size_top:
  cur_indices_sub.push_back(0);
  if ((int)cur_indices_sub.size() > disj_arity) {
    this->done = true;
    return;
  }
  t = 0;
  goto body_start;

body_start:
  if (t == (int)cur_indices_sub.size()) {
    return;
  }

  if (t > 0) {
    //assert (t-1 >= 0);
    //assert (t <= (int)var_index_states.size());
    //assert (0 <= t);
    //assert (t-1 <= (int)cur_indices_sub.size());
    var_index_states[t] = sub_ts.next(
      var_index_states[t-1],
      cur_indices_sub[t-1]);
  }

  cur_indices_sub[t] = (t == 0 ? 0 : cur_indices_sub[t-1] + 1);

  goto loop_start_before_check;

loop_start:
  //assert (t >= 0);
  //assert (t <= (int)var_index_states.size());
  //assert (0 <= t);
  //assert (t <= (int)cur_indices_sub.size());
  //cout << cur_indices_sub[t] << endl;
  //cout << sub_ts.nTransitions() << endl;
  //cout << slice_index_map.size() << endl;
  if (sub_ts.next(var_index_states[t], cur_indices_sub[t]) != -1) {
    t++;
    goto body_start;
  }

call_end:
  cur_indices_sub[t]++;
loop_start_before_check:
  if (cur_indices_sub[t] >= n) {
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

void AltDisjunctCandidateSolver::setSubSlice(TemplateSubSlice const& tss)
{
  this->tss = tss; 
  auto p = get_subslice_index_map(ts, tss.ts);
  slice_index_map = p.first.first;
  //cout << "slice_index_map" << endl;
  //for (int i : slice_index_map) {
  //  cout << "slice_index_map " << i << endl;
  //}
  sub_ts = p.first.second;
  target_state = p.second;
  assert (target_state != -1);

  //cout << "chunk: " << sc.nums.size() << " / " << sc.size << endl;
  assert (tss.ts.k > 0);
  cur_indices_sub.resize(tss.ts.k);
  cur_indices.resize(tss.ts.k);
  assert ((int)tss.prefix.size() <= tss.ts.k);
  for (int i = 0; i < (int)tss.prefix.size(); i++) {
    cur_indices_sub[i] = tss.prefix[i];
  }
  for (int i = 1; i <= (int)tss.prefix.size(); i++) {
    var_index_states[i] = sub_ts.next(
        var_index_states[i-1],
        cur_indices_sub[i-1]);
  }
  start_from = tss.prefix.size();
  done_cutoff = tss.prefix.size();
  done = false;
  finish_at_cutoff = true;
}

long long AltDisjunctCandidateSolver::getPreSymmCount() {
  long long ans = 1;
  for (int i = 0; i < disj_arity; i++) ans *= (long long)pieces.size();
  return ans;
}
