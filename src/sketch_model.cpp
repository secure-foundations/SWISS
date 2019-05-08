#include "sketch_model.h"

using namespace std;

void SketchModel::assert_formula(value v) {
  solver.add(to_sat(v, 1, {}));
}

sat_expr SketchModel::to_sat(value v, size_t res, std::map<iden, ValueVars> const& vars) {
  value substv = v;
  for (auto p : vars) {
    ValueVars& vv = p.second;
    substv = substv->subst(p.first, v_var(
        string_to_iden("__c." +
            vv.sort->to_string() + "." + to_string(vv.constant_value)),
        vv.sort));
  }
  ComparableValue cmpval(substv);
  auto iter = value_to_expr_map.find(make_pair(cmpval, res));
  if (iter != value_to_expr_map.end()) {
    return iter->second;
  }

  sat_expr bc = bool_const(name("sme"));
  sat_expr result = bc; // initialize to whatever, doesn't matter

  //printf("doing %s\n", substv->to_string().c_str());

  assert(v.get() != NULL);
  if (Forall* value = dynamic_cast<Forall*>(v.get())) {
    result = to_sat_forall_exists(res, vars, true, value->decls, value->body);
  }
  else if (Exists* value = dynamic_cast<Exists*>(v.get())) {
    result = to_sat_forall_exists(res, vars, false, value->decls, value->body);
  }
  else if (NearlyForall* value = dynamic_cast<NearlyForall*>(v.get())) {
    assert(false && "SketchModel::to_sat NearlyForall case not implemented");
  }
  else if (Var* value = dynamic_cast<Var*>(v.get())) {
    auto iter = vars.find(value->name);
    assert(iter != vars.end());
    result = iter->second.get(res);
  }
  else if (Const* value = dynamic_cast<Const*>(v.get())) {
    assert(false && "shouldn't get here?");
  }
  else if (Eq* value = dynamic_cast<Eq*>(v.get())) {
    int n = get_domain_size(value->left->get_sort());
    if (res == 1) {
      vector<sat_expr> eqs;
      for (int i = 0; i < n; i++) {
        vector<sat_expr> sides;
        sides.push_back(to_sat(value->left, i, vars));
        sides.push_back(to_sat(value->right, i, vars));
        eqs.push_back(sat_and(sides));
      }
      result = sat_or(eqs);
    } else {
      vector<sat_expr> neqs;
      for (int i = 0; i < n; i++) {
        vector<sat_expr> a_and_b;
        a_and_b.push_back(to_sat(value->left, i, vars));
        vector<sat_expr> b;
        for (int j = 0; j < n; j++) {
          if (i != j) {
            b.push_back(to_sat(value->right, j, vars));
          }
        }
        a_and_b.push_back(sat_or(b));
        neqs.push_back(sat_and(a_and_b));
      }
      result = sat_or(neqs);
    }
  }
  else if (Not* value = dynamic_cast<Not*>(v.get())) {
    result = to_sat(value->val, res == 1 ? 0 : 1, vars);
  }
  else if (Implies* value = dynamic_cast<Implies*>(v.get())) {
    if (res == 1) {
      vector<sat_expr> vec;
      vec.push_back(to_sat(value->left, 0, vars));
      vec.push_back(to_sat(value->right, 1, vars));
      result = sat_or(vec);
    } else {
      vector<sat_expr> vec;
      vec.push_back(to_sat(value->left, 1, vars));
      vec.push_back(to_sat(value->right, 0, vars));
      result = sat_and(vec);
    }
  }
  else if (Apply* value = dynamic_cast<Apply*>(v.get())) {
    Const* func = dynamic_cast<Const*>(value->func.get());
    iden func_name = func->name;
    auto iter = functions.find(func_name);
    assert(iter != functions.end());
    SketchFunction const& sf = iter->second;

    vector<sat_expr> full_vec;
    for (SketchFunctionEntry const& fe : sf.table) {
      vector<sat_expr> vec;
      vec.push_back(fe.res.get(res));
      for (int i = 0; i < fe.args.size(); i++) {
        vec.push_back(to_sat(value->args[i], fe.args[i], vars));
      }
      full_vec.push_back(sat_and(vec));
    }
    result = sat_or(full_vec);
  }
  else if (And* value = dynamic_cast<And*>(v.get())) {
    vector<sat_expr> vec;
    for (shared_ptr<Value> arg : value->args) {
      vec.push_back(to_sat(arg, res, vars));
    }
    result = res == 1 ? sat_and(vec) : sat_or(vec);
  }
  else if (Or* value = dynamic_cast<Or*>(v.get())) {
    vector<sat_expr> vec;
    for (shared_ptr<Value> arg : value->args) {
      vec.push_back(to_sat(arg, res, vars));
    }
    result = res == 1 ? sat_or(vec) : sat_and(vec);
  }
  else {
    //printf("value2expr got: %s\n", v->to_string().c_str());
    assert(false && "SketchModel::to_sat does not support this case");
  }

  solver.add(sat_implies(bc, result));
  value_to_expr_map.insert(make_pair(make_pair(cmpval, res), bc));
  return bc;
}

sat_expr SketchModel::to_sat_forall_exists(
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

  vector<sat_expr> vec;

  while (true) {
    map<iden, ValueVars> new_vars = vars;
    for (int i = 0; i < n; i++) {
      new_vars.insert(make_pair(decls[i].name, make_value_vars_const(decls[i].sort, args[i])));
    }
    vec.push_back(to_sat(body, res, new_vars));

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
      sat_and(vec) : sat_or(vec);
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
    vv.exprs.push_back(i == val ? sat_true() : sat_false());
  }
  vv.constant_value = val;
  return vv;
}

ValueVars SketchModel::make_value_vars_var(lsort sort, string const& name)
{
  ValueVars vv;
  vv.sort = sort;
  vv.n = get_domain_size(sort);
  for (int i = 0; i < vv.n; i++) {
    string na = ::name(name + "_eq_" + to_string(i));
    vv.exprs.push_back(bool_const(na));
  }
  for (int i = 0; i < vv.n; i++) {
    for (int j = i+1; j < vv.n; j++) {
      vector<sat_expr> vec;
      vec.push_back(vv.exprs[i]);
      vec.push_back(vv.exprs[j]);
      solver.add(sat_not(sat_and(vec)));
    }
  }
  vv.constant_value = -1;
  return vv;
}

sat_expr SketchModel::bool_const(std::string const& name) {
  bool_count++;
  return solver.new_sat_var(name.c_str());
}

sat_expr ValueVars::get(size_t i) const {
  assert(i < exprs.size());
  return exprs[i];
}

SketchModel::SketchModel(
    SatSolver& solver,
    std::shared_ptr<Module> module,
    int n)
    : solver(solver), module(module), bool_count(0)
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

    if (iden_to_string(decl.name) == "le") {
      for (SketchFunctionEntry& sfe : sf.table) {
        assert(sfe.args.size() == 2);
        if (sfe.args[0] <= sfe.args[1]) {
          solver.add(sat_not(sfe.res.get(0)));
          solver.add(sfe.res.get(1));
        } else {
          solver.add(sfe.res.get(0));
          solver.add(sat_not(sfe.res.get(1)));
        }
      }
    }

    functions.insert(make_pair(decl.name, sf));
  }

  for (value axiom : module->axioms) {
    assert_formula(axiom);
  }
}

shared_ptr<Model> SketchModel::to_model()
{
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
      ft->value = get_value_from_model(sfe.res);
    }

    function_info.insert(make_pair(name, move(finfo)));
  }

  return shared_ptr<Model>(new Model(this->module, move(sort_info), move(function_info)));
}

object_value SketchModel::get_value_from_model(ValueVars& vv)
{
  for (int i = 0; i < vv.exprs.size(); i++) {
    if (solver.get(vv.exprs[i])) {
      return i;
    }
  }
  return 0;
}
