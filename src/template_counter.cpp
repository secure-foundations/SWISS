#include "template_counter.h"

#include <iostream>
#include <cassert>

#include "logic.h"
#include "enumerator.h"
#include "var_lex_graph.h"
#include "tree_shapes.h"
#include "top_quantifier_desc.h"

using namespace std;

struct TransitionSystem {
  vector<vector<int>> transitions;

  TransitionSystem(vector<vector<int>> transitions)
      : transitions(transitions) { }

  // returns -1 if no edge
  int next(int state, int trans) const {
    return transitions[state][trans];
  }

  int nTransitions() const {
    return transitions[0].size();
  }
  int nStates() const {
    return transitions.size();
  }
};

struct Vector {
  vector<long long> v;

  void subtract(Vector const& other)
  {
    for (int i = 0; i < (int)v.size(); i++) {
      v[i] -= other.v[i];
    }
  }

  long long get_entry_or_sum(int idx) {
    if (idx == -1) {
      long long sum = 0;
      for (int i = 0; i < (int)v.size(); i++) {
        sum += v[i];
      }
      return sum;
    } else {
      return v[idx];
    }
  }
};

struct Matrix {
  vector<vector<long long>> m;
  Matrix() { }
  Matrix(int n) {
    m.resize(n);
    for (int i = 0; i < n; i++) {
      m[i].resize(n);
    }
  }
  void add(Matrix const& a)
  {
    int n = m.size();
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
        m[i][j] += a.m[i][j];
      }
    }
  }
  void add_product(Matrix const& a, Matrix const& b)
  {
    int n = m.size();
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
        for (int k = 0; k < n; k++) {
          m[i][k] += a.m[i][k] * b.m[k][j];
        }
      }
    }
  }
  void add_product(TransitionSystem const& ts, int trans, Matrix const& a, Matrix const& b)
  {
    int n = m.size();
    for (int i = 0; i < n; i++) {
      int i1 = ts.next(i, trans);
      if (i1 != -1) {
        for (int j = 0; j < n; j++) {
          for (int k = 0; k < n; k++) {
            m[i][j] += a.m[i1][k] * b.m[k][j];
          }
        }
      }
    }
  }
  void add_product(TransitionSystem const& ts, int trans, Matrix const& a)
  {
    int n = m.size();
    for (int i = 0; i < n; i++) {
      int i1 = ts.next(i, trans);
      if (i1 != -1) {
        for (int j = 0; j < n; j++) {
          m[i][j] += a.m[i1][j];
        }
      }
    }
  }
  void set_to_identity()
  {
    int n = m.size();
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
        m[i][j] = 0;
      }
      m[i][i] = 1;
    }
  }
  long long count_from_to(int from, int to) {
    if (to == -1) {
      long long res = 0;
      int n = m.size();
      for (int i = 0; i < n; i++) {
        res += m[from][i];
      }
      return res;
    } else {
      return m[from][to];
    }
  }
  Vector get_row(int from) {
    Vector v;
    v.v = m[from];
    return v;
  }
};

struct GroupSpec {
  int groupSize;
  int nGroups;
  int firstMin;
  GroupSpec(
    int groupSize,
    int nGroups,
    int firstMin)
  : groupSize(groupSize), nGroups(nGroups), firstMin(firstMin) { }
};

map<pair<int, int>, vector<Matrix*>> group_spec_to_matrix;

Matrix* getMatrixForGroupSpec(GroupSpec gs, TransitionSystem const& ts) {
  auto key = make_pair(gs.groupSize, gs.nGroups);
  auto it = group_spec_to_matrix.find(key);
  if (it != group_spec_to_matrix.end()) {
    return it->second[gs.firstMin];
  }

  int m = ts.nTransitions();
  int n = ts.nStates();

  vector<Matrix*> res;
  res.resize(m + 1);

  if (gs.nGroups == 0) {
    res[m] = new Matrix(n);
    res[m]->set_to_identity();
    for (int i = m-1; i >= 0; i--) {
      res[i] = res[m];
    }
  } else {
    res[m] = new Matrix(n);

    for (int i = m - 1; i >= 0; i--) {
      res[i] = new Matrix();
      res[i]->m = res[i+1]->m;

      if (gs.groupSize >= 2) {
        for (int numSame = 1; numSame < gs.nGroups; numSame++) {
          res[i]->add_product(ts, i,
              *getMatrixForGroupSpec(GroupSpec(gs.groupSize - 1, numSame, i+1), ts),
              *getMatrixForGroupSpec(GroupSpec(gs.groupSize, gs.nGroups-numSame, i+1), ts));
        }
        res[i]->add_product(ts, i,
            *getMatrixForGroupSpec(GroupSpec(gs.groupSize - 1, gs.nGroups, i+1), ts));
      } else {
        assert (gs.groupSize == 1);
        res[i]->add_product(ts, i,
            *getMatrixForGroupSpec(GroupSpec(1, gs.nGroups - 1, i+1), ts));
      }
    }
  }

  group_spec_to_matrix[key] = res;
  return res[gs.firstMin];
}


EnumInfo::EnumInfo(std::shared_ptr<Module> module, value templ)
{
  clauses = get_clauses_for_template(module, templ);
  var_index_transitions = get_var_index_transitions(templ, clauses);
}

pair<TransitionSystem, int> build_transition_system(
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

  return make_pair(TransitionSystem(matrix), last_idx);
}

vector<Vector> countDepth1(
    TransitionSystem const& ts,
    int d)
{
  vector<Vector> res;
  res.resize(d + 1);

  for (int i = 1; i <= d; i++) {
    Matrix* m = getMatrixForGroupSpec(GroupSpec(1, i, 0), ts);
    res[i] = m->get_row(0);
  }

  return res;
}

vector<Vector> countDepth2(
    TransitionSystem const& ts,
    int d)
{
  if (d < 2) {
    d = 2;
  }

  int nStates = ts.nStates();

  // position, max len after position, state
  vector<vector<Matrix>> dp;
  dp.resize(d+1);
  for (int i = 0; i <= d; i++) {
    dp[i].resize(d);
    for (int j = 0; j < d; j++) {
      dp[i][j] = Matrix(nStates);
    }
  }

  for (int j = 0; j < d; j++) {
    dp[d][j].set_to_identity();
  }

  for (int i = d-1; i >= 0; i--) {
    for (int j = 1; j < d; j++) {
      dp[i][j] = dp[i][j-1];
      int groupSize = j;
      for (int nGroups = 1; i + groupSize * nGroups <= d; nGroups++) {
        Matrix *m = getMatrixForGroupSpec(GroupSpec(groupSize, nGroups, 0), ts);
        dp[i][j].add_product(*m, dp[i + groupSize * nGroups][groupSize - 1]);
      }
    }
  }

  vector<Vector> res;
  res.resize(d+1);
  for (int i = 1; i <= d; i++) {
    //cout << "yo " << dp[d-i][d][0] << endl;
    res[i] = dp[d-i][d-1].get_row(0);
  }

  for (int i = 2; i < d; i++) {
    Matrix *m = getMatrixForGroupSpec(GroupSpec(i, 1, 0), ts);
    res[i].subtract(m->get_row(0));
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
  pair<TransitionSystem, int> p = build_transition_system(
          get_var_index_init_state(templ),
          ei.var_index_transitions,
          get_num_vars(module, templ));
  cout << "clauses: " << ei.clauses.size() << endl;
  /*for (value v : ei.clauses) {
    cout << v->to_string() << endl;
  }*/
  cout << "nStates: " << p.first.nStates() << endl;

  auto matrix = p.first;
  int final = p.second;
  //auto matrix_rev = transpose_matrix(matrix);

  assert (final != -1);

  if (!useAllVars) {
    final = -1;
  }

  vector<Vector> counts;
  if (depth2) {
    counts = countDepth2(matrix, k);
  } else {
    counts = countDepth1(matrix, k);
  }

  long long total = 0;
  for (int i = 1; i <= k; i++) {
    long long v = counts[i].get_entry_or_sum(final);
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

/*void count_many_templates(
    shared_ptr<Module> module,
    int maxClauses,
    bool depth2,
    int maxVars) {
  value templ = make_template_with_max_vars(module, maxVars);
  
  EnumInfo ei(module, templ);
  pair<TransitionSystem, int> p = build_transition_system(
          get_var_index_init_state(templ),
          ei.var_index_transitions,
          -1);

  
}*/
