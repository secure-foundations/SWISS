#ifndef TEMPLATE_COUNTER_H
#define TEMPLATE_COUNTER_H

#include "logic.h"
#include "var_lex_graph.h"
#include "template_desc.h"

#include <algorithm>

struct TransitionSystem {
  std::vector<std::vector<int>> transitions;
  std::vector<std::vector<int>> state_reps;

  TransitionSystem() { }

  TransitionSystem(
      std::vector<std::vector<int>> transitions,
      std::vector<std::vector<int>> state_reps)
      : transitions(transitions), state_reps(state_reps) { }

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

  TransitionSystem reorder_rows(std::vector<int> const& maps_to) {
    std::vector<int> rev;
    rev.resize(transitions.size());
    for (int i = 0; i < (int)rev.size(); i++) {
      rev[i] = -1;
    }
    for (int i = 0; i < (int)maps_to.size(); i++) {
      rev[maps_to[i]] = i;
    }

    std::vector<std::vector<int>> transitions1;
    std::vector<std::vector<int>> state_reps1;
    for (int i = 0; i < (int)maps_to.size(); i++) {
      state_reps1.push_back(state_reps[maps_to[i]]);
      transitions1.push_back(transitions[maps_to[i]]);
      for (int j = 0; j < (int)transitions1[i].size(); j++) {
        if (transitions1[i][j] != -1) {
          transitions1[i][j] = rev[transitions1[i][j]];
        }
      }
    }
    return TransitionSystem(transitions1, state_reps1);
  }

  TransitionSystem cap_total_vars(int mvars) {
    std::vector<int> rows_to_keep;
    for (int i = 0; i < (int)transitions.size(); i++) {
      int sum = 0;
      for (int j : state_reps[i]) {
        sum += j;
      }
      if (sum <= mvars) {
        rows_to_keep.push_back(i);
      }
    }
    return reorder_rows(rows_to_keep);
  }

  TransitionSystem make_upper_triangular() {
    std::vector<std::pair<int, int>> v;
    for (int i = 0; i < (int)transitions.size(); i++) {
      int sum = 0;
      for (int j : state_reps[i]) {
        sum += j;
      }
      v.push_back(std::make_pair(sum, i));
    }
    std::sort(v.begin(), v.end());
    std::vector<int> rows;
    for (auto p : v) {
      rows.push_back(p.second);
    }
    return reorder_rows(rows);
  }

  TransitionSystem remove_unused_transitions() {
    int n = nStates();
    int m = nTransitions();
    std::vector<bool> used;
    used.resize(m);
    for (int i = 0; i < m; i++) {
      bool u = false;
      for (int j = 0; j < n; j++) {
        if (transitions[j][i] != -1) {
          u = true;
          break;
        }
      }
      used[i] = u;
    }
    std::vector<std::vector<int>> transitions1;
    for (int i = 0; i < n; i++) {
      std::vector<int> row1;
      for (int j = 0; j < m; j++) {
        if (used[j]) {
          row1.push_back(transitions[i][j]);
        }
      }
      transitions1.push_back(row1);
    }
    return TransitionSystem(transitions1, state_reps);
  }
};

TransitionSystem build_transition_system(
      VarIndexState const& init,
      std::vector<VarIndexTransition> const& transitions);

//long long count_space(std::shared_ptr<Module> module, int k, int maxVars);
long long count_template(
    std::shared_ptr<Module> module,
    value templ,
    int k,
    bool depth2,
    bool useAllVars);

struct EnumInfo {
  std::vector<value> clauses;
  std::vector<VarIndexTransition> var_index_transitions;

  EnumInfo(std::shared_ptr<Module>, value templ);
};

std::vector<TemplateDesc> count_many_templates(
    std::shared_ptr<Module> module,
    int maxClauses,
    bool depth2,
    int maxVars);

#endif
