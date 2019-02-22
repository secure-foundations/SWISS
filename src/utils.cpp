#include "utils.h"

#include <algorithm>
#include <vector>
#include <cassert>

using namespace std;

namespace {
struct NormalizeState {
  vector<iden> names;
  iden get_name(iden name) {
    int idx = -1;
    for (int i = 0; i < names.size(); i++) {
      if (names[i] == name) {
        idx = i;
        break;
      }
    }
    if (idx == -1) {
      names.push_back(name);
      idx = names.size() - 1;
    }
    return idx;
  }
};

value normalize(value v, NormalizeState& ns) {
  assert(v.get() != NULL);
  if (Forall* va = dynamic_cast<Forall*>(v.get())) {
    return v_forall(va->decls, normalize(va->body, ns));
  }
  else if (Exists* va = dynamic_cast<Exists*>(v.get())) {
    return v_exists(va->decls, normalize(va->body, ns));
  }
  else if (Var* va = dynamic_cast<Var*>(v.get())) {
    return v_var(ns.get_name(va->name), va->sort);
  }
  else if (Const* va = dynamic_cast<Const*>(v.get())) {
    return v;
  }
  else if (Eq* va = dynamic_cast<Eq*>(v.get())) {
    return v_eq(
        normalize(va->left, ns),
        normalize(va->right, ns));
  }
  else if (Not* va = dynamic_cast<Not*>(v.get())) {
    return v_not(
        normalize(va->val, ns));
  }
  else if (Implies* va = dynamic_cast<Implies*>(v.get())) {
    return v_implies(
        normalize(va->left, ns),
        normalize(va->right, ns));
  }
  else if (Apply* va = dynamic_cast<Apply*>(v.get())) {
    value func = normalize(va->func, ns);
    vector<value> args;
    for (value arg : va->args) {
      args.push_back(normalize(arg, ns));
    }
    return v_apply(func, args);
  }
  else if (And* va = dynamic_cast<And*>(v.get())) {
    vector<value> args;
    for (value arg : va->args) {
      args.push_back(normalize(arg, ns));
    }
    return v_and(args);
  }
  else if (Or* va = dynamic_cast<Or*>(v.get())) {
    vector<value> args;
    for (value arg : va->args) {
      args.push_back(normalize(arg, ns));
    }
    return v_or(args);
  }
  else {
    //printf("value2expr got: %s\n", templ->to_string().c_str());
    assert(false && "value2expr does not support this case");
  }
}
}

bool is_redundant_quick(value a, value b)
{
  while (true) {
    Forall* a_f = dynamic_cast<Forall*>(a.get());
    Forall* b_f = dynamic_cast<Forall*>(b.get());
    if (a_f == NULL || b_f == NULL) {
      break;
    }

    if (a_f->decls.size() != b_f->decls.size()) {
      return false;
    }
    for (int i = 0; i < a_f->decls.size(); i++) {
      if (a_f->decls[i].name != b_f->decls[i].name) {
        return false;
      }
    }

    a = a_f->body;
    b = b_f->body;
  }

  Not* a_n = dynamic_cast<Not*>(a.get());
  Not* b_n = dynamic_cast<Not*>(b.get());

  if (a_n == NULL || b_n == NULL) {
    return false;
  }

  And* a_and = dynamic_cast<And*>(a_n->val.get());
  And* b_and = dynamic_cast<And*>(b_n->val.get());
  vector<value> a_and_args;
  vector<value> b_and_args;

  if (a_and == NULL) {
    a_and_args.push_back(a_n->val);
  } else {
    a_and_args = a_and->args;
  }

  if (b_and == NULL) {
    b_and_args.push_back(b_n->val);
  } else {
    b_and_args = b_and->args;
  }

  int n = a_and_args.size();
  int m = b_and_args.size();
  if (!(n < m)) {
    return false;
  }

  NormalizeState ns;
  string a_str = normalize(v_and(a_and_args), ns)->to_string();

  vector<int> perm;
  for (int i = 0; i < m; i++) {
    perm.push_back(i);
  }
  do {
    vector<value> b_subset;
    for (int i = 0; i < n; i++) {
      b_subset.push_back(b_and_args[perm[i]]);
    }
    value b_and_subset = v_and(b_subset);

    NormalizeState ns;
    string b_str = normalize(b_and_subset, ns)->to_string();

    //printf("cmp %s %s %s\n", a_str.c_str(), b_str.c_str(), a_str == b_str ? "yes" : "no");
    if (a_str == b_str) {
      return true;
    }

    reverse(perm.begin() + n, perm.end());
  } while (next_permutation(perm.begin(), perm.end()));

  return false;
}
