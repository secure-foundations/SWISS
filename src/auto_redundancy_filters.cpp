#include "auto_redundancy_filters.h"

#include <cassert>
#include <iostream>

#include "top_quantifier_desc.h"

using namespace std;

bool has_subclause(value v, value sub)
{
  assert(v.get() != NULL);

  if (values_equal(v, sub)) {
    return true;
  }

  if (Forall* value = dynamic_cast<Forall*>(v.get())) {
    return has_subclause(value->body, sub);
  }
  else if (Exists* value = dynamic_cast<Exists*>(v.get())) {
    return has_subclause(value->body, sub);
  }
  else if (NearlyForall* value = dynamic_cast<NearlyForall*>(v.get())) {
    return has_subclause(value->body, sub);
  }
  else if (Var* value = dynamic_cast<Var*>(v.get())) {
    return false;
  }
  else if (Const* value = dynamic_cast<Const*>(v.get())) {
    return false;
  }
  else if (Eq* value = dynamic_cast<Eq*>(v.get())) {
    return has_subclause(value->left, sub)
        || has_subclause(value->right, sub);
  }
  else if (Not* value = dynamic_cast<Not*>(v.get())) {
    return has_subclause(value->val, sub);
  }
  else if (Implies* value = dynamic_cast<Implies*>(v.get())) {
    return has_subclause(value->left, sub)
        || has_subclause(value->right, sub);
  }
  else if (Apply* value = dynamic_cast<Apply*>(v.get())) {
    if (has_subclause(value->func, sub)) {
      return true;
    }
    for (shared_ptr<Value> arg : value->args) {
      if (has_subclause(arg, sub)) {
        return true;
      }
    }
    return false;
  }
  else if (And* value = dynamic_cast<And*>(v.get())) {
    for (shared_ptr<Value> arg : value->args) {
      if (has_subclause(arg, sub)) {
        return true;
      }
    }
    return false;
  }
  else if (Or* value = dynamic_cast<Or*>(v.get())) {
    for (shared_ptr<Value> arg : value->args) {
      if (has_subclause(arg, sub)) {
        return true;
      }
    }
    return false;
  }
  else if (IfThenElse* value = dynamic_cast<IfThenElse*>(v.get())) {
    return has_subclause(value->cond, sub)
        || has_subclause(value->then_value, sub)
        || has_subclause(value->else_value, sub);
  }
  else {
    //printf("has_subclause got: %s\n", v->to_string().c_str());
    assert(false && "has_subclause does not support this case");
  }
}

value get_later_not_var(value a, value b) {
  assert (!dynamic_cast<Var*>(a.get()));
  assert (!dynamic_cast<Var*>(b.get()));
  return lt_value(a, b) ? b : a;
}

vector<int> sort2(int a, int b) {
  assert (a != b);
  if (a < b) {
    return {a,b};
  } else {
    return {b,a};
  }
}

std::vector<std::vector<int>> get_auto_redundancy_filters(
    std::vector<value> const& _pieces)
{
  vector<value> pieces;
  for (value v : _pieces) {
    pieces.push_back(get_taqd_and_body(v).second);
  }

  vector<vector<int>> res;

  for (int i = 0; i < (int)pieces.size(); i++) {
    if (Not* n = dynamic_cast<Not*>(pieces[i].get())) {
      for (int j = 0; j < (int)pieces.size(); j++) {
        if (values_equal(n->val, pieces[j])) {
          res.push_back(sort2(i,j));
        }
      }
      if (Eq* e = dynamic_cast<Eq*>(n->val.get())) {
        // Note: these aren't tautological, but they are redundant,
        // since one expression can always be replaced with the other.
        value dis_allowed = get_later_not_var(e->left, e->right);
        for (int j = 0; j < (int)pieces.size(); j++) {
          if (i != j) {
            if (has_subclause(pieces[j], dis_allowed)) {
              res.push_back(sort2(i,j));
            }
          }
        }
      }
    }
  }

  /*cout << "get_auto_redundancy_filters" << endl;
  for (vector<int> vs : res) {
    for (int i : vs) {
      cout << pieces[i]->to_string() << " ";
    }
    cout << endl;
  }
  assert(false);*/

  return res;
}
