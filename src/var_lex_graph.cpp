#include "var_lex_graph.h"

#include <cassert>

#include "top_quantifier_desc.h"

using namespace std;

struct GI {
  int group;
  int index;
};

GI get_gi(vector<QSRange> const& groups, iden name)
{
  for (int i = 0; i < groups.size(); i++) {
    for (int j = 0; j < groups[i].decls.size(); j++) {
      if (groups[i].decls[j].name == name) {
        GI gi;
        gi.group = i;
        gi.index = j + 1;
        return gi;
      }
    }
  }
  assert(false);
}

void get_var_index_transition_rec(
  vector<QSRange> const& groups,
  value v,
  VarIndexTransition& vit)
{
  assert(v.get() != NULL);
  if (Forall* val = dynamic_cast<Forall*>(v.get())) {
    get_var_index_transition_rec(groups, val->body, vit);
  }
  else if (Exists* val = dynamic_cast<Exists*>(v.get())) {
    get_var_index_transition_rec(groups, val->body, vit);
  }
  else if (NearlyForall* val = dynamic_cast<NearlyForall*>(v.get())) {
    get_var_index_transition_rec(groups, val->body, vit);
  }
  else if (Var* val = dynamic_cast<Var*>(v.get())) {
    GI gi = get_gi(groups, val->name);
    // If the largest seen (`res`) jumps ahead by more than 1,
    // then update `pre`.
    if (gi.index - 1 > vit.res.indices[gi.group]) {
      vit.pre.indices[gi.group] = gi.index - 1;
    }
    // Update the largest seen (`res`)
    if (gi.index > vit.res.indices[gi.group]) {
      vit.res.indices[gi.group] = gi.index;
    }
  }
  else if (Const* val = dynamic_cast<Const*>(v.get())) {
  }
  else if (Eq* val = dynamic_cast<Eq*>(v.get())) {
    get_var_index_transition_rec(groups, val->left, vit);
    get_var_index_transition_rec(groups, val->right, vit);
  }
  else if (Not* val = dynamic_cast<Not*>(v.get())) {
    get_var_index_transition_rec(groups, val->val, vit);
  }
  else if (Implies* val = dynamic_cast<Implies*>(v.get())) {
    get_var_index_transition_rec(groups, val->left, vit);
    get_var_index_transition_rec(groups, val->right, vit);
  }
  else if (Apply* val = dynamic_cast<Apply*>(v.get())) {
    get_var_index_transition_rec(groups, val->func, vit);
    for (value arg : val->args) {
      get_var_index_transition_rec(groups, arg, vit);
    }
  }
  else if (And* val = dynamic_cast<And*>(v.get())) {
    for (value arg : val->args) {
      get_var_index_transition_rec(groups, arg, vit);
    }
  }
  else if (Or* val = dynamic_cast<Or*>(v.get())) {
    for (value arg : val->args) {
      get_var_index_transition_rec(groups, arg, vit);
    }
  }
  else if (IfThenElse* val = dynamic_cast<IfThenElse*>(v.get())) {
    get_var_index_transition_rec(groups, val->cond, vit);
    get_var_index_transition_rec(groups, val->then_value, vit);
    get_var_index_transition_rec(groups, val->else_value, vit);
  }
  else {
    assert(false && "get_var_index_transition_rec does not support this case");
  }
}

VarIndexTransition get_var_index_transition(
  vector<QSRange> const& groups,
  value v)
{
  int n = groups.size();

  VarIndexTransition vit;
  vit.pre.indices.resize(n);
  vit.res.indices.resize(n);
  for (int i = 0; i < n; i++) {
    vit.pre.indices[i] = 0;
    vit.res.indices[i] = 0;
  }

  get_var_index_transition_rec(groups, v, vit);
  return vit;
}

vector<VarIndexTransition> get_var_index_transitions(
  value templ,
  vector<value> const& values)
{
  TopQuantifierDesc tqd(templ);
  vector<QSRange> groups = tqd.grouped_by_sort();

  int ngroups = groups.size();

  for (int i = 0; i < ngroups; i++) {
    for (int j = 1; j < groups[i].decls.size(); j++) {
      if (!(iden_to_string(groups[i].decls[j-1].name) <
            iden_to_string(groups[i].decls[j].name))) {
        assert(false && "template args should be in alphabetical order (yeah it's dumb, but or else we might accidentally rely on two different sorting orders being the same or something)");
      }
    }
  }

  vector<VarIndexTransition> vits;

  for (int i = 0; i < values.size(); i++) {
    vits.push_back(get_var_index_transition(groups, values[i]));
  }

  return vits;
}

VarIndexState get_var_index_init_state(value templ)
{
  TopQuantifierDesc tqd(templ);
  vector<QSRange> groups = tqd.grouped_by_sort();
  int ngroups = groups.size();
  VarIndexState vis;
  vis.indices.resize(ngroups);
  for (int i = 0; i < ngroups; i++) {
    vis.indices[i] = 0;
  }
  return vis;
}