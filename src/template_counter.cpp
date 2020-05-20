#include "template_counter.h"

#include <iostream>
#include <cassert>

#include "logic.h"
#include "enumerator.h"
#include "var_lex_graph.h"
#include "tree_shapes.h"

using namespace std;

pair<vector<vector<int>>, int> build_transition_matrix(
      VarIndexState const& init,
      std::vector<VarIndexTransition> const& transitions,
      std::vector<int> const& partition)
{
  vector<VarIndexState> idx_to_state;
  idx_to_state.push_back(init);
  vector<vector<int>> matrix;

  int cur = 0;
  while (cur < (int)idx_to_state.size()) {
    matrix.push_back(vector<int>(transitions.size()));

    VarIndexState& cur_state = idx_to_state[cur];
    VarIndexState next(cur_state.indices.size());
    for (int i = 0; i < (int)transitions.size(); i++) {
      if (var_index_is_valid_transition(cur_state, transitions[i].pre)) {
        var_index_do_transition(cur_state, transitions[i].res, next);
        int next_idx = -1;
        for (int j = 0; j < (int)idx_to_state.size(); j++) {
          if (next == idx_to_state[j]) {
            next_idx = j;
            break;
          }
        }
        if (next_idx == -1) {
          idx_to_state.push_back(next);
          next_idx = idx_to_state.size() - 1;
        }
        matrix[cur][i] = next_idx;
      } else {
        matrix[cur][i] = -1;
      }
    }

    cur++;
  }

  VarIndexState last_state(0);
  for (int k : partition) {
    if (k != 0) {
      last_state.indices.push_back(k);
    }
  }
  int last_idx = -1;
  for (int j = 0; j < (int)idx_to_state.size(); j++) {
    //cout << "matrix" << endl;
    //for (int k : idx_to_state[j].indices) cout << k << " ";
    //cout << endl;
    if (last_state == idx_to_state[j]) {
      last_idx = j;
      break;
    }
  }
  return make_pair(matrix, last_idx);
}

long long rec_main(
  vector<vector<int>> const& matrix,
  int final,
  TreeShape const& ts,
  vector<int>& indices,
  int idx,
  int state)
{
  if (idx == (int)indices.size()) {
    if (state == final && is_normalized_for_tree_shape(ts, indices)) {
      /*cout << "counting ";
      for (int i = 0; i < idx; i++) {
        cout << indices[i] << " ";
      }
      cout << endl;*/
      return 1;
    } else {
      return 0;
    }
  }

  //int n = matrix.size();
  int m = matrix[0].size();

  long long res = 0;

  SymmEdge const& symm_edge = ts.symmetry_back_edges[idx];
  int t = symm_edge.idx == -1 ? 0 : indices[symm_edge.idx] + symm_edge.inc;
  for (int j = t; j < m; j++) {
    indices[idx] = j;
    if (matrix[state][j] != -1) {
      res += rec_main(matrix, final, ts, indices, idx+1, matrix[state][j]);
    }
  }
  return res;
}

int name_idx = 0;

value make_template(
    shared_ptr<Module> module,
    vector<int> const& partition)
{
  vector<VarDecl> decls;
  for (int i = 0; i < (int)partition.size(); i++) {
    lsort so = s_uninterp(module->sorts[i]);
    for (int j = 0; j < partition[i]; j++) {
      string name = "A" + to_string(name_idx);
      name_idx++;

      decls.push_back(VarDecl(string_to_iden(name), so));
    }
  }

  value v = v_template_hole();
  for (int i = decls.size() - 1; i >= 0; i--) {
    v = v_forall({decls[i]}, v);
  }
  return v;
}

long long count_space_for_partition(shared_ptr<Module> module, int k, vector<int> const& partition)
{
  assert (partition.size() == module->sorts.size());
  cout << endl;
  cout << "doing variable set:";
  for (int i = 0; i < (int)module->sorts.size(); i++) {
    cout << " " << partition[i] << " " << module->sorts[i];
  }
  cout << endl;

  value templ = make_template(module, partition);
  module->templates.push_back(templ);
  auto values = cached_get_unfiltered_values(module, templ, 1);
  values->init_simp();
  vector<value> pieces = values->values;

  cout << "number of values: " << pieces.size() << endl;
  if (pieces.size() == 0) {
    return 0;
  }
  //for (value v : pieces) {
  //  cout << v->to_string() << endl;
  //}

  auto tree_shapes = get_tree_shapes_up_to(k);

  /*std::vector<VarIndexState> var_index_states;
  var_index_states.push_back(get_var_index_init_state(templ));
  for (int i = 1; i < total_arity + 2; i++) {
    var_index_states.push_back(var_index_states[0]);
  }*/

  std::vector<VarIndexTransition> var_index_transitions =
      get_var_index_transitions(templ, pieces);

  auto p = build_transition_matrix(
      get_var_index_init_state(templ),
      var_index_transitions,
      partition);
  vector<vector<int>> transition_matrix = p.first;
  int start = 0;
  int final = p.second;

  if (final == -1) {
    cout << "skipping" << endl;
    return 0;
  }

  long long result = 0;
  for (int i = 0; i < (int)tree_shapes.size(); i++) {
    if (tree_shapes[i].top_level_is_conj || tree_shapes[i].total == 1) {
      vector<int> indices;
      indices.resize(tree_shapes[i].total);
      long long r = rec_main(
          transition_matrix, final, tree_shapes[i], indices, 0, start);
      long long mul = (tree_shapes[i].total == 1 ? 1 : 2);
      result += r * mul;
    }
  }
  cout << "counted " << result << " for this variable set" << endl;

  int num_nonzero = 0;
  for (int p : partition) {
    if (p != 0) num_nonzero++;
  }

  long long mult_result = (result << (long long)num_nonzero);
  cout << result << " * 2^" << num_nonzero << " = " << mult_result << endl;

  return mult_result;
}

long long count_space_make_partition(
  shared_ptr<Module> module, int k, int maxVars, int sum,
  vector<int> const& partition)
{
  if (partition.size() == module->sorts.size()) {
    return count_space_for_partition(module, k, partition);
  }
  long long res = 0;
  for (int i = 0; sum + i <= maxVars; i++) {
    vector<int> newp = partition;
    newp.push_back(i);
    res += count_space_make_partition(module, k, maxVars, sum + i, newp);
  }
  return res;
}

long long count_space(shared_ptr<Module> module, int k, int maxVars) {
  vector<int> partition;
  return count_space_make_partition(module, k, maxVars, 0, partition);
}
