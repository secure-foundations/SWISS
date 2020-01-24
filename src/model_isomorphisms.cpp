#include "model_isomorphisms.h"

#include <algorithm>

using namespace std;

bool is_function_iso(shared_ptr<Model> m1, shared_ptr<Model> m2,
    shared_ptr<Module> module,
    VarDecl const& decl,
    vector<vector<int>> const& perms)
{
  vector<object_value> args;
  vector<int> idx_for_arg;
  vector<int> domain_sizes;
  for (lsort so : decl.sort->get_domain_as_function()) {
    domain_sizes.push_back(m1->get_domain_size(so));
    args.push_back(0);
    for (int i = 0; i < (int)module->sorts.size(); i++) {
      if (sorts_eq(so, s_uninterp(module->sorts[i]))) {
        idx_for_arg.push_back(i);
        goto end_of_loop;
      }
    }
    assert(false);
    end_of_loop: {}
  }

  int n = args.size();

  while (true) {
    vector<object_value> perm_args(n);
    for (int i = 0; i < n; i++) {
      perm_args[i] = perms[idx_for_arg[i]][args[i]];
    }

    object_value v1 = m1->func_eval(decl.name, args);
    object_value v2 = m1->func_eval(decl.name, perm_args);

    if (v1 != v2) {
      return false;
    }

    int i;
    for (i = 0; i < n; i++) {
      args[i]++;
      if ((int)args[i] == domain_sizes[i]) {
        args[i] = 0;
      } else {
        break;
      }
    }
    if (i == n) {
      break;
    }
  }

  return true;
}

bool do_test(shared_ptr<Model> m1, shared_ptr<Model> m2,
    shared_ptr<Module> module,
    vector<vector<int>> const& perms)
{
  if (module->sorts.size() == perms.size()) {
    for (VarDecl decl : module->functions) {
      if (!is_function_iso(m1, m2, module, decl, perms)) {
        return false;
      }
    }
    return true;
  } else {
    int idx = perms.size();
    string sort = module->sorts[idx];
    vector<int> perm;
    int n = m1->get_domain_size(sort);
    for (int i = 0; i < n; i++) {
      perm.push_back(i);
    }
    do {
      vector<vector<int>> new_perms = perms;
      new_perms.push_back(perm); 
      if (do_test(m1, m2, module, new_perms)) {
        return true;
      }
    } while (next_permutation(perm.begin(), perm.end()));
    return false;
  }
}


bool are_models_isomorphic(shared_ptr<Model> m1, shared_ptr<Model> m2)
{
  assert(m1->module.get() == m2->module.get());

  shared_ptr<Module> module = m1->module;

  for (string sort : module->sorts) {
    if (m1->get_domain_size(sort) != m2->get_domain_size(sort)) {
      return false;
    }
  }

  return do_test(m1, m2, module, {});
}
