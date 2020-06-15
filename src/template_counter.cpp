#include "template_counter.h"

#include <iostream>
#include <cassert>

#include "logic.h"
#include "enumerator.h"
#include "var_lex_graph.h"
#include "tree_shapes.h"
#include "top_quantifier_desc.h"

using namespace std;

EnumInfo::EnumInfo(std::shared_ptr<Module> module, value templ)
{
  clauses = get_clauses_for_template(module, templ);
  var_index_transitions = get_var_index_transitions(templ, clauses);
}

pair<vector<vector<int>>, int> build_transition_matrix(
      VarIndexState const& init,
      std::vector<VarIndexTransition> const& transitions,
      int totalNumVars)
{
  vector<VarIndexState> idx_to_state;
  idx_to_state.push_back(init);
  vector<vector<int>> matrix;

  int cur = 0;
  while (cur < (int)idx_to_state.size()) {
    matrix.push_back(vector<int>(transitions.size()));

    VarIndexState cur_state = idx_to_state[cur];
    //cout << "cur " << cur_state.to_string() << endl;
    VarIndexState next(cur_state.indices.size());
    for (int i = 0; i < (int)transitions.size(); i++) {
      //cout << "transition " << i << endl;
      //cout << "pre " << transitions[i].pre.to_string() << endl;
      if (var_index_is_valid_transition(cur_state, transitions[i].pre)) {
        //cout << "cur " << cur_state.to_string() << endl;
        //cout << "res " << transitions[i].res.to_string() << endl;
        var_index_do_transition(cur_state, transitions[i].res, next);
        //cout << "next " << next.to_string() << endl;
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

  int last_idx = -1;

  for (int j = 0; j < (int)idx_to_state.size(); j++) {
    int sum = 0;
    for (int k = 0; k < (int)idx_to_state[j].indices.size(); k++) {
      sum += idx_to_state[j].indices[k];
    }

    if (sum == totalNumVars) {
      last_idx = j;
      break;
    }
  }

  return make_pair(matrix, last_idx);
}

vector<vector<vector<int>>> transpose_matrix(
    vector<vector<int>> const& matrix)
{
  int n = matrix[0].size();
  int m = matrix.size();

  vector<vector<vector<int>>> rev_matrix;
  rev_matrix.resize(m);
  for (int i = 0; i < m; i++) {
    rev_matrix[i].resize(n);
  }
  for (int i = 0; i < m; i++) {
    for (int j = 0; j < n; j++) {
      int k = matrix[i][j];
      if (k != -1) {
        rev_matrix[k][j].push_back(i);
      }
    }
  }
  return rev_matrix;
}

void count_prefixes_rec(
    int n, int m, int d,
    vector<vector<int>> const& matrix,
    vector<vector<vector<long long>>>& res,
    int idx, int state, int minNext)
{
  res[idx][state][minNext]++;
  if (idx < d) {
    for (int next = minNext; next < n; next++) {
      if (matrix[state][next] != -1) {
        count_prefixes_rec(n, m, d, matrix, res, idx+1, matrix[state][next], next+1);
      }
    }
  }
}

vector<vector<vector<long long>>> count_prefixes(
    vector<vector<int>> const& matrix,
    int d)
{
  int m = matrix.size();
  int n = matrix[0].size();
  vector<vector<vector<long long>>> res;
  res.resize(d+1);
  for (int i = 0; i <= d; i++) {
    res[i].resize(m);
    for (int j = 0; j < m; j++) {
      res[i][j].resize(n+1);
      for (int k = 0; k < n+1; k++) {
        res[i][j][k] = 0;
      }
    }
  }
  count_prefixes_rec(n, m, d, matrix, res, 0, 0, 0);
  return res;
}

void count_suffixes_rec(
    int n, int m, int d,
    vector<vector<vector<int>>> const& matrix_rev,
    vector<vector<vector<long long>>>& res,
    int idx, int state, int minNext)
{
  res[idx][state][minNext]++;
  if (idx < d) {
    for (int prev = minNext - 1; prev >= 0; prev--) {
      for (int prev_state : matrix_rev[state][prev]) {
        count_suffixes_rec(n, m, d, matrix_rev, res, idx+1, prev_state, prev);
      }
    }
  }
}

vector<vector<vector<long long>>> count_suffixes(
    vector<vector<vector<int>>> const& matrix_rev,
    int d, int final)
{
  int m = matrix_rev.size();
  int n = matrix_rev[0].size();
  vector<vector<vector<long long>>> res;
  res.resize(d+1);
  for (int i = 0; i <= d; i++) {
    res[i].resize(m);
    for (int j = 0; j < m; j++) {
      res[i][j].resize(n+1);
      for (int k = 0; k < n+1; k++) {
        res[i][j][k] = 0;
      }
    }
  }
  if (final != -1) {
    count_suffixes_rec(n, m, d, matrix_rev, res, 0, final, n);
  } else {
    for (int f = 0; f < m; f++) {
      count_suffixes_rec(n, m, d, matrix_rev, res, 0, f, n);
    }
  }
  for (int i = 0; i <= d; i++) {
    for (int j = 0; j < m; j++) {
      for (int k = n-1; k >= 0; k--) {
        res[i][j][k] += res[i][j][k+1];
      }
    }
  }
  return res;
}

long long meetInMiddle(
    vector<vector<vector<long long>>> const& pref,
    vector<vector<vector<long long>>> const& suff,
    int k)
{
  int a = k < (int)pref.size() - 1 ? k : (int)pref.size() - 1;
  int b = k - a;
  assert (0 <= a && a <= (int)pref.size() - 1);
  assert (0 <= b && b <= (int)suff.size() - 1);
  assert (a + b == k);
  long long res = 0;
  for (int i = 0; i < (int)pref[a].size(); i++) {
    for (int j = 0; j < (int)pref[a][i].size(); j++) {
      res += pref[a][i][j] * suff[b][i][j];
    }
  }
  return res;
}

int get_num_vars(shared_ptr<Module> module, value templ)
{
  TopAlternatingQuantifierDesc taqd(templ);
  vector<Alternation> alts = taqd.alternations();
  int count = 0;
  for (int j = 0; j < (int)alts.size(); j++) {
    count += alts[j].decls.size();
  }
  return count;
}

long long count_template(
    shared_ptr<Module> module,
    value templ,
    int k,
    bool depth2,
    bool useAllVars)
{
  EnumInfo ei(module, templ);
  pair<vector<vector<int>>, int> p = build_transition_matrix(
          get_var_index_init_state(templ),
          ei.var_index_transitions,
          get_num_vars(module, templ));
  cout << "clauses: " << ei.clauses.size() << endl;
  cout << "matrix: " << p.first.size() << endl;

  auto matrix = p.first;
  int final = p.second;
  auto matrix_rev = transpose_matrix(matrix);

  assert (final != -1);

  auto prefixes = count_prefixes(matrix, (k+1)/2);
  auto suffixes = count_suffixes(matrix_rev, k/2, useAllVars ? final : -1);

  long long total = 0;
  for (int i = 1; i <= k; i++) {
    long long v = meetInMiddle(prefixes, suffixes, i);
    cout << "k = " << i << " : " << v << endl;
    total += v;
  }
  cout << "total = " << total << endl;
  return total;
}

/*
vector<vector<int>> get_matrix_counts(
  vector<vector<int>> const& matrix,
  int final)
{
  vector<vector<int>> res;
  for (int state = 0; state < (int)matrix.size(); state++) {
    res.push_back(vector<int>(matrix[state].size() + 1));
    res[state][matrix[state].size()] = 0;
    for (int j = matrix[state].size() - 1; j >= 0; j--) {
      res[state][j] = res[state][j+1] + (matrix[state][j] == final ? 1 : 0);
    }
  }
  return res;
}

long long rec_main(
  vector<vector<int>> const& matrix,
  vector<vector<int>> const& matrix_counts,
  int final,
  TreeShape const& ts,
  vector<int>& indices,
  int idx,
  int state)
{
  if (idx == (int)indices.size() - 1) {
    int minLast = is_normalized_for_tree_shape_get_min_of_last(ts, indices);
    //for (int i = 0; i < (int)indices.size()-1; i++) cout << indices[i] << " ";
    //cout << "_ " << ts.to_string() << " " << minLast << endl;
    if (minLast != -1) {
      return matrix_counts[state][minLast];
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
      res += rec_main(matrix, matrix_counts, final, ts, indices, idx+1, matrix[state][j]);
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

  //std::vector<VarIndexState> var_index_states;
  //var_index_states.push_back(get_var_index_init_state(templ));
  //for (int i = 1; i < total_arity + 2; i++) {
  //  var_index_states.push_back(var_index_states[0]);
  //}

  std::vector<VarIndexTransition> var_index_transitions =
      get_var_index_transitions(templ, pieces);

  auto p = build_transition_matrix(
      get_var_index_init_state(templ),
      var_index_transitions,
      partition);
  vector<vector<int>> transition_matrix = p.first;
  int start = 0;
  int final = p.second;

  vector<vector<int>> matrix_counts = get_matrix_counts(transition_matrix, final);

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
          transition_matrix, matrix_counts, final, tree_shapes[i], indices, 0, start);
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
*/
