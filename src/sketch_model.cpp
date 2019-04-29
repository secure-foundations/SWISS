#include "sketch_model.h"

using namespace std;

void SketchModel::assert_formula(value v) {
  solver.add(to_z3(v, 1, {}));
}

z3::expr SketchModel::to_z3(value v, size_t res, std::map<iden, ValueVars> const& vars) {
  assert(v.get() != NULL);
  if (Forall* value = dynamic_cast<Forall*>(v.get())) {
    return to_z3_forall_exists(res, vars, true, value->decls, value->body);
  }
  else if (Exists* value = dynamic_cast<Exists*>(v.get())) {
    return to_z3_forall_exists(res, vars, false, value->decls, value->body);
  }
  else if (NearlyForall* value = dynamic_cast<NearlyForall*>(v.get())) {
    assert(false && "SketchModel::z3 NearlyForall case not implemented");
  }
  else if (Var* value = dynamic_cast<Var*>(v.get())) {
    auto iter = vars.find(value->name);
    assert(iter != vars.end());
    return iter->second.get(res);
  }
  else if (Const* value = dynamic_cast<Const*>(v.get())) {
    assert(false && "shouldn't get here?");
  }
  else if (Eq* value = dynamic_cast<Eq*>(v.get())) {
    //int n = get_domain_size(value->left->get_sort());
    assert(false);
  }
  else if (Not* value = dynamic_cast<Not*>(v.get())) {
    return to_z3(value->val, res == 1 ? 0 : 1, vars);
  }
  else if (Implies* value = dynamic_cast<Implies*>(v.get())) {
    if (res == 1) {
      z3::expr_vector vec(ctx);
      vec.push_back(to_z3(value->left, 0, vars));
      vec.push_back(to_z3(value->right, 1, vars));
      return z3::mk_or(vec);
    } else {
      z3::expr_vector vec(ctx);
      vec.push_back(to_z3(value->left, 1, vars));
      vec.push_back(to_z3(value->right, 0, vars));
      return z3::mk_and(vec);
    }
  }
  else if (Apply* value = dynamic_cast<Apply*>(v.get())) {
    Const* func = dynamic_cast<Const*>(value->func.get());
    iden func_name = func->name;
    auto iter = functions.find(func_name);
    assert(iter != functions.end());
    SketchFunction const& sf = iter->second;

    z3::expr_vector full_vec(ctx);
    for (SketchFunctionEntry const& fe : sf.table) {
      z3::expr_vector vec(ctx);
      vec.push_back(fe.res.get(res));
      for (int i = 0; i < fe.args.size(); i++) {
        vec.push_back(to_z3(value->args[i], fe.args[i], vars));
      }
      full_vec.push_back(z3::mk_and(vec));
    }
    return z3::mk_or(full_vec);
  }
  else if (And* value = dynamic_cast<And*>(v.get())) {
    z3::expr_vector vec(ctx);
    for (shared_ptr<Value> arg : value->args) {
      vec.push_back(to_z3(arg, res, vars));
    }
    return res == 1 ? z3::mk_and(vec) : z3::mk_or(vec);
  }
  else if (Or* value = dynamic_cast<Or*>(v.get())) {
    z3::expr_vector vec(ctx);
    for (shared_ptr<Value> arg : value->args) {
      vec.push_back(to_z3(arg, res, vars));
    }
    return res == 1 ? z3::mk_or(vec) : z3::mk_and(vec);
  }
  else {
    //printf("value2expr got: %s\n", v->to_string().c_str());
    assert(false && "SketchModel::to_z3 does not support this case");
  }
}

z3::expr SketchModel::to_z3_forall_exists(
  size_t res,
  map<iden, ValueVars> const& vars,
  bool is_forall,
  vector<VarDecl> const& decls,
  value body)
{
  int n = decls.size();

  vector<size_t> args;
  args.resize(n);

  vector<size_t> domain_sizes;
  for (int i = 0; i < n; i++) {
    domain_sizes.push_back(get_domain_size(decls[i].sort));
  }

  z3::expr_vector vec(ctx);

  while (true) {
    map<iden, ValueVars> new_vars = vars;
    for (int i = 0; i < n; i++) {
      new_vars.insert(make_pair(decls[i].name, make_value_vars_const(decls[i].sort, args[i])));
    }
    vec.push_back(to_z3(body, res, new_vars));

    int i;
    for (i = 0; i < n; i++) {
      args[i]++;
      if (args[i] == domain_sizes[i]) {
        args[i] = 0;
      } else {
        break;
      }
    }
    if (i == n) {
      break;
    }
  }

  return ((res == 1 && is_forall) || (res == 0 && !is_forall)) ?
      z3::mk_and(vec) : z3::mk_or(vec);
}

size_t SketchModel::get_domain_size(lsort sort)
{
  if (dynamic_cast<BooleanSort*>(sort.get())) {
    return 2;
  }
  else if (UninterpretedSort* usort = dynamic_cast<UninterpretedSort*>(sort.get())) {
    return get_domain_size(usort->name);
  }
  else {
    assert(false);
  }
}

size_t SketchModel::get_domain_size(std::string sort)
{
  auto iter = domain_sizes.find(sort);
  assert(iter != domain_sizes.end());
  return iter->second;
}
