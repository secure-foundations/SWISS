#include "clause_gen.h"

#include <cassert>

#include "utils.h"

using namespace std;

vector<value> gen_uninterp(int depth,
    shared_ptr<Module> module,
    vector<VarDecl> const& decls,
    std::string const& sort_name,
    bool var_only);

vector<value> gen_apps(int depth,
    shared_ptr<Module> module,
    vector<VarDecl> const& decls,
    VarDecl func)
{
  assert (depth < 10);

  bool is_btw = (iden_to_string(func.name) == "btw");

  vector<vector<value>> poss_args;
  vector<int> indices;
  vector<lsort> domains = func.sort->get_domain_as_function();
  for (int i = 0; i < (int)domains.size(); i++) {
    UninterpretedSort* us = dynamic_cast<UninterpretedSort*>(domains[i].get());
    assert (us != NULL);
    vector<value> vs = gen_uninterp(depth+1, module, decls, us->name, false);
    if (vs.size() == 0) {
      return {};
    }
    poss_args.push_back(vs);
    indices.push_back(0);
  }

  vector<value> res;

  while (true) {
    bool skip = false;
    if (is_btw) {
      assert(indices.size() == 3);
      if (indices[0] == indices[1]
          || indices[1] == indices[2]
          || indices[2] == indices[0]) {
        skip = true;
      }
    }

    if (!skip) {
      vector<value> args;
      for (int i = 0; i < (int)indices.size(); i++) {
        args.push_back(poss_args[i][indices[i]]);
      }
      if (args.size() == 0) {
        res.push_back(v_const(func.name, func.sort));
      } else {
        res.push_back(v_apply(v_const(func.name, func.sort), args));
      }
    }

    int i;
    for (i = 0; i < (int)indices.size(); i++) {
      indices[i]++;
      if (indices[i] == (int)poss_args[i].size()) {
        indices[i] = 0;
      } else {
        break;
      }
    }
    if (i == (int)indices.size()) {
      break;
    }
  }

  return res;
}

vector<value> gen_bool(int depth,
    shared_ptr<Module> module,
    vector<VarDecl> const& decls)
{
  assert (depth < 10);

  vector<value> res;

  for (VarDecl func : module->functions) {
    lsort so = func.sort->get_range_as_function();
    if (dynamic_cast<BooleanSort*>(so.get())) {
      vector<value> apps = gen_apps(depth+1, module, decls, func);
      for (value v : apps) {
        res.push_back(v);
        res.push_back(v_not(v));
      }
    }
  }

  for (string sort_name : module->sorts) {
    vector<value> equalands = gen_uninterp(depth+1, module, decls, sort_name, false /* var_only */);
    for (int i = 0; i < (int)equalands.size(); i++) {
      for (int j = 0; j < (int)equalands.size(); j++) {
        if (i != j && lt_value(equalands[i], equalands[j])) {
          res.push_back(v_eq(equalands[i], equalands[j]));
          res.push_back(v_not(v_eq(equalands[i], equalands[j])));
        }
      }
    }
  }

  return res;
}

vector<value> gen_uninterp(int depth,
    shared_ptr<Module> module,
    vector<VarDecl> const& decls,
    std::string const& sort_name,
    bool var_only)
{
  assert (depth < 10);

  vector<value> res;

  if (!var_only) {
    for (VarDecl func : module->functions) {
      lsort so = func.sort->get_range_as_function();
      if (UninterpretedSort* us = dynamic_cast<UninterpretedSort*>(so.get())) {
        if (us->name == sort_name) {
          vector<value> apps = gen_apps(depth+1, module, decls, func);
          for (value v : apps) {
            res.push_back(v);
          }
        }
      }
    }
  }

  for (VarDecl d : decls) {
    lsort so = d.sort;
    if (UninterpretedSort* us = dynamic_cast<UninterpretedSort*>(so.get())) {
      if (us->name == sort_name) {
        res.push_back(v_var(d.name, d.sort));
      }
    }
  }

  return res;
}

std::vector<value> gen_clauses(
    std::shared_ptr<Module> module,
    std::vector<VarDecl> const& decls)
{
  vector<value> res = gen_bool(0, module, decls);

  std::sort(res.begin(), res.end(), [](value a, value b) {
    return lt_value(a, b);
  });

  return res;
}
