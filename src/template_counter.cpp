#include "template_counter.h"

#include <iostream>
#include <cassert>
#include <algorithm>
#include <unordered_map>
#include <set>

#include "logic.h"
#include "enumerator.h"
#include "var_lex_graph.h"
#include "tree_shapes.h"
#include "top_quantifier_desc.h"

using namespace std;

struct Vector {
  vector<unsigned long long> v;

  void subtract(Vector const& other)
  {
    for (int i = 0; i < (int)v.size(); i++) {
      v[i] -= other.v[i];
    }
  }

  unsigned long long get_entry_or_sum(int idx) {
    if (idx == -1) {
      unsigned long long sum = 0;
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
  vector<vector<unsigned long long>> m;
  Matrix() { }
  Matrix(int n) {
    m.resize(n);
    for (int i = 0; i < n; i++) {
      m[i].resize(n-i);
    }
  }
  void add(Matrix const& a)
  {
    int n = m.size();
    for (int i = 0; i < n; i++) {
      for (int j = i; j < n; j++) {
        m[i][j-i] += a.m[i][j-i];
      }
    }
  }
  void add_product(Matrix const& a, Matrix const& b)
  {
    int n = m.size();
    for (int i = 0; i < n; i++) {
      for (int k = i; k < n; k++) {
        for (int j = k; j < n; j++) {
          m[i][j-i] += a.m[i][k-i] * b.m[k][j-k];
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
        for (int k = i1; k < n; k++) {
          for (int j = k; j < n; j++) {
            m[i][j-i] += a.m[i1][k-i1] * b.m[k][j-k];
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
        for (int j = i1; j < n; j++) {
          m[i][j-i] += a.m[i1][j-i1];
        }
      }
    }
  }
  void set_to_identity()
  {
    int n = m.size();
    for (int i = 0; i < n; i++) {
      for (int j = i+1; j < n; j++) {
        m[i][j-i] = 0;
      }
      m[i][i-i] = 1;
    }
  }
  unsigned long long count_from_to(int from, int to) {
    if (to == -1) {
      unsigned long long res = 0;
      int n = m.size();
      for (int i = from; i < n; i++) {
        res += m[from][i-from];
      }
      return res;
    } else {
      if (to >= from) {
        return m[from][to-from];
      } else {
        return 0;
      }
    }
  }
  Vector get_row(int from) {
    Vector v;
    v.v.resize(m.size());
    for (int i = from; i < (int)m.size(); i++) {
      v.v[i] = m[from][i-from];
    }
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

map<pair<int, int>, vector<shared_ptr<Matrix>>> group_spec_to_matrix;

Matrix* getMatrixForGroupSpec(GroupSpec gs, TransitionSystem const& ts) {
  auto key = make_pair(gs.groupSize, gs.nGroups);
  auto it = group_spec_to_matrix.find(key);
  if (it != group_spec_to_matrix.end()) {
    return it->second[gs.firstMin].get();
  }

  int m = ts.nTransitions();
  int n = ts.nStates();

  vector<shared_ptr<Matrix>> res;
  res.resize(m + 1);

  if (gs.nGroups == 0) {
    res[m] = shared_ptr<Matrix>(new Matrix(n));
    res[m]->set_to_identity();
    for (int i = m-1; i >= 0; i--) {
      res[i] = res[m];
    }
  } else {
    res[m] = shared_ptr<Matrix>(new Matrix(n));

    for (int i = m - 1; i >= 0; i--) {
      res[i] = shared_ptr<Matrix>(new Matrix());
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
  //cout << group_spec_to_matrix.size() << endl;
  return res[gs.firstMin].get();
}


EnumInfo::EnumInfo(std::shared_ptr<Module> module, value templ)
{
  clauses = get_clauses_for_template(module, templ);
  var_index_transitions = get_var_index_transitions(module, templ, clauses);
}

TransitionSystem build_transition_system(
      VarIndexState const& init,
      std::vector<VarIndexTransition> const& transitions,
      int maxVars)
{
  vector<VarIndexState> idx_to_state;
  idx_to_state.push_back(init);
  vector<vector<int>> matrix;

  unordered_map<VarIndexState, int> state_to_idx;
  state_to_idx.insert(make_pair(init, 0));

  int cur = 0;
  while (cur < (int)idx_to_state.size()) {
    //cout << "cur " << cur << endl;
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

        bool okay;
        if (maxVars != -1) {
          int sum = 0;
          for (int j : next.indices) { sum += j; }
          okay = (sum <= maxVars);
        } else {
          okay = true;
        }

        if (okay) {
          int next_idx;
          auto iter = state_to_idx.find(next);
          if (iter == state_to_idx.end()) {
            idx_to_state.push_back(next);
            state_to_idx.insert(make_pair(next, idx_to_state.size() - 1));
            next_idx = idx_to_state.size() - 1;
          } else {
            next_idx = iter->second;
          }
          matrix[cur][i] = next_idx;
        } else {
          matrix[cur][i] = -1;
        }
      } else {
        matrix[cur][i] = -1;
      }
    }

    cur++;
  }

  vector<vector<int>> state_reps;
  for (int i = 0; i < (int)idx_to_state.size(); i++) {
    state_reps.push_back(idx_to_state[i].indices);
  }

  return TransitionSystem(matrix, state_reps);
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

unsigned long long count_template(
    shared_ptr<Module> module,
    value templ,
    int k,
    bool depth2,
    bool useAllVars)
{
  EnumInfo ei(module, templ);
  TransitionSystem ts = build_transition_system(
          get_var_index_init_state(module, templ),
          ei.var_index_transitions,
          -1);

  ts = ts.cap_total_vars(get_num_vars(module, templ));
  ts = ts.remove_unused_transitions();
  ts = ts.make_upper_triangular();

  cout << "clauses: " << ei.clauses.size() << endl;
  /*for (value v : ei.clauses) {
    cout << v->to_string() << endl;
  }*/
  cout << "nStates: " << ts.nStates() << endl;

  int final = ts.nStates() - 1;

  assert (final != -1);

  if (!useAllVars) {
    final = -1;
  }

  vector<Vector> counts;
  if (depth2) {
    counts = countDepth2(ts, k);
  } else {
    counts = countDepth1(ts, k);
  }

  group_spec_to_matrix.clear();

  unsigned long long total = 0;
  for (int i = 1; i <= k; i++) {
    unsigned long long v = counts[i].get_entry_or_sum(final);

    if (depth2 && i > 1) {
      v *= 2;
    }

    cout << "k = " << i << " : " << v << endl;

    total += v;
  }
  cout << "total = " << total << endl;
  return total;
}

vector<TemplateSlice> count_many_templates(
    shared_ptr<Module> module,
    value templ,
    int maxClauses,
    bool depth2,
    int maxVars)
{
  EnumInfo ei(module, templ);
  TransitionSystem ts = build_transition_system(
          get_var_index_init_state(module, templ),
          ei.var_index_transitions,
          maxVars);
  if (maxVars != -1) {
    ts = ts.cap_total_vars(maxVars);
  }
  ts = ts.remove_unused_transitions();
  ts = ts.make_upper_triangular();

  cout << "states " << ts.nStates() << endl;
  cout << "transitions " << ts.nTransitions() << endl;

  vector<Vector> counts;
  if (depth2) {
    counts = countDepth2(ts, maxClauses);
  } else {
    counts = countDepth1(ts, maxClauses);
  }

  group_spec_to_matrix.clear();

  vector<TemplateSlice> tds;

  for (int d = 1; d <= maxClauses; d++) {
    for (int i = 0; i < ts.nStates(); i++) {
      TemplateSlice td;
      td.vars = ts.state_reps[i];
      td.quantifiers.resize(td.vars.size());
      for (int j = 0; j < (int)td.quantifiers.size(); j++) {
        td.quantifiers[j] = Quantifier::Forall;
      }
      td.k = d;
      td.depth = (depth2 ? 2 : 1);
      td.count = (depth2 && d > 1 ? 2 : 1) * counts[d].v[i];
      tds.push_back(td);
    }
  }
  sort(tds.begin(), tds.end());
  /*for (TemplateSlice const& td : tds) {
    cout << td.to_string(module) << endl;
  }*/

  return tds;
}

struct Partial {
  int capTo;

  int sort_idx;
  int sort_exact_num;
  int other_bound;

  int a_idx;
  int b_idx;
  int v;
  int w;

  Partial() : capTo(-1), sort_idx(-1), sort_exact_num(-1), a_idx(-1), b_idx(-1) { }
};

value make_template_with_max_vars(shared_ptr<Module> module, int maxVars,
    Partial partial)
{
  vector<VarDecl> decls;
  int idx = 0;
  int so_idx = 0;
  for (string so_name : module->sorts) {
    //int v = (so_idx == 1 ? maxVars - 2 : 2);
    int v;
    if (partial.capTo != -1) {
      v = partial.capTo;
    } else if (partial.sort_idx != -1) {
      v = (so_idx == partial.sort_idx ? partial.sort_exact_num
          : partial.other_bound);
    } else if (partial.a_idx != -1) {
      v = (so_idx == partial.a_idx || so_idx == partial.b_idx
          ? partial.v : partial.w);
    } else {
      assert(false);
    }

    lsort so = s_uninterp(so_name);
    for (int i = 0; i < v; i++) {
      idx++;
      string s = to_string(idx);
      while (s.size() < 4) { s = "0" + s; }
      s = "A" + s;

      decls.push_back(VarDecl(string_to_iden(s), so));
    }

    so_idx++;
  }
  return v_forall(decls, v_template_hole());
}

value make_template_with_max_vars(shared_ptr<Module> module, vector<int> const& va)
{
  vector<VarDecl> decls;
  int idx = 0;
  int so_idx = 0;
  for (string so_name : module->sorts) {
    int v = va[so_idx];

    lsort so = s_uninterp(so_name);
    for (int i = 0; i < v; i++) {
      idx++;
      string s = to_string(idx);
      while (s.size() < 4) { s = "0" + s; }
      s = "A" + s;

      decls.push_back(VarDecl(string_to_iden(s), so));
    }

    so_idx++;
  }
  return v_forall(decls, v_template_hole());
}

vector<TemplateSlice> count_many_templates(
    shared_ptr<Module> module,
    int maxClauses,
    bool depth2,
    int maxVars)
{
  vector<TemplateSlice> res;

  if (maxVars % 2 == 1 && maxVars >= 5) {
    vector<Partial> partials;

    int halfCap = maxVars / 2 - 1;
    Partial m;
    m.capTo = halfCap;
    partials.push_back(m);

    /*for (int i = 0; i < (int)module->sorts.size(); i++) {
      for (int j = maxVars / 2; j < (int)module->sorts.size(); j++) {
        Partial p;
        p.sort_idx = i;
        p.sort_exact_num = j;
        p.other_bound = min(maxVars - j, maxVars / 2 - 1);
        partials.push_back(p);
      }
    }*/

    /*for (int i = 0; i < (int)module->sorts.size(); i++) {
      for (int j = i+1; j < (int)module->sorts.size(); j++) {
        Partial p;
        p.a_idx = i;
        p.b_idx = j;
        p.v = maxVars / 2 + 1;
        p.w = 1;
        partials.push_back(p);
      }
    }*/

    for (Partial partial : partials) {
      cout << "partial" << endl;
      value templ = make_template_with_max_vars(module, maxVars, partial);
      vector<TemplateSlice> slices = count_many_templates(module, templ, maxClauses, depth2, maxVars);
      if (partial.sort_idx != -1) {
        for (TemplateSlice const& ts : slices) {
          if (ts.vars[partial.sort_idx] == partial.sort_exact_num) {
            cout << ts << endl;
            res.push_back(ts);
          }
        }
      } else if (partial.a_idx != -1) {
        for (TemplateSlice const& ts : slices) {
          if (ts.vars[partial.a_idx] >= partial.v - 1
           && ts.vars[partial.b_idx] >= partial.v - 1) {
            cout << ts << endl;
            res.push_back(ts);
          }
        }
      } else {
        for (TemplateSlice const& ts : slices) {
          cout << ts << endl;
          res.push_back(ts);
        }
      }
    }

    set<vector<int>> seen;
    for (TemplateSlice const& ts : res) {
      seen.insert(ts.vars);
    }

    vector<int> v;
    v.resize(module->sorts.size());
    while (true) {
      bool okay = false;
      int sum = 0;
      for (int i = 0; i < (int)v.size(); i++) {
        if (v[i] >= maxVars / 2) {
          okay = true;
        }
        sum += v[i];
      }

      if (okay && sum == maxVars) {
        cout << "doing ";
        for (int w : v) cout << w << " ";
        cout << endl;

        value templ = make_template_with_max_vars(module, v);
        vector<TemplateSlice> slices = count_many_templates(module, templ, maxClauses, depth2, maxVars);

        for (TemplateSlice const& ts : slices) {
          if (seen.find(ts.vars) == seen.end()) {
            cout << ts << endl;
            res.push_back(ts);
          }
        }
        for (TemplateSlice const& ts : slices) {
          seen.insert(ts.vars);
        }
      }

      int i;
      for (i = 0; i < (int)v.size(); i++) {
        v[i]++;
        if (v[i] == maxVars+1) {
          v[i] = 0;
        } else {
          break;
        }
      }
      if (i == (int)v.size()) {
        break;
      }
    }
  } else {
    vector<Partial> partials;

    int halfCap = maxVars / 2;
    Partial m;
    m.capTo = halfCap;
    partials.push_back(m);

    for (int i = 0; i < (int)module->sorts.size(); i++) {
      for (int j = halfCap + 1; j <= maxVars; j++) {
        Partial p;
        p.sort_idx = i;
        p.sort_exact_num = j;
        p.other_bound = maxVars - j;
        partials.push_back(p);
      }
    }

    for (Partial partial : partials) {
      cout << "partial" << endl;
      value templ = make_template_with_max_vars(module, maxVars, partial);
      vector<TemplateSlice> slices = count_many_templates(module, templ, maxClauses, depth2, maxVars);
      if (partial.sort_idx != -1) {
        for (TemplateSlice const& ts : slices) {
          if (ts.vars[partial.sort_idx] == partial.sort_exact_num) {
            cout << ts << endl;
            res.push_back(ts);
          }
        }
      } else {
        for (TemplateSlice const& ts : slices) {
          cout << ts << endl;
          res.push_back(ts);
        }
      }
    }
  }

  sort(res.begin(), res.end());

  return res;
}

vector<TemplateSlice> count_many_templates(
    shared_ptr<Module> module,
    TemplateSpace const& ts)
{
  value templ = ts.make_templ(module);
  vector<TemplateSlice> slices = count_many_templates(module, templ, ts.k, (ts.depth == 2), -1);
  for (int i = 0; i < (int)slices.size(); i++) {
    slices[i].quantifiers = ts.quantifiers;
    assert (slices[i].quantifiers.size() == slices[i].vars.size());
    for (int j = 0; j < (int)slices[i].quantifiers.size(); j++) {
      if (slices[i].vars[j] == 0) {
        slices[i].quantifiers[j] = Quantifier::Forall;
      }
    }
  }
  return slices;
}

pair<std::pair<std::vector<int>, TransitionSystem>, int> get_subslice_index_map(
    TransitionSystem const& ts,
    TemplateSlice const& tslice)
{
  vector<bool> in_slice;
  in_slice.resize(ts.nStates());
  for (int i = 0; i < ts.nStates(); i++) {
    in_slice[i] = true;
    for (int j = 0; j < (int)tslice.vars.size(); j++) {
      if (tslice.vars[j] < ts.state_reps[i][j]) {
        in_slice[i] = false;
        break;
      }
    }
  }

  vector<int> res;
  vector<bool> trans_keep;
  trans_keep.resize(ts.nTransitions());
  for (int i = 0; i < ts.nTransitions(); i++) {
    bool is_okay = false;
    for (int j = 0; j < ts.nStates(); j++) {
      if (in_slice[j] && ts.next(j, i) != -1 && in_slice[ts.next(j, i)]) {
        is_okay = true;
        break;
      }
    }
    trans_keep[i] = is_okay;
    if (is_okay) {
      res.push_back(i);
    }
  }

  int target_state = -1;
  for (int i = 0; i < ts.nStates(); i++) {
    if (ts.state_reps[i] == tslice.vars) {
      target_state = i;
      break;
    }
  }
  //assert(target_state != -1);

  return make_pair(make_pair(res, ts.remove_transitions(trans_keep)), target_state);
}
