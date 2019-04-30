#include "model.h"

#include <cassert>
#include <map>
#include <algorithm>

using namespace std;
using namespace json11;

enum class EvalExprType {
  Forall,
  Exists,
  NearlyForall,
  Var,
  Const,
  Eq,
  Not,
  Implies,
  Apply,
  And,
  Or
};

struct EvalExpr {
  EvalExprType type;

  vector<EvalExpr> args;
  object_value const_value;
  int quantifier_domain_size;
  int var_index;
  FunctionInfo const * function_info;

  vector<int> nearlyforall_quantifier_domain_sizes;
  vector<int> nearlyforall_var_indices;
};

EvalExpr Model::value_to_eval_expr(
    shared_ptr<Value> v,
    vector<iden> const& names) const {
  assert(v.get() != NULL);

  EvalExpr ee;

  if (dynamic_cast<Forall*>(v.get()) || dynamic_cast<Exists*>(v.get()) || dynamic_cast<NearlyForall*>(v.get())) {
    std::vector<VarDecl> const * decls;
    std::shared_ptr<Value> body;

    if (Forall* value = dynamic_cast<Forall*>(v.get())) {
      decls = &value->decls;
      body = value->body;
    } else if (Exists* value = dynamic_cast<Exists*>(v.get())) {
      decls = &value->decls;
      body = value->body;
    } else if (NearlyForall* value = dynamic_cast<NearlyForall*>(v.get())) {
      decls = &value->decls;
      body = value->body;
    } else {
      assert(false);
    }

    vector<iden> new_names = names;
    for (VarDecl decl : *decls) {
      new_names.push_back(decl.name);
    }

    ee = value_to_eval_expr(body, new_names); 

    if (dynamic_cast<NearlyForall*>(v.get())) {
      EvalExpr ee2;
      ee2.type = EvalExprType::NearlyForall;
      for (int i = 0; i < decls->size(); i++) {
        VarDecl decl = (*decls)[i];
        ee2.nearlyforall_quantifier_domain_sizes.push_back(get_domain_size(decl.sort.get()));
        ee2.nearlyforall_var_indices.push_back(names.size() + i);
      }
      ee2.args.push_back(move(ee));
      ee = move(ee2);
    } else {
      for (int i = decls->size() - 1; i >= 0; i--) {
        VarDecl decl = (*decls)[i];
        EvalExpr ee2;
        ee2.type = dynamic_cast<Forall*>(v.get()) ? EvalExprType::Forall : EvalExprType::Exists;
        ee2.quantifier_domain_size = get_domain_size(decl.sort.get());
        ee2.var_index = names.size() + i;
        ee2.args.push_back(move(ee));
        ee = move(ee2);
      }
    }
  }
  else if (Var* value = dynamic_cast<Var*>(v.get())) {
    int idx = -1;
    for (int i = 0; i < names.size(); i++) {
      if (names[i] == value->name) {
        idx = i;
        break;
      }
    }
    if (idx == -1) {
      printf("could not find var: %s\n", iden_to_string(value->name).c_str());
      assert(false);
    }
    ee.type = EvalExprType::Var;
    ee.var_index = idx;
  }
  else if (Const* value = dynamic_cast<Const*>(v.get())) {
    auto iter = function_info.find(value->name);
    assert(iter != function_info.end());
    FunctionInfo const& finfo = iter->second;
    FunctionTable* ftable = finfo.table.get();
    int val = ftable == NULL ? finfo.else_value : ftable->value;

    ee.type = EvalExprType::Const;
    ee.const_value = val;
  }
  else if (Eq* value = dynamic_cast<Eq*>(v.get())) {
    ee.type = EvalExprType::Eq;
    ee.args.push_back(value_to_eval_expr(value->left, names));
    ee.args.push_back(value_to_eval_expr(value->right, names));
  }
  else if (Not* value = dynamic_cast<Not*>(v.get())) {
    ee.type = EvalExprType::Not;
    ee.args.push_back(value_to_eval_expr(value->val, names));
  }
  else if (Implies* value = dynamic_cast<Implies*>(v.get())) {
    ee.type = EvalExprType::Implies;
    ee.args.push_back(value_to_eval_expr(value->left, names));
    ee.args.push_back(value_to_eval_expr(value->right, names));
  }
  else if (Apply* value = dynamic_cast<Apply*>(v.get())) {
    Const* func = dynamic_cast<Const*>(value->func.get());
    auto iter = function_info.find(func->name);
    if (iter == function_info.end()) {
      printf("could not find function name %s\n", iden_to_string(func->name).c_str());
      assert(false);
    }
    FunctionInfo const& finfo = iter->second;

    ee.type = EvalExprType::Apply;
    ee.function_info = &finfo;

    for (auto arg : value->args) {
      ee.args.push_back(value_to_eval_expr(arg, names));
    }
  }
  else if (And* value = dynamic_cast<And*>(v.get())) {
    ee.type = EvalExprType::And;
    for (auto arg : value->args) {
      ee.args.push_back(value_to_eval_expr(arg, names));
    }
  }
  else if (Or* value = dynamic_cast<Or*>(v.get())) {
    ee.type = EvalExprType::Or;
    for (auto arg : value->args) {
      ee.args.push_back(value_to_eval_expr(arg, names));
    }
  }
  else {
    assert(false && "value2eval_expr does not support this case");
  }

  return ee;
}

int max_var(EvalExpr& ee) {
  int res = -1;
  for (EvalExpr& child : ee.args) {
    res = max(res, max_var(child));
  }
  if (ee.type == EvalExprType::Forall || ee.type == EvalExprType::Exists) {
    res = max(res, ee.var_index);
  }
  if (ee.type == EvalExprType::NearlyForall) {
    for (int idx : ee.nearlyforall_var_indices) {
      res = max(res, idx);
    }
  }
  return res;
}

object_value eval(EvalExpr const& ee, int* var_values) {
  switch (ee.type) {
    case EvalExprType::Forall: {
      int idx = ee.var_index;
      int q = ee.quantifier_domain_size;
      EvalExpr const& body = ee.args[0];
      for (int i = 0; i < q; i++) {
        var_values[idx] = i;
        if (!eval(body, var_values)) {
          return 0;
        }
      }
      return 1;
    }

    case EvalExprType::Exists: {
      int idx = ee.var_index;
      int q = ee.quantifier_domain_size;
      EvalExpr const& body = ee.args[0];
      for (int i = 0; i < q; i++) {
        var_values[idx] = i;
        if (eval(body, var_values)) {
          return 1;
        }
      }
      return 0;
    }

    case EvalExprType::NearlyForall: {
      int n = ee.nearlyforall_quantifier_domain_sizes.size();
      for (int i = 0; i < n; i++) {
        var_values[ee.nearlyforall_var_indices[i]] = 0;
      }
      int bad_count = 0;
      EvalExpr const& body = ee.args[0];
      while (true) {
        if (!eval(body, var_values)) {
          bad_count++;
          if (bad_count >= 2) {
            return 0;
          }
        }
        
        int i;
        for (i = 0; i < n; i++) {
          int idx = ee.nearlyforall_var_indices[i];
          int sz = ee.nearlyforall_quantifier_domain_sizes[i];
          var_values[idx]++;
          if (var_values[idx] == sz) {
            var_values[idx] = 0;
          } else {
            break;
          }
        }
        if (i == n) {
          break;
        }
      }

      return 1;
    }

    case EvalExprType::Var:
      return var_values[ee.var_index];

    case EvalExprType::Const:
      return ee.const_value;

    case EvalExprType::Eq:
      return eval(ee.args[0], var_values) ==
             eval(ee.args[1], var_values);

    case EvalExprType::Not:
      return 1 - eval(ee.args[0], var_values);

    case EvalExprType::Implies:
      return (int)(!eval(ee.args[0], var_values) || (bool)eval(ee.args[1], var_values));

    case EvalExprType::Apply: {
      FunctionTable* ftable = ee.function_info->table.get();
      for (EvalExpr const& arg : ee.args) {
        if (ftable == NULL) {
          break;
        }
        object_value arg_res = eval(arg, var_values);
        ftable = ftable->children[arg_res].get();
      }
      return ftable == NULL ? ee.function_info->else_value : ftable->value;
    }

    case EvalExprType::And:
      for (EvalExpr const& arg : ee.args) {
        if (!eval(arg, var_values)) {
          return 0;
        }
      }
      return 1;

    case EvalExprType::Or:
      for (EvalExpr const& arg : ee.args) {
        if (eval(arg, var_values)) {
          return 1;
        }
      }
      return 0;
  }
}

bool Model::eval_predicate(shared_ptr<Value> value) const {
  EvalExpr ee = value_to_eval_expr(value, {});

  int n_vars = max_var(ee) + 1;
  int* var_values = new int[n_vars];

  int ans = eval(ee, var_values);

  delete[] var_values;

  return ans == 1;
}

int get_num_forall_quantifiers_at_top(EvalExpr* ee);

bool eval_get_counterexample(
    EvalExpr const& ee,
    int* var_values,
    QuantifierInstantiation& qi)
{
  if (ee.type != EvalExprType::Forall) {
    return eval(ee, var_values) == 1;
  }

  int idx = ee.var_index;
  int q = ee.quantifier_domain_size;
  EvalExpr const& body = ee.args[0];
  for (int i = 0; i < q; i++) {
    var_values[idx] = i;
    if (!eval_get_counterexample(body, var_values, qi)) {
      qi.variable_values[idx] = i;
      return false;
    }
  }
  return true;
}

QuantifierInstantiation get_counterexample(shared_ptr<Model> model, value v) {
  EvalExpr ee = model->value_to_eval_expr(v, {});
  int n = get_num_forall_quantifiers_at_top(&ee);

  QuantifierInstantiation qi;
  qi.formula = v;
  qi.variable_values.resize(n);
  qi.model = model;

  value w = v;
  int idx = 0;
  for (int i = 0; i < n; i++) {
    Forall* f = dynamic_cast<Forall*>(w.get());
    assert(f != NULL);
    qi.decls.push_back(f->decls[idx]);
    idx++;
    if (idx == f->decls.size()) {
      idx = 0;
      w = f->body;
    }
  }

  int n_vars = max(max_var(ee), n) + 1;
  int* var_values = new int[n_vars];

  bool ans = eval_get_counterexample(ee, var_values, qi);

  delete[] var_values;

  if (ans) {
    qi.non_null = false;
    qi.variable_values.clear();
  } else {
    qi.non_null = true;
  }

  return qi;
}

int get_num_forall_or_nearlyforall_quantifiers_at_top(EvalExpr* ee) {
  int n = 0;
  while (true) {
    if (ee->type == EvalExprType::Forall) {
      n++;
      ee = &ee->args[0];
    } else if (ee->type == EvalExprType::NearlyForall) {
      n += ee->nearlyforall_var_indices.size();
      ee = &ee->args[0];
    } else {
      break;
    }
  }
  return n;
}

template <typename A>
void append_vector(vector<A>& a, vector<A> const& b) {
  for (A const& x : b) {
    a.push_back(x);
  }
}

bool eval_get_multiqi_counterexample(
    EvalExpr const& ee,
    int* var_values,
    vector<vector<object_value>>& res, int total_len)
{
  if (ee.type == EvalExprType::Forall) {
    int idx = ee.var_index;
    int q = ee.quantifier_domain_size;
    EvalExpr const& body = ee.args[0];
    for (int i = 0; i < q; i++) {
      var_values[idx] = i;
      if (!eval_get_multiqi_counterexample(body, var_values, res, total_len)) {
        for (vector<object_value>& qi : res) {
          qi[idx] = i;
        }
        return false;
      }
    }
    return true;
  }
  else if (ee.type == EvalExprType::NearlyForall) {
    int len = ee.nearlyforall_quantifier_domain_sizes.size();
    for (int i = 0; i < len; i++) {
      var_values[ee.nearlyforall_var_indices[i]] = 0;
    }
    int bad_count = 0;
    EvalExpr const& body = ee.args[0];
    while (true) {
      vector<vector<object_value>> r;
      if (!eval_get_multiqi_counterexample(body, var_values, r, total_len)) {
        for (vector<object_value>& qi : r) {
          for (int i = 0; i < len; i++) {
            qi[ee.nearlyforall_var_indices[i]] =
                var_values[ee.nearlyforall_var_indices[i]];
          }
        }

        if (bad_count == 0) {
          res = move(r);
          bad_count++;
        } else {
          append_vector(res, r);
          return false;
        }
      }
      
      int i;
      for (i = 0; i < len; i++) {
        int idx = ee.nearlyforall_var_indices[i];
        int sz = ee.nearlyforall_quantifier_domain_sizes[i];
        var_values[idx]++;
        if (var_values[idx] == sz) {
          var_values[idx] = 0;
        } else {
          break;
        }
      }
      if (i == len) {
        break;
      }
    }

    res.clear();
    return true;
  } else {
    if (eval(ee, var_values) == 1) {
      return true;
    } else {
      vector<object_value> os;
      os.resize(total_len);
      res.push_back(os);
      return false;
    }
  }
}


bool get_multiqi_counterexample(shared_ptr<Model> model, value v,
    vector<QuantifierInstantiation>& result) {
  EvalExpr ee = model->value_to_eval_expr(v, {});
  int n = get_num_forall_or_nearlyforall_quantifiers_at_top(&ee);

  QuantifierInstantiation qi;
  qi.formula = v;
  qi.model = model;
  qi.non_null = true;

  value w = v;
  int idx = 0;
  for (int i = 0; i < n; i++) {
    int sz;
    value bd;
    if (Forall* f = dynamic_cast<Forall*>(w.get())) {
      qi.decls.push_back(f->decls[idx]);
      sz = f->decls.size();
      bd = f->body;
    }
    else if (NearlyForall* f = dynamic_cast<NearlyForall*>(w.get())) {
      qi.decls.push_back(f->decls[idx]);
      sz = f->decls.size();
      bd = f->body;
    }
    else {
      assert(false);
    }
    idx++;
    if (idx == sz) {
      idx = 0;
      w = bd;
    }
  }

  int n_vars = max(max_var(ee), n) + 1;
  int* var_values = new int[n_vars];

  vector<vector<object_value>> qis;
  bool ans = eval_get_multiqi_counterexample(ee, var_values, qis, n);

  delete[] var_values;

  //cout << "get_multiqi_counterexample:" << endl;
  //model->dump();
  //cout << "expr: " << v->to_string() << endl;

  result.clear();
  if (ans) {
    //cout << "result: no counterexample found" << endl;
    return true;
  } else {
    qi.non_null = true;
    //cout << "result:" << endl;
    for (vector<object_value> const& q : qis) {
      //cout << "vals:";
      //for (object_value ov : q) {
      //  cout << " " << ov;
      //}
      //cout << endl;

      qi.variable_values = q;
      result.push_back(qi);
    }
    return false;
  }
}

int get_num_forall_quantifiers_at_top(EvalExpr* ee) {
  int n = 0;
  while (true) {
    if (ee->type == EvalExprType::Forall) {
      n++;
      ee = &ee->args[0];
    } else {
      break;
    }
  }
  return n;
}

vector<shared_ptr<Model>> Model::extract_minimal_models_from_z3(
    z3::context& ctx,
    z3::solver& solver,
    shared_ptr<Module> module,
    vector<shared_ptr<ModelEmbedding>> es)
{
  assert (es.size() > 0);

  vector<shared_ptr<Model>> all_models;
  for (auto e : es) {
    all_models.push_back(extract_model_from_z3(ctx, solver, module, *e));
  }

  shared_ptr<Model> model0 = all_models[0];

  BackgroundContext& bgctx = *es[0]->ctx;

  vector<string> sorts;
  vector<int> sizes;
  for (auto p : bgctx.sorts) {
    sorts.push_back(p.first);
    sizes.push_back(model0->get_domain_size(p.first));
  }

  int sort_idx = 0;
  while (sort_idx < sorts.size()) {
    if (sizes[sort_idx] <= 1) {
      sort_idx++;
      continue;
    }

    solver.push();
    sizes[sort_idx]--;

    for (int i = 0; i < sorts.size(); i++) {
      z3::sort so = bgctx.getUninterpretedSort(sorts[i]);
      z3::expr_vector vec(ctx);
      z3::expr elem = ctx.constant(name("valvar").c_str(), so);
      for (int j = 0; j < sizes[i]; j++) {
        z3::expr c = ctx.constant(name("val").c_str(), so);
        vec.push_back(elem == c);
      }
      z3::expr_vector qvars(ctx);
      qvars.push_back(elem);
      solver.add(z3::forall(qvars, mk_or(vec)));
    }

    z3::check_result res = solver.check();
    assert(res == z3::sat || res == z3::unsat);
    if (res == z3::sat) {
      all_models.clear();
      for (auto e : es) {
        all_models.push_back(extract_model_from_z3(ctx, solver, module, *e));
      }
    } else {
      sizes[sort_idx]++;
      sort_idx++;
    }

    solver.pop();
  }

  return all_models;
}

shared_ptr<Model> Model::extract_model_from_z3(
    z3::context& ctx,
    z3::solver& solver,
    std::shared_ptr<Module> module,
    ModelEmbedding const& e)
{
  std::unordered_map<std::string, SortInfo> sort_info;
  std::unordered_map<iden, FunctionInfo> function_info;

  z3::model z3model = solver.get_model();

  map<string, z3::expr_vector> universes;

  for (auto p : e.ctx->sorts) {
    string name = p.first;
    z3::sort s = p.second;

    // The C++ api doesn't seem to have the functionality we need.
    // Go down to the C API.
    Z3_ast_vector c_univ = Z3_model_get_sort_universe(ctx, z3model, s);
    z3::expr_vector univ(ctx, c_univ);

    universes.insert(make_pair(name, univ));

    int len = univ.size();

    SortInfo sinfo;
    sinfo.domain_size = len;
    sort_info[name] = sinfo;
  }

  auto get_value = [&z3model, &ctx, &universes](
        Sort* sort, z3::expr expression1) -> object_value {
    z3::expr expression = z3model.eval(expression1, true);
    if (dynamic_cast<BooleanSort*>(sort)) {
      if (z3::eq(expression, ctx.bool_val(true))) {
        return 1;
      } else if (z3::eq(expression, ctx.bool_val(false))) {
        return 0;
      } else {
        assert(false);
      }
    } else if (UninterpretedSort* usort = dynamic_cast<UninterpretedSort*>(sort)) {
      auto iter = universes.find(usort->name);
      assert(iter != universes.end());
      z3::expr_vector& vec = iter->second;
      for (object_value i = 0; i < vec.size(); i++) {
        if (z3::eq(expression, vec[i])) {
          return i;
        }
      }
      assert(false);
    } else {
      assert(false && "expected boolean sort or uninterpreted sort");
    }
  };

  auto get_expr = [&ctx, &universes](
        Sort* sort, object_value v) -> z3::expr {
    if (dynamic_cast<BooleanSort*>(sort)) {
      if (v == 0) {
        return ctx.bool_val(false);
      } else if (v == 1) {
        return ctx.bool_val(true);
      } else {
        assert(false);
      }
    } else if (UninterpretedSort* usort = dynamic_cast<UninterpretedSort*>(sort)) {
      auto iter = universes.find(usort->name);
      assert(iter != universes.end());
      z3::expr_vector& vec = iter->second;
      assert (0 <= v && v < vec.size());
      return vec[v];
    } else {
      assert(false && "expected boolean sort or uninterpreted sort");
    }
  };

  for (VarDecl decl : module->functions) {
    iden name = decl.name;
    z3::func_decl fdecl = e.getFunc(name);

    int num_args;
    Sort* range_sort;
    vector<Sort*> domain_sorts;
    vector<int> domain_sort_sizes;
    if (FunctionSort* functionSort = dynamic_cast<FunctionSort*>(decl.sort.get())) {
      num_args = functionSort->domain.size();
      range_sort = functionSort->range.get();
      for (auto ptr : functionSort->domain) {
        Sort* argsort = ptr.get();
        domain_sorts.push_back(argsort);
        size_t sz;
        if (dynamic_cast<BooleanSort*>(argsort)) {
          sz = 2;
        } else if (UninterpretedSort* usort = dynamic_cast<UninterpretedSort*>(argsort)) {
          sz = sort_info[usort->name].domain_size;
        } else {
          assert(false && "expected boolean sort or uninterpreted sort");
        }
        domain_sort_sizes.push_back(sz);
      }
    } else {
      num_args = 0;
      range_sort = decl.sort.get();
    }

    function_info.insert(make_pair(name, FunctionInfo()));
    FunctionInfo& finfo = function_info[name];

    if (z3model.has_interp(fdecl)) {
      if (fdecl.is_const()) {
        z3::expr e = z3model.get_const_interp(fdecl);
        finfo.else_value = 0;
        finfo.table.reset(new FunctionTable());
        finfo.table->value = get_value(range_sort, e);
      } else {
        z3::func_interp finterp = z3model.get_func_interp(fdecl);

        vector<object_value> args;
        for (int i = 0; i < num_args; i++) {
          args.push_back(0);
        }
        while (true) {
          z3::expr_vector args_exprs(ctx);
          unique_ptr<FunctionTable>* table = &finfo.table;
          for (int argnum = 0; argnum < num_args; argnum++) {
            object_value argvalue = args[argnum];
            args_exprs.push_back(get_expr(domain_sorts[argnum], argvalue));
            if (!table->get()) {
              table->reset(new FunctionTable());
              (*table)->children.resize(domain_sort_sizes[argnum]);
            }
            assert(0 <= argvalue && argvalue < domain_sort_sizes[argnum]);
            table = &(*table)->children[argvalue];
          }
          object_value result_value =
              get_value(range_sort, finterp.else_value().substitute(args_exprs));

          assert (table != NULL);
          if (!table->get()) {
            table->reset(new FunctionTable());
          }
          (*table)->value = result_value;

          int i;
          for (i = num_args - 1; i >= 0; i--) {
            args[i]++;
            if (args[i] == domain_sort_sizes[i]) {
              args[i] = 0;
            } else {
              break;
            }
          }
          if (i == -1) {
            break;
          }
        }

        for (size_t i = 0; i < finterp.num_entries(); i++) {
          z3::func_entry fentry = finterp.entry(i);
          
          unique_ptr<FunctionTable>* table = &finfo.table;
          for (int argnum = 0; argnum < num_args; argnum++) {
            object_value argvalue = get_value(domain_sorts[argnum], fentry.arg(argnum));

            if (!table->get()) {
              table->reset(new FunctionTable());
              (*table)->children.resize(domain_sort_sizes[argnum]);
            }
            assert(0 <= argvalue && argvalue < domain_sort_sizes[argnum]);
            table = &(*table)->children[argvalue];
          }

          (*table)->value = get_value(range_sort, fentry.value());
        }
      }
    } else {
      finfo.else_value = 0;
    }
  }

  return shared_ptr<Model>(new Model(module, move(sort_info), move(function_info)));
}

void Model::dump_sizes() const {
  printf("Model sizes: ");
  bool start = true;
  for (string sort : module->sorts) {
    int domain_size = (int) get_domain_size(sort);
    if (!start) {
      printf(", ");
    }
    printf("%s: %d", sort.c_str(), domain_size);
    start = false;
  }
  printf("\n");
}

void Model::dump() const {
  printf("Model:\n\n");
  for (string sort : module->sorts) {
    int domain_size = (int) get_domain_size(sort);
    printf("%s: %d\n", sort.c_str(), domain_size);
  }
  printf("\n");
  for (VarDecl decl : module->functions) {
    iden name = decl.name;
    FunctionInfo const& finfo = get_function_info(name);

    size_t num_args;
    Sort* range_sort;
    vector<Sort*> domain_sorts;
    if (FunctionSort* functionSort = dynamic_cast<FunctionSort*>(decl.sort.get())) {
      num_args = functionSort->domain.size();
      range_sort = functionSort->range.get();
      for (auto ptr : functionSort->domain) {
        Sort* argsort = ptr.get();
        domain_sorts.push_back(argsort);
      }
    } else {
      num_args = 0;
      range_sort = decl.sort.get();
    }

    printf("%s(default) -> %s\n", iden_to_string(name).c_str(),
        obj_to_string(range_sort, finfo.else_value).c_str());
    
    vector<object_value> args;
    for (int i = 0; i < num_args; i++) {
      args.push_back(0);
    }
    while (true) {
      FunctionTable* ftable = finfo.table.get();
      for (int i = 0; i < num_args; i++) {
        if (ftable == NULL) break;
        ftable = ftable->children[args[i]].get();
      }
      if (ftable != NULL) {
        object_value res = ftable->value;
        printf("%s(", iden_to_string(name).c_str());
        for (int i = 0; i < num_args; i++) {
          if (i > 0) {
            printf(", ");
          }
          printf("%s", obj_to_string(domain_sorts[i], args[i]).c_str());
        }
        printf(") -> %s\n", obj_to_string(range_sort, res).c_str());
      }

      int i;
      for (i = num_args - 1; i >= 0; i--) {
        args[i]++;
        if (args[i] == get_domain_size(domain_sorts[i])) {
          args[i] = 0;
        } else {
          break;
        }
      }
      if (i == -1) {
        break;
      }
    }
    printf("\n");
  }
}

string Model::obj_to_string(Sort* sort, object_value ov) const {
  if (dynamic_cast<BooleanSort*>(sort)) {
    if (ov == 0) {
      return "false";
    }
    if (ov == 1) {
      return "true";
    }
    assert(false);
  } else if (UninterpretedSort* usort = dynamic_cast<UninterpretedSort*>(sort)) {
    auto iter = sort_info.find(usort->name);
    assert(iter != sort_info.end());
    SortInfo sinfo = iter->second;
    assert(0 <= ov && ov < sinfo.domain_size);

    return usort->name + ":" + to_string(ov + 1);
  } else {
    assert(false && "expected boolean sort or uninterpreted sort");
  }   
}

size_t Model::get_domain_size(Sort* s) const {
  if (dynamic_cast<BooleanSort*>(s)) {
    return 2;
  } else if (UninterpretedSort* usort = dynamic_cast<UninterpretedSort*>(s)) {
    return get_domain_size(usort->name);
  } else {
    assert(false && "expected boolean sort or uninterpreted sort");
  }
}

size_t Model::get_domain_size(std::string name) const {
  auto iter = sort_info.find(name);
  assert(iter != sort_info.end());
  return iter->second.domain_size;
}

size_t Model::get_domain_size(lsort s) const {
  return this->get_domain_size(s.get());
}


FunctionInfo const& Model::get_function_info(iden name) const {
  auto iter = function_info.find(name);
  assert(iter != function_info.end());
  return iter->second;
}

void Model::assert_model_is(shared_ptr<ModelEmbedding> e) {
  assert_model_is_or_isnt(e, true, false);
}

void Model::assert_model_is_not(shared_ptr<ModelEmbedding> e) {
  assert_model_is_or_isnt(e, true, true);
}

void Model::assert_model_does_not_have_substructure(shared_ptr<ModelEmbedding> e) {
  assert_model_is_or_isnt(e, false, true);
}

void Model::assert_model_is_or_isnt(shared_ptr<ModelEmbedding> e,
    bool exact, bool negate) {
  BackgroundContext& bgctx = *e->ctx;
  z3::solver& solver = bgctx.solver;

  unordered_map<string, z3::expr_vector> consts;

  z3::expr_vector assertions(bgctx.ctx);

  for (auto p : this->sort_info) {
    string sort_name = p.first;
    SortInfo sinfo = p.second;
    z3::sort so = bgctx.getUninterpretedSort(sort_name);

    z3::expr_vector vec(bgctx.ctx);
    for (int i = 0; i < sinfo.domain_size; i++) {
      vec.push_back(bgctx.ctx.constant(name(sort_name + "_val").c_str(), so));
    }
    for (int i = 0; i < vec.size(); i++) {
      for (int j = i+1; j < vec.size(); j++) {
        assertions.push_back(vec[i] != vec[j]);
      }
    }

    if (exact) {
      z3::expr elem = bgctx.ctx.constant(name(sort_name).c_str(), so);
      z3::expr_vector eqs(bgctx.ctx);
      for (int i = 0; i < vec.size(); i++) {
        eqs.push_back(vec[i] == elem);
      }
      z3::expr_vector qvars(bgctx.ctx);
      qvars.push_back(elem);
      assertions.push_back(z3::forall(qvars, mk_or(eqs)));
    }

    consts.insert(make_pair(sort_name, vec));
  }

  auto mkExpr = [&bgctx, &consts](Sort* so, object_value val) {
    if (dynamic_cast<BooleanSort*>(so)) {
      return bgctx.ctx.bool_val((bool)val);
    } else if (UninterpretedSort* usort = dynamic_cast<UninterpretedSort*>(so)) {
      auto iter = consts.find(usort->name);
      assert(iter != consts.end());
      assert(0 <= val && val < iter->second.size());
      return iter->second[val];
    } else {
      assert(false);
    }
  };

  for (VarDecl decl : module->functions) {
    iden name = decl.name;
    FunctionInfo const& finfo = get_function_info(name);

    size_t num_args;
    Sort* range_sort;
    vector<Sort*> domain_sorts;
    if (FunctionSort* functionSort = dynamic_cast<FunctionSort*>(decl.sort.get())) {
      num_args = functionSort->domain.size();
      range_sort = functionSort->range.get();
      for (auto ptr : functionSort->domain) {
        Sort* argsort = ptr.get();
        domain_sorts.push_back(argsort);
      }
    } else {
      num_args = 0;
      range_sort = decl.sort.get();
    }

    vector<object_value> args;
    for (int i = 0; i < num_args; i++) {
      args.push_back(0);
    }
    while (true) {
      object_value res = finfo.else_value;
      FunctionTable* ftable = finfo.table.get();
      for (int i = 0; i < num_args; i++) {
        if (ftable == NULL) break;
        ftable = ftable->children[args[i]].get();
      }
      if (ftable != NULL) {
        res = ftable->value;
      }

      z3::expr_vector z3_args(bgctx.ctx);
      for (int i = 0; i < domain_sorts.size(); i++) {
        z3_args.push_back(mkExpr(domain_sorts[i], args[i]));
      }
      assertions.push_back(e->getFunc(name)(z3_args) == mkExpr(range_sort, res));

      int i;
      for (i = num_args - 1; i >= 0; i--) {
        args[i]++;
        if (args[i] == get_domain_size(domain_sorts[i])) {
          args[i] = 0;
        } else {
          break;
        }
      }
      if (i == -1) {
        break;
      }
    }
  }

  if (negate) {
    solver.add(!z3::mk_and(assertions));
  } else {
    solver.add(z3::mk_and(assertions));
  }

  //printf("'%s'\n", solver.to_smt2().c_str());
}

shared_ptr<Model> transition_model(
    z3::context& ctx,
    shared_ptr<Module> module,
    std::shared_ptr<Model> start_state,
    int which_action
) {
  shared_ptr<BackgroundContext> bgctx = make_shared<BackgroundContext>(ctx, module);
  z3::solver& solver = bgctx->solver;

  shared_ptr<ModelEmbedding> e1 = ModelEmbedding::makeEmbedding(bgctx, module);
  shared_ptr<Action> action;
  if (which_action == -1) {
    // allow any action
    action.reset(new ChoiceAction(module->actions));
  } else {
    assert(0 <= which_action && which_action < module->actions.size());
    action = module->actions[which_action];
  }
  ActionResult res = applyAction(e1, action, {});
  shared_ptr<ModelEmbedding> e2 = res.e;
  // Add the relation between the two states
  solver.add(res.constraint);
  // Add the axioms
  for (shared_ptr<Value> axiom : module->axioms) {
    solver.add(e1->value2expr(axiom, {}));
  }

  start_state->assert_model_is(e1);
  z3::check_result sat_result = solver.check();
  if (sat_result == z3::sat) {
    return Model::extract_model_from_z3(ctx, solver, module, *e2);
  } else {
    return nullptr;
  }
}

void get_tree_of_models_(
  z3::context& ctx,
  shared_ptr<Module> module,
  std::shared_ptr<Model> start_state,
  int depth,
  vector<shared_ptr<Model>>& res
) {
  res.push_back(start_state);
  if (depth == 0) {
    return;
  }

  for (int i = 0; i < module->actions.size(); i++) {
    shared_ptr<Model> next_state = transition_model(ctx, module, start_state, i);
    if (next_state != nullptr) {
      get_tree_of_models_(ctx, module, next_state, depth - 1, res);
    }
  }
}

vector<shared_ptr<Model>> get_tree_of_models(
  z3::context& ctx,
  shared_ptr<Module> module,
  std::shared_ptr<Model> start_state,
  int depth
) {
  vector<shared_ptr<Model>> res;
  get_tree_of_models_(ctx, module, start_state, depth, res);
  return res;
}

void get_tree_of_models2_(
  z3::context& z3ctx,
  shared_ptr<Module> module,
  vector<int> action_indices,
  int depth,
  int multiplicity,
  bool reversed, // find bad models starting at NOT(safety condition)
  vector<shared_ptr<Model>>& res
) {
  shared_ptr<BackgroundContext> ctx = make_shared<BackgroundContext>(z3ctx, module);
  z3::solver& solver = ctx->solver;

  shared_ptr<ModelEmbedding> e1 = ModelEmbedding::makeEmbedding(ctx, module);

  shared_ptr<ModelEmbedding> e2 = e1;

  vector<int> action_indices_ordered = action_indices;
  if (reversed) {
    reverse(action_indices_ordered.begin(), action_indices_ordered.end());
  }

  for (int action_index : action_indices_ordered) {
    ActionResult res = applyAction(e2, module->actions[action_index], {});
    e2 = res.e;
    ctx->solver.add(res.constraint);
  }

  // Add the axioms
  for (shared_ptr<Value> axiom : module->axioms) {
    ctx->solver.add(e1->value2expr(axiom, {}));
  }

  if (!reversed) {
    // Add initial conditions
    for (shared_ptr<Value> init : module->inits) {
      ctx->solver.add(e1->value2expr(init));
    }
  } else {
    // Add the opposite of the safety condition
    ctx->solver.add(e2->value2expr(v_not(v_and(module->conjectures))));
  }

  bool found_any = false;

  for (int j = 0; j < multiplicity; j++) {
    z3::check_result sat_result = solver.check();
    if (sat_result != z3::sat) {
      break;
    } else {
      auto model = Model::extract_model_from_z3(z3ctx, solver, module, reversed ? *e1 : *e2);
      res.push_back(model);
      found_any = true;

      model->assert_model_is_not(reversed ? e1 : e2);
    }
  }

  // recurse
  if (found_any) {
    if (action_indices.size() < depth) { 
      action_indices.push_back(0);
      for (int i = 0; i < module->actions.size(); i++) {
        action_indices[action_indices.size() - 1] = i;
        get_tree_of_models2_(z3ctx, module, action_indices, depth, multiplicity, reversed, res);
      }
    }
  }
}

vector<shared_ptr<Model>> get_tree_of_models2(
  z3::context& ctx,
  shared_ptr<Module> module,
  int depth,
  int multiplicity,
  bool reversed // find bad models instead of good ones (starting at NOT(safety condition))
) {
  vector<shared_ptr<Model>> res;
  get_tree_of_models2_(ctx, module, {}, depth, multiplicity, reversed, res);
  return res;
}

/*
Z3VarSet add_existential_constraint(
    shared_ptr<ModelEmbedding> me,
    value v)
{
  shared_ptr<BackgroundContext> bgctx = me->ctx;
  z3::context& ctx = bgctx->ctx;
  z3::solver& solver = bgctx->solver;

  // Change NOT(forall ...) into a (exists ...)
  if (Not* n = dynamic_cast<Not*>(v.get())) {
    v = n->val->negate();
  }

  Z3VarSet res;
  unordered_map<iden, z3::expr> vars;

  Exists* exists;
  while ((exists = dynamic_cast<Exists*>(v.get())) != NULL) {
    for (VarDecl decl : exists->decls) {
      z3::func_decl fd = ctx.function(name(decl.name).c_str(), 0, 0, bgctx->getSort(decl.sort));
      z3::expr e = fd();
      res.vars.push_back(e);
      vars.insert(make_pair(decl.name, e));
    }

    v = exists->body;
  }

  solver.add(me->value2expr_with_vars(v, vars));

  return res;
}

QuantifierInstantiation z3_var_set_2_quantifier_instantiation(
    Z3VarSet const&,
    z3::solver&,
    std::shared_ptr<Model>,
    value v)
{
  QuantifierInstantiation qi;
  qi.non_null = true;
  qi.formula = v;
  qi.
}
*/

bool eval_qi(QuantifierInstantiation const& qi, value v)
{
  assert(qi.non_null);
  vector<iden> names;
  for (VarDecl const& decl : qi.decls) {
    names.push_back(decl.name);
  }
  EvalExpr ee = qi.model->value_to_eval_expr(v, names);

  int n_vars = max(max_var(ee), (int)qi.variable_values.size()) + 1;
  int* var_values = new int[n_vars];

  for (int i = 0; i < qi.variable_values.size(); i++) {
    var_values[i] = qi.variable_values[i];
  }

  int ans = eval(ee, var_values);

  delete[] var_values;

  return ans == 1;
}

vector<size_t> Model::get_domain_sizes_for_function(iden name) const {
  lsort sort;
  for (VarDecl decl : module->functions) {
    if (decl.name == name) {
      sort = decl.sort;
      break;
    }
  }
  assert(sort != nullptr);

  vector<size_t> domain_sizes;
  if (FunctionSort* functionSort = dynamic_cast<FunctionSort*>(sort.get())) {
    for (auto so : functionSort->domain) {
      domain_sizes.push_back(get_domain_size(so));
    }
  }

  return domain_sizes;
}

std::vector<FunctionEntry> Model::getFunctionEntries(iden name)
{
  vector<FunctionEntry> entries;

  lsort sort;
  for (VarDecl decl : module->functions) {
    if (decl.name == name) {
      sort = decl.sort;
      break;
    }
  }
  assert(sort != nullptr);

  FunctionInfo const& finfo = get_function_info(name);

  size_t num_args;
  Sort* range_sort;
  vector<Sort*> domain_sorts;
  if (FunctionSort* functionSort = dynamic_cast<FunctionSort*>(sort.get())) {
    num_args = functionSort->domain.size();
    range_sort = functionSort->range.get();
    for (auto ptr : functionSort->domain) {
      Sort* argsort = ptr.get();
      domain_sorts.push_back(argsort);
    }
  } else {
    num_args = 0;
    range_sort = sort.get();
  }

  vector<object_value> args;
  for (int i = 0; i < num_args; i++) {
    args.push_back(0);
  }
  while (true) {
    FunctionEntry entry;

    FunctionTable* ftable = finfo.table.get();
    for (int i = 0; i < num_args; i++) {
      if (ftable == NULL) break;
      ftable = ftable->children[args[i]].get();
    }
    for (int i = 0; i < num_args; i++) {
      entry.args.push_back(args[i]);
    }
    if (ftable != NULL) {
      entry.res = ftable->value;
    } else {
      entry.res = finfo.else_value;
    }
    entries.push_back(entry);

    int i;
    for (i = num_args - 1; i >= 0; i--) {
      args[i]++;
      if (args[i] == get_domain_size(domain_sorts[i])) {
        args[i] = 0;
      } else {
        break;
      }
    }
    if (i == -1) {
      break;
    }
  }

  return entries;
}

Json Model::to_json() const {
  map<string, Json> o_sort_info;
  for (auto& p : sort_info) {
    o_sort_info.insert(make_pair(p.first, Json((int)p.second.domain_size)));
  }
  Json j_sort_info = Json(o_sort_info);

  map<string, Json> o_function_info;
  for (auto& p : function_info) {
    o_function_info.insert(make_pair(iden_to_string(p.first), p.second.to_json()));
  }
  Json j_function_info = Json(o_function_info);

  map<string, Json> o;
  o.insert(make_pair("sorts", j_sort_info));
  o.insert(make_pair("functions", j_function_info));
  return Json(o);
}

shared_ptr<Model> Model::from_json(Json j, shared_ptr<Module> module) {
  std::unordered_map<std::string, SortInfo> sort_info;
  std::unordered_map<iden, FunctionInfo> function_info;

  Json j_sort_info = j["sorts"];
  Json j_func_info = j["functions"];
  assert(j_sort_info.is_object());
  assert(j_func_info.is_object());

  for (auto p : j_sort_info.object_items()) {
    Json j = p.second;
    assert(j.is_number());
    SortInfo si;
    si.domain_size = j.int_value();

    sort_info.insert(make_pair(p.first, si));
  }

  for (auto p : j_func_info.object_items()) {
    Json j = p.second;
    function_info.insert(make_pair(string_to_iden(p.first), FunctionInfo::from_json(j)));
  }

  return shared_ptr<Model>(new Model(module, move(sort_info), move(function_info)));
}

Json FunctionInfo::to_json() const {
  map<string, Json> o;
  o.insert(make_pair("else", Json((int)else_value)));
  o.insert(make_pair("table", table == nullptr ? Json() : table->to_json()));
  return Json(o);
}

FunctionInfo FunctionInfo::from_json(Json j) {
  assert(j.is_object());
  FunctionInfo fi;
  assert(j["else"].is_number());
  fi.else_value = (object_value) j["else"].int_value();
  fi.table = FunctionTable::from_json(j["table"]);
  return fi;
}

Json FunctionTable::to_json() const {
  if (children.size() > 0) {
    vector<Json> v;
    for (auto& ftable : children) {
      v.push_back(ftable == nullptr ? Json() : ftable->to_json());
    }
    return Json(v);
  } else {
    return Json((int)value);
  }
}

unique_ptr<FunctionTable> FunctionTable::from_json(Json j) {
  unique_ptr<FunctionTable> ft;
  if (j.is_null()) {
    return ft;
  } else if (j.is_array()) {
    ft.reset(new FunctionTable());
    for (Json j1 : j.array_items()) {
      ft->children.push_back(FunctionTable::from_json(j1));
    }
  } else {
    ft.reset(new FunctionTable());
    assert(j.is_number());
    ft->value = (object_value) j.int_value();
  }
  return ft;
}
