#include "model.h"

#include <cassert>
#include <map>

using namespace std;

bool Model::eval_predicate(shared_ptr<Value> value) const {
  return eval(value, {}) == 1;
}

object_value Model::eval(
    std::shared_ptr<Value> v,
    std::unordered_map<std::string, object_value> const& vars) const
{
  assert(v.get() != NULL);

  if (Forall* value = dynamic_cast<Forall*>(v.get())) {
    std::unordered_map<std::string, object_value> vars_copy(vars);
    for (VarDecl decl : value->decls) {
      vars_copy[decl.name] = 0;
    }
    return do_forall(value, vars_copy, 0);
  }
  else if (Exists* value = dynamic_cast<Exists*>(v.get())) {
    std::unordered_map<std::string, object_value> vars_copy(vars);
    for (VarDecl decl : value->decls) {
      vars_copy[decl.name] = 0;
    }
    return do_exists(value, vars_copy, 0);
  }
  else if (Var* value = dynamic_cast<Var*>(v.get())) {
    auto iter = vars.find(value->name);
    if (iter == vars.end()) {
      printf("could not find var: %s\n", value->name.c_str());
      assert(false);
    }
    return iter->second;
  }
  else if (Const* value = dynamic_cast<Const*>(v.get())) {
    auto iter = function_info.find(value->name);
    assert(iter != function_info.end());
    FunctionInfo const& finfo = iter->second;
    FunctionTable* ftable = finfo.table.get();
    return ftable == NULL ? finfo.else_value : ftable->value;
  }
  else if (Eq* value = dynamic_cast<Eq*>(v.get())) {
    return (object_value)(eval(value->left, vars) == eval(value->right, vars));
  }
  else if (Not* value = dynamic_cast<Not*>(v.get())) {
    return (object_value)(!eval(value->value, vars));
  }
  else if (Implies* value = dynamic_cast<Implies*>(v.get())) {
    return (object_value)(!eval(value->left, vars) || eval(value->right, vars));
  }
  else if (Apply* value = dynamic_cast<Apply*>(v.get())) {
    Const* func = dynamic_cast<Const*>(value->func.get());
    auto iter = function_info.find(func->name);
    if (iter == function_info.end()) {
      printf("could not find function name %s\n", func->name.c_str());
      assert(false);
    }
    FunctionInfo const& finfo = iter->second;
    FunctionTable* ftable = finfo.table.get();
    for (auto arg : value->args) {
      if (ftable == NULL) {
        break;
      }
      object_value arg_res = eval(arg, vars);
      ftable = ftable->children[arg_res].get();
    }
    return ftable == NULL ? finfo.else_value : ftable->value;
  }
  else if (And* value = dynamic_cast<And*>(v.get())) {
    for (auto arg : value->args) {
      if (!eval(arg, vars)) {
        return false;
      }
    }
    return true;
  }
  else if (Or* value = dynamic_cast<Or*>(v.get())) {
    for (auto arg : value->args) {
      if (eval(arg, vars)) {
        return true;
      }
    }
    return false;
  }
  else {
    assert(false && "value2expr does not support this case");
  }
}

object_value Model::do_forall(
    Forall* value,
    std::unordered_map<std::string, object_value>& vars,
    int quantifier_index) const
{
  if (quantifier_index == value->decls.size()) {
    return eval(value->body, vars);    
  }

  VarDecl const& decl = value->decls[quantifier_index];
  auto iter = vars.find(decl.name);
  assert(iter != vars.end());

  size_t domain_size = get_domain_size(decl.sort.get());

  for (object_value i = 0; i < domain_size; i++) {
    iter->second = i;
    if (!do_forall(value, vars, quantifier_index + 1)) {
      return false;
    }
  }
  return true;
}

object_value Model::do_exists(
    Exists* value,
    std::unordered_map<std::string, object_value>& vars,
    int quantifier_index) const
{
  if (quantifier_index == value->decls.size()) {
    return eval(value->body, vars);    
  }

  VarDecl const& decl = value->decls[quantifier_index];
  auto iter = vars.find(decl.name);
  assert(iter != vars.end());

  size_t domain_size = get_domain_size(decl.sort.get());

  for (object_value i = 0; i < domain_size; i++) {
    iter->second = i;
    if (do_exists(value, vars, quantifier_index + 1)) {
      return true;
    }
  }
  return false;
}

shared_ptr<Model> Model::extract_model_from_z3(
    z3::context& ctx,
    z3::solver& solver,
    std::shared_ptr<Module> module,
    ModelEmbedding const& e)
{
  shared_ptr<Model> model = make_shared<Model>();
  model->module = module;

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
    model->sort_info[name] = sinfo;
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

  auto get_expr = [&z3model, &ctx, &universes](
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
    string name = decl.name;
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
          sz = model->sort_info[usort->name].domain_size;
        } else {
          assert(false && "expected boolean sort or uninterpreted sort");
        }
        domain_sort_sizes.push_back(sz);
      }
    } else {
      num_args = 0;
      range_sort = decl.sort.get();
    }

    model->function_info.insert(make_pair(name, FunctionInfo()));
    FunctionInfo& finfo = model->function_info[name];

    if (z3model.has_interp(fdecl)) {
      if (fdecl.is_const()) {
        z3::expr e = z3model.get_const_interp(fdecl);
        finfo.else_value = 0;
        finfo.table.reset(new FunctionTable());
        finfo.table->value = get_value(range_sort, e);
      } else {
        z3::func_interp finterp = z3model.get_func_interp(fdecl);

        /*
        printf("name = %s\n", fdecl.name().str().c_str());
        printf("else value\n");
        finfo.else_value = get_value(range_sort, finterp.else_value());
        */
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

  return model;
}

void Model::dump() const {
  printf("Model:\n\n");
  for (string sort : module->sorts) {
    int domain_size = (int) get_domain_size(sort);
    printf("%s: %d\n", sort.c_str(), domain_size);
  }
  printf("\n");
  for (VarDecl decl : module->functions) {
    string name = decl.name;
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

    printf("%s(default) -> %s\n", name.c_str(),
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
        printf("%s(", name.c_str());
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

FunctionInfo const& Model::get_function_info(std::string name) const {
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
    string name = decl.name;
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
