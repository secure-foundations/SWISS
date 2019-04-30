#include "sketch_model.h"

using namespace std;

void SketchModel::assert_formula(value v) {
  solver.add(to_z3(v, 1, {}));
}

z3::expr SketchModel::to_z3(value v, size_t res, std::map<iden, ValueVars> const& vars) {
  // TODO kill redundancy

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
    int n = get_domain_size(value->left->get_sort());
    if (res == 1) {
      z3::expr_vector eqs(ctx);
      for (int i = 0; i < n; i++) {
        z3::expr_vector sides(ctx);
        sides.push_back(to_z3(value->left, i, vars));
        sides.push_back(to_z3(value->right, i, vars));
        eqs.push_back(z3::mk_and(sides));
      }
      return z3::mk_or(eqs);
    } else {
      z3::expr_vector neqs(ctx);
      for (int i = 0; i < n; i++) {
        z3::expr_vector a_and_b(ctx);
        a_and_b.push_back(to_z3(value->left, i, vars));
        z3::expr_vector b(ctx);
        for (int j = 0; j < n; j++) {
          if (i != j) {
            b.push_back(to_z3(value->right, j, vars));
          }
        }
        a_and_b.push_back(z3::mk_or(b));
        neqs.push_back(z3::mk_and(a_and_b));
      }
      return z3::mk_or(neqs);
    }
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

ValueVars SketchModel::make_value_vars_const(lsort sort, size_t val)
{
  ValueVars vv;
  vv.sort = sort;
  vv.n = get_domain_size(sort);
  for (int i = 0; i < vv.n; i++) {
    vv.exprs.push_back(i == val ? ctx.bool_val(true) : ctx.bool_val(false));
  }
  return vv;
}

ValueVars SketchModel::make_value_vars_var(lsort sort, string const& name)
{
  ValueVars vv;
  vv.sort = sort;
  vv.n = get_domain_size(sort);
  for (int i = 0; i < vv.n; i++) {
    string na = ::name(name + "_eq_" + to_string(i));
    vv.names.push_back(na);
    vv.exprs.push_back(bool_const(na));
  }
  for (int i = 0; i < vv.n; i++) {
    for (int j = i+1; j < vv.n; j++) {
      z3::expr_vector vec(ctx);
      vec.push_back(vv.exprs[i]);
      vec.push_back(vv.exprs[j]);
      solver.add(!z3::mk_and(vec));
    }
  }
  return vv;
}

z3::expr SketchModel::bool_const(std::string const& name) {
  bool_count++;
  return ctx.bool_const(name.c_str());
}

z3::expr ValueVars::get(size_t i) const {
  assert(i < exprs.size());
  return exprs[i];
}

SketchModel::SketchModel(
    z3::context& ctx,
    z3::solver& solver,
    std::shared_ptr<Module> module,
    int n)
    : ctx(ctx), solver(solver), module(module), bool_count(0)
{
  for (string sort_name : module->sorts) {
    domain_sizes.insert(make_pair(sort_name, n));
  }

  for (VarDecl decl : module->functions) {
    SketchFunction sf;
    vector<lsort> domain = decl.sort->get_domain_as_function();
    lsort range = decl.sort->get_range_as_function();
    vector<size_t> args;
    for (lsort so : domain) {
      sf.domain_sizes.push_back(get_domain_size(so));
      args.push_back(0);
    }

    while (true) {
      string name = "sm_" + iden_to_string(decl.name);
      for (int i = 0; i < domain.size(); i++) {
        name += "_" + to_string(args[i]);
      }

      SketchFunctionEntry sfe;
      sfe.args = args;
      sfe.res = make_value_vars_var(range, name);
      sf.table.push_back(move(sfe));

      int i;
      for (i = 0; i < domain.size(); i++) {
        args[i]++;
        if (args[i] == sf.domain_sizes[i]) {
          args[i] = 0;
        } else {
          break;
        }
      }
      if (i == domain.size()) {
        break;
      }
    }

    functions.insert(make_pair(decl.name, sf));
  }

  for (value axiom : module->axioms) {
    assert_formula(axiom);
  }
}

shared_ptr<Model> SketchModel::to_model(z3::model& m)
{
  map<string, bool> bool_map = get_bool_map(m);

  std::unordered_map<std::string, SortInfo> sort_info;
  for (auto p : domain_sizes) {
    SortInfo si;
    si.domain_size = p.second;
    sort_info.insert(make_pair(p.first, si));
  }

  std::unordered_map<iden, FunctionInfo> function_info;
  for (auto& p : functions) {
    iden name = p.first;
    SketchFunction& sf = p.second;

    FunctionInfo finfo;
    finfo.else_value = 0; /* doesn't matter */
    finfo.table.reset(new FunctionTable());

    for (SketchFunctionEntry& sfe : sf.table) {
      FunctionTable* ft = finfo.table.get();
      for (int i = 0; i < sfe.args.size(); i++) {
        if (ft->children.size() == 0) {
          ft->children.resize(sf.domain_sizes[i]);
          for (int j = 0; j < sf.domain_sizes[i]; j++) {
            ft->children[j].reset(new FunctionTable());
          }
        }
        assert(0 <= sfe.args[i] && sfe.args[i] < ft->children.size());
        ft = ft->children[sfe.args[i]].get();
      }
      ft->value = get_value_from_z3(sfe.res, bool_map);
    }

    function_info.insert(make_pair(name, move(finfo)));
  }

  return shared_ptr<Model>(new Model(this->module, move(sort_info), move(function_info)));
}

map<string, bool> SketchModel::get_bool_map(z3::model model) {
  map<string, bool> bool_map;
  int n = model.num_consts();
  z3::expr e_true = ctx.bool_val(true);
  z3::expr e_false = ctx.bool_val(false);
  for (int i = 0; i < n; i++) {
    z3::func_decl fd = model.get_const_decl(i);
    z3::expr res = model.get_const_interp(fd);
    if (eq(res, e_true)) {
      bool_map.insert(make_pair(fd.name().str(), true));
    } else if (eq(res, e_false)) {
      bool_map.insert(make_pair(fd.name().str(), false));
    } else {
      assert(false);
    }
  }
  return bool_map;
}

object_value SketchModel::get_value_from_z3(
    ValueVars& vv,
    map<string, bool> const& bool_map)
{
  assert(vv.names.size() > 0);
  for (int i = 0; i < vv.names.size(); i++) {
    auto iter = bool_map.find(vv.names[i]);
    if (iter != bool_map.end() && iter->second) {
      return i;
    }
  }
  return 0;
}
