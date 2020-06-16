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

vector<vector<vector<long long>>> getCountsFromVarStateToVarState(
    vector<vector<int>> const& matrix,
    int d)
{
  int m = matrix.size();
  int n = matrix[0].size();

  vector<vector<vector<long long>>> res;
  res.resize(d+1);
  for (int i = 0; i < d+1; i++) {
    res[i].resize(m);
    for (int j = 0; j < m; j++) {
      res[i][j].resize(m);
    }
  }

  vector<vector<vector<long long>>> dp;
  dp.resize(d+1);
  for (int i = 0; i < d+1; i++) {
    dp[i].resize(m);
    for (int j = 0; j < m; j++) {
      dp[i][j].resize(n+1);
    }
  }

  for (int final = 0; final < m; final++) {
    for (int j = 0; j < m; j++) {
      for (int k = 0; k <= n; k++) {
        dp[d][j][k] = (j == final ? 1 : 0);
      }
    }

    for (int i = d-1; i >= 0; i--) {
      for (int j = 0; j < m; j++) {
        dp[i][j][n] = 0;
        for (int k = n-1; k >= 0; k--) {
          dp[i][j][k] = dp[i][j][k+1];
          if (matrix[j][k] != -1) {
            dp[i][j][k] += dp[i+1][matrix[j][k]][k+1];
          }
        }
      }
    }

    for (int i = 0; i <= d; i++) {
      for (int j = 0; j < m; j++) {
        res[i][j][final] = dp[d-i][j][0];
      }
    }
  }

  return res;
}

void multiplyMatrixRec(
    vector<vector<int>> const& matrix,
    vector<vector<int>>& matrix1,
    vector<int>& indices,
    int i,
    int& idx)
{
  int n = matrix[0].size();

  if (i == (int)indices.size()) {
    int m = matrix.size();
    for (int t = 0; t < m; t++) {
      int u = t;
      for (int j = 0; j < (int)indices.size(); j++) {
        u = matrix[u][indices[j]];
        if (u == -1) {
          break;
        }
      }
      matrix1[t][idx] = u;
    }

    idx++;
    return;
  }

  int next = (i == 0 ? 0 : indices[i-1] + 1);
  for (int j = next; j < n; j++) {
    indices[i] = j;
    multiplyMatrixRec(matrix, matrix1, indices, i+1, idx);
  }
}

vector<vector<int>> multiplyMatrix(
    vector<vector<int>> matrix,
    int k)
{
  assert (k >= 1);
  if (k == 1) {
    return matrix;
  }

  int m = matrix.size();
  int n = matrix[0].size();

  long long n1 = 1;
  for (int i = n; i >= n - k + 1; i--) {
    n1 *= (long long)i;
    n1 /= n - i + 1;
  }

  vector<vector<int>> matrix1;
  matrix1.resize(m);
  for (int i = 0; i < m; i++) {
    matrix1[i].resize(n1);
  }

  vector<int> indices;
  indices.resize(k);
  int idx = 0;
  multiplyMatrixRec(matrix, matrix1, indices, 0, idx);

  return matrix1;
}

vector<vector<vector<long long>>> getCountsFromVarStateToVarStateForGroups(
    vector<vector<int>> const& matrix,
    int groupSize, int nGroups)
{
  assert (groupSize > 0);
  if (nGroups == 1) {
    vector<vector<vector<long long>>> r =
      getCountsFromVarStateToVarState(matrix, groupSize);

    vector<vector<vector<long long>>> res;
    res.resize(2);
    res[0] = r[0];
    res[1] = r[groupSize];
    return res;
  } else {
    return getCountsFromVarStateToVarState(
        multiplyMatrix(matrix, groupSize), nGroups);
  }
}

vector<long long> countDepth1(
    vector<vector<int>> const& matrix,
    int d,
    int final)
{
  vector<vector<vector<long long>>> trans =
      getCountsFromVarStateToVarStateForGroups(matrix, 1, d);
  vector<long long> res;
  res.resize(d + 1);
  res[0] = 0;

  int m = matrix.size();

  for (int i = 1; i <= d; i++) {
    if (final == -1) {
      res[i] = 0;
      for (int j = 0; j < m; j++) {
        res[i] += trans[i][0][j];
        //cout << i << " " << j << ", " << trans[i][0][j] << endl;
      }
    } else {
      res[i] = trans[i][0][final];
    }
  }

  return res;
}

vector<long long> countDepth2(
    vector<vector<int>> const& matrix,
    int d,
    int final)
{
  if (d < 2) {
    d = 2;
  }

  // groupSize, nGroups, startState, endState
  vector<vector<vector<vector<long long>>>> trans;
  trans.push_back({});
  for (int i = 1; i < d; i++) {
    trans.push_back(getCountsFromVarStateToVarStateForGroups(
        matrix, i, d / i));
  }

  int m = matrix.size();

  /*for (int i = 1; i <= (int)trans.size(); i++) {
    for (int j = 1; j < (int)trans[i].size(); j++) {
      for (int k = 0; k < m; k++) {
        for (int l = 0; l < m; l++) {
          cout << "(" << j << " groups of " << i << ") from " << k << " to " << l << " gives " << trans[i][j][k][l] << endl;
        }
      }
    }
  }*/

  // position, max len after position, state
  vector<vector<vector<long long>>> dp;
  dp.resize(d+1);
  for (int i = 0; i <= d; i++) {
    dp[i].resize(d);
    for (int j = 0; j < d; j++) {
      dp[i][j].resize(m);
    }
  }

  for (int j = 0; j < d; j++) {
    for (int k = 0; k < m; k++) {
      dp[d][j][k] = (final == -1 || k == final ? 1 : 0);
    }
  }

  for (int i = d-1; i >= 0; i--) {
    for (int k = 0; k < m; k++) {
      dp[i][0][k] = 0;
    }

    for (int j = 1; j < d; j++) {
      for (int k = 0; k < m; k++) {
        dp[i][j][k] = dp[i][j-1][k];
        int groupSize = j;
        for (int nGroups = 1; nGroups < (int)trans[groupSize].size() && i + groupSize * nGroups <= d; nGroups++) {
          //cout << "i = " << i << " nGroups = " << nGroups << " groupSize = " << groupSize << endl;
          for (int l = 0; l < m; l++) {
            dp[i][j][k] += trans[groupSize][nGroups][k][l] * dp[i + groupSize * nGroups][groupSize - 1][l];
          }
        }
        //cout << "dp[" << i << "][" << j << "][" << k << "] = " << dp[i][j][k] << endl;
      }
    }
  }

  vector<long long> res;
  res.resize(d+1);
  res[0] = 0;
  for (int i = 1; i <= d; i++) {
    //cout << "yo " << dp[d-i][d][0] << endl;
    res[i] = dp[d-i][d-1][0];
  }

  for (int i = 2; i < d; i++) {
    if (final == -1) {
      for (int l = 0; l < m; l++) {
        res[i] -= trans[i][1][0][l];
      }
    } else {
      res[i] -= trans[i][1][0][final];
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
  /*for (value v : ei.clauses) {
    cout << v->to_string() << endl;
  }*/
  cout << "matrix: " << p.first.size() << endl;

  auto matrix = p.first;
  int final = p.second;
  auto matrix_rev = transpose_matrix(matrix);

  assert (final != -1);

  if (!useAllVars) {
    final = -1;
  }

  vector<long long> counts;
  if (depth2) {
    counts = countDepth2(matrix, k, final);
  } else {
    counts = countDepth1(matrix, k, final);
  }

  long long total = 0;
  for (int i = 1; i <= k; i++) {
    long long v = counts[i];
    cout << "k = " << i << " : " << v << endl;

    if (depth2 && i > 1) {
      total += 2*v;
    } else {
      total += v;
    }
  }
  cout << "total = " << total << endl;
  return total;

  /*{
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
  }*/
}
