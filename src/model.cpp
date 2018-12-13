#include "model.h"

#include <cassert>

using namespace std;

bool Model::eval_predicate(shared_ptr<Value> value) {
  return eval(value, {}) == 1;
}

object_value Model::eval(
    std::shared_ptr<Value> v,
    std::unordered_map<std::string, object_value> const& vars)
{
  assert(v.get() != NULL);

  if (Forall* value = dynamic_cast<Forall*>(v.get())) {
    std::unordered_map<std::string, object_value> vars_copy(vars);
    for (VarDecl decl : value->decls) {
      vars_copy[decl.name] = 0;
    }
    return do_forall(value, vars_copy, 0);
  }
  else if (Var* value = dynamic_cast<Var*>(v.get())) {
    auto iter = vars.find(value->name);
    assert(iter != vars.end());
    return iter->second;
  }
  else if (Const* value = dynamic_cast<Const*>(v.get())) {
    auto iter = function_info.find(value->name);
    assert(iter != function_info.end());
    FunctionInfo* finfo = iter->second.get();
    return finfo == NULL ? 0 : finfo.value;
  }
  else if (Eq* value = dynamic_cast<Eq*>(v.get())) {
    return (object_value)(eval(value->left, vars), eval(value->right, vars));
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
    assert (iter != function_info.end());
    FunctionInfo* f = &iter->second;
    for (auto arg : value->args) {
      if (f == NULL) {
        break;
      }
      object_value arg_res = eval(arg, vars);
      f = f->contents[arg_res].get();
    }
    return f == NULL ? 0 : f->value;
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
    int quantifier_index)
{
  if (quantifier_index == value->decls.size()) {
    return eval(value->body, vars);    
  }

  VarDecl const& decl = value->decls[quantifier_index];
  auto iter = vars.find(decl.name);
  assert(iter != vars.end());

  int domain_size = sort_info[decl.name].domain_size;

  for (object_value i = 0; i < domain_size; i++) {
    iter->second = i;
    if (!do_forall(value, vars, quantifier_index + 1)) {
      return false;
    }
  }
  return true;
}

Model Model::extract_model_from_z3(
    z3::context ctx,
    z3::solver solver,
    std::shared_ptr<Module> module,
    ModelEmbedding const& e)
{
  Model model;
  model.module = module;

  z3::model z3model = solver.get_model();

  map<string, expr_vector> universes;

  for (auto p : e.ctx->sorts) {
    string name = p.first;
    z3::sort s = p.second;

    // The C++ api doesn't seem to have the functionality we need.
    // Go down to the C API.
    Z3_ast_vector c_univ = Z3_model_get_sort_universe(ctx, model, s);
    z3::expr_vector univ(ctx, c_univ);

    universes.insert(make_pair(name, univ));

    int len = univ.size();

    SortInfo sinfo;
    sinfo.domain_size = len;
    model.sort_info[name] = sinfo;
  }

  for (auto p : e.ctx->functions) {
    string name = p.first;  
    z3::func_decl fdecl = p.second;

    function_info.insert(make_pair(name, FunctionInfo()));
    FunctionInfo& finfo = function_info[name];

    if (z3model.has_interp(fdecl)) {
      finfo.else_value = get_value(range_sort_name, finfo.else_value());

      z3::func_interp finterp = z3model.get_func_interp(fdecl);
      for (size_t i = 0; i < finterp.num_entries(); i++) {
        func_entry fentry = finterp.entry(i);
        
        unique_ptr<FunctionTable>& table = finfo.table;
        for (int argnum = 0; argnum < num_args; argnum++) {
          object_value argvalue = get_value(domain_sort_names[argnum], fentry.arg(argnum));

          if (!table.get()) {
            table.reset(new FunctionTable());
            table->children.resize(domain_sort_sizes[argnum]);
          }
          assert(0 <= argvalue && argvalue < domain_sort_sizes[argnum]);
          table = table->children[argvalue];

          table->value = get_value(fentry.value(), range_sort_name);
        }
      }
    } else {
      finfo.else_value = 0;
    }
  }
}
