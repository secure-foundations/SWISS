#include "alt_depth2_synth_enumerator.h"

#include "enumerator.h"
#include "var_lex_graph.h"
#include "utils.h"

using namespace std;

AltDepth2CandidateSolver::AltDepth2CandidateSolver(
      shared_ptr<Module> module,
      TemplateSpace const& tspace)
  : module(module)
  , total_arity(tspace.k)
  , progress(0)
  , tspace(tspace)
  , taqd(v_template_hole())
  , start_from(-1)
  , done_cutoff(0)
  , finish_at_cutoff(false)
{
  cout << "Using AltDepth2CandidateSolver" << endl;
  cout << "total_arity: " << total_arity << endl;

  assert (tspace.depth == 2);

  value templ = tspace.make_templ(module);
  taqd = TopAlternatingQuantifierDesc(templ);
  EnumInfo ei(module, templ);

  pieces = ei.clauses;

  tree_shapes = get_tree_shapes_up_to(total_arity);

  //for (TreeShape const& ts : tree_shapes) {
  //  cout << ts.to_string() << endl;
  //}

  //cout << "Using " << pieces.size() << " terms" << endl;
  //for (value p : pieces) {
  //  cout << "piece: " << p->to_string() << endl;
  //}
  //assert(false);

  tree_shape_idx = -1;
  cur_indices = {};
  done = false;

  var_index_states.resize(total_arity + 2);
  ts = build_transition_system(
      get_var_index_init_state(module, templ),
      ei.var_index_transitions);
}

void AltDepth2CandidateSolver::addCounterexample(Counterexample cex, value candidate)
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

void AltDepth2CandidateSolver::addExistingInvariant(value inv0)
{
  assert(false);
  /*for (value inv : taqd.rename_into_all_possibilities(inv0)) {
    auto indices = get_indices_of_value(inv);
    existing_invariants_append(indices);

    value norm = inv->totally_normalize();
    existing_invariant_set.insert(ComparableValue(norm));
  }*/
}

value AltDepth2CandidateSolver::get_clause(int i)
{
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
  return v;
}

value AltDepth2CandidateSolver::get_current_value()
{
  vector<value> top_level;
  TreeShape const& ts = tree_shapes[tree_shape_idx];
  int k = 0;
  for (int i = 0; i < (int)ts.parts.size(); i++) {
    vector<value> mid_level;
    for (int j = 0; j < ts.parts[i]; j++) {
      mid_level.push_back(get_clause(slice_index_map[cur_indices[k]]));
      k++;
    }
    top_level.push_back(
        ts.top_level_is_conj ?
          v_or(mid_level) : v_and(mid_level));
  }
  value body = 
      ts.top_level_is_conj ?
        v_and(top_level) : v_or(top_level);

  return taqd.with_body(body);
}

//int t = 0;

value AltDepth2CandidateSolver::getNext() {
  while (true) {
    while (true) {
      increment();
      if (done) {
        return nullptr;
      }
      if (var_index_states[cur_indices.size()] == target_state) {
        break;
      }
    }
    progress++;

    for (int i = 0; i < (int)cur_indices.size(); i++) {
      cur_indices[i] = slice_index_map[cur_indices_sub[i]];
    }

    /*t++;
    if (t == 50000) {
      cout << "incrementing... ";
      dump_cur_indices();
      t = 0;
    }*/

    // TODO comment this
    //dump_cur_indices();
    //value sanity_v = get_current_value();
    //cout << "genning >>>>>>>>>>>>>>>>>>>>>>>>> " << sanity_v->to_string() << endl;

    bool failed = false;

    //// Check if it violates a countereample

    for (int i = 0; i < (int)cexes.size(); i++) {
      if (cexes[i].is_true) {
        setup_abe2(abes[i].second, cex_results[i], cur_indices);
        bool res = abes[i].second.evaluate();
        //if (res != cexes[i].is_true->eval_predicate(sanity_v)) {
          /*cexes[i].is_true->dump();
          cout << sanity_v->to_string() << endl;
          cout << "result shoudl be " << cexes[i].is_true->eval_predicate(sanity_v) << endl;
          for (int k = 0; k < arity1 + arity2; k++) {
            cex_results[i][cur_indices[k]].second.dump();
          }*/

         // assert(false);
        //}
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

    value v = get_current_value();

    //// Check if it's equivalent to an existing invariant
    //// by some normalization.
    /*if (existing_invariant_set.count(ComparableValue(v->totally_normalize())) > 0) {
      existing_invariants_append(make_pair(simple_indices, ci));
      continue;
    }*/

    dump_cur_indices();
    return v;
  }
}

void AltDepth2CandidateSolver::dump_cur_indices()
{
  cout << "cur_indices_sub:";
  for (int i : cur_indices_sub) {
    cout << " " << i;
  }
  cout << " / " << slice_index_map.size() << " in tree shape (idx = "
      << tree_shape_idx << ") "
      << tree_shapes[tree_shape_idx].to_string() << endl;
}

void AltDepth2CandidateSolver::increment()
{
  int n = pieces.size();
  int t = cur_indices_sub.size();

  if (tree_shape_idx == -1) {
    goto level_size_top;
  }

  if (start_from != -1) {
    t = start_from;
    start_from = -1;
    goto body_start;
  }

  goto body_end;

level_size_top:
  tree_shape_idx++;
  if (tree_shape_idx >= (int)tree_shapes.size()) {
    this->done = true;
    return;
  }
  cout << "moving to tree " << tree_shapes[tree_shape_idx].to_string() << endl;
  cout << "progress " << progress << endl;
  cur_indices_sub.resize(tree_shapes[tree_shape_idx].total);
  t = 0;

  goto body_start;

body_start:
  if (t == (int)cur_indices_sub.size()) {
    if (is_normalized_for_tree_shape(tree_shapes[tree_shape_idx], cur_indices_sub)) {
      return;
    } else {
      goto body_end;
    }
  }

  if (t > 0) {
    var_index_states[t] = sub_ts.next(
      var_index_states[t-1],
      slice_index_map[cur_indices_sub[t-1]]);
  }

  {
    SymmEdge const& symm_edge = tree_shapes[tree_shape_idx].symmetry_back_edges[t];
    cur_indices_sub[t] = symm_edge.idx == -1 ? 0 :
        cur_indices_sub[symm_edge.idx] + symm_edge.inc;
  }

  goto loop_start_before_check;

loop_start:
  if (sub_ts.next(var_index_states[t-1], cur_indices_sub[t]) != -1) {
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

void AltDepth2CandidateSolver::setup_abe1(AlternationBitsetEvaluator& abe, 
    std::vector<std::pair<BitsetEvalResult, BitsetEvalResult>> const& cex_result,
    std::vector<int> const& cur_indices)
{
  TreeShape const& ts = tree_shapes[tree_shape_idx];
  if (ts.top_level_is_conj) {
    abe.reset_for_conj();
  } else {
    abe.reset_for_disj();
  }

  int k = 0;

  if (evaluator_buf.size() < cex_result[cur_indices[k]].first.v.size()) {
    evaluator_buf.resize(cex_result[cur_indices[k]].first.v.size());
  }

  for (int i = 0; i < (int)ts.parts.size(); i++) {
    if (ts.parts[i] == 1) {
      if (ts.top_level_is_conj) {
        abe.add_conj(cex_result[cur_indices[k]].first);
      } else {
        abe.add_disj(cex_result[cur_indices[k]].first);
      }
      k++;
    } else {
      vec_copy_ber(evaluator_buf, cex_result[cur_indices[k]].first);
      k++;
      for (int j = 1; j < (int)ts.parts[i]; j++) {
        if (ts.top_level_is_conj) {
          vec_apply_disj(evaluator_buf, cex_result[cur_indices[k]].first);
        } else {
          vec_apply_conj(evaluator_buf, cex_result[cur_indices[k]].first);
        }
        k++;
      }
      if (ts.top_level_is_conj) {
        abe.add_conj(evaluator_buf);
      } else {
        abe.add_disj(evaluator_buf);
      }
    }
  }
}

void AltDepth2CandidateSolver::setup_abe2(AlternationBitsetEvaluator& abe, 
    std::vector<std::pair<BitsetEvalResult, BitsetEvalResult>> const& cex_result,
    std::vector<int> const& cur_indices)
{
  TreeShape const& ts = tree_shapes[tree_shape_idx];
  if (ts.top_level_is_conj) {
    abe.reset_for_conj();
  } else {
    abe.reset_for_disj();
  }

  int k = 0;

  if (evaluator_buf.size() < cex_result[cur_indices[k]].second.v.size()) {
    evaluator_buf.resize(cex_result[cur_indices[k]].second.v.size());
  }

  for (int i = 0; i < (int)ts.parts.size(); i++) {
    if (ts.parts[i] == 1) {
      if (ts.top_level_is_conj) {
        abe.add_conj(cex_result[cur_indices[k]].second);
      } else {
        abe.add_disj(cex_result[cur_indices[k]].second);
      }
      k++;
    } else {
      vec_copy_ber(evaluator_buf, cex_result[cur_indices[k]].second);
      k++;
      for (int j = 1; j < (int)ts.parts[i]; j++) {
        if (ts.top_level_is_conj) {
          vec_apply_disj(evaluator_buf, cex_result[cur_indices[k]].second);
        } else {
          vec_apply_conj(evaluator_buf, cex_result[cur_indices[k]].second);
        }
        k++;
      }
      if (ts.top_level_is_conj) {
        abe.add_conj(evaluator_buf);
      } else {
        abe.add_disj(evaluator_buf);
      }
    }
  }
}

void AltDepth2CandidateSolver::setSubSlice(TemplateSubSlice const& tss)
{
  this->tss = tss; 
  auto p = get_subslice_index_map(ts, tss.ts);
  slice_index_map = p.first.first;
  sub_ts = p.first.second;
  target_state = p.second;

  //cout << "chunk: " << sc.nums.size() << " / " << sc.size << endl;
  assert (0 <= tss.tree_idx && tss.tree_idx < (int)tree_shapes.size());
  tree_shape_idx = tss.tree_idx;
  TreeShape const& ts = tree_shapes[tree_shape_idx];
  assert (ts.total == tss.ts.k);
  cur_indices.resize(ts.total);
  cur_indices_sub.resize(ts.total);
  assert (tss.prefix.size() <= cur_indices.size());
  for (int i = 0; i < (int)tss.prefix.size(); i++) {
    cur_indices[i] = tss.prefix[i];
  }
  for (int i = 1; i <= (int)tss.prefix.size(); i++) {
    var_index_states[i] = this->sub_ts.next(
        var_index_states[i-1],
        slice_index_map[cur_indices_sub[i-1]]);
  }
  start_from = tss.prefix.size();
  done_cutoff = tss.prefix.size();
  done = false;
  finish_at_cutoff = true;
}

long long AltDepth2CandidateSolver::getPreSymmCount() {
  long long ans = 1;
  for (int i = 0; i < total_arity; i++) ans *= (long long)pieces.size();
  long long mul;
  if (total_arity == 1) mul = 1;
  else mul = 2 * (long long)((1 << (total_arity - 1)) - 1);
  return ans * mul;
}
