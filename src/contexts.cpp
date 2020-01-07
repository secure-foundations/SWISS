#include "contexts.h"
#include "model.h"
#include "wpr.h"

#include <cstdlib>

using namespace std;
using z3::context;
using z3::solver;
using z3::sort;
using z3::func_decl;
using z3::expr;

int name_counter = 1;
string name(iden basename) {
  return "x" + to_string(rand()) + "_" +
      iden_to_string(basename) + "__" + to_string(name_counter++);
}

string name(string basename) {
  return "x" + to_string(rand()) + "_" +
      basename + "__" + to_string(name_counter++);
}

/*
 * BackgroundContext
 */

BackgroundContext::BackgroundContext(z3::context& ctx, std::shared_ptr<Module> module)
    : ctx(ctx),
      solver(ctx)
{
  for (string sort : module->sorts) {
    this->sorts.insert(make_pair(sort, ctx.uninterpreted_sort(sort.c_str())));
  }
}

z3::sort BackgroundContext::getUninterpretedSort(std::string name) {
  auto iter = sorts.find(name);
  if (iter == sorts.end()) {
    printf("failed to find sort %s\n", name.c_str());
    assert(false);
  }
  return iter->second;
}

z3::sort BackgroundContext::getSort(std::shared_ptr<Sort> sort) {
  Sort* s = sort.get();
  if (dynamic_cast<BooleanSort*>(s)) {
    return ctx.bool_sort();
  } else if (UninterpretedSort* usort = dynamic_cast<UninterpretedSort*>(s)) {
    return getUninterpretedSort(usort->name);
  } else if (FunctionSort* fsort = dynamic_cast<FunctionSort*>(s)) {
    assert(false && "getSort does not support FunctionSort");
  } else {
    assert(false && "getSort does not support unknown sort");
  }
}

/*
 * ModelEmbedding
 */

shared_ptr<ModelEmbedding> ModelEmbedding::makeEmbedding(
    shared_ptr<BackgroundContext> ctx,
    shared_ptr<Module> module)
{
  unordered_map<iden, func_decl> mapping;
  for (VarDecl decl : module->functions) {
    Sort* s = decl.sort.get();
    if (FunctionSort* fsort = dynamic_cast<FunctionSort*>(s)) {
      z3::sort_vector domain(ctx->ctx);
      for (std::shared_ptr<Sort> domain_sort : fsort->domain) {
        domain.push_back(ctx->getSort(domain_sort));
      }
      z3::sort range = ctx->getSort(fsort->range);
      mapping.insert(make_pair(decl.name, ctx->ctx.function(
          name(decl.name).c_str(), domain, range)));
    } else {
      mapping.insert(make_pair(decl.name, ctx->ctx.function(name(decl.name).c_str(), 0, 0,
          ctx->getSort(decl.sort))));
    }
  }

  return shared_ptr<ModelEmbedding>(new ModelEmbedding(ctx, mapping));
}

z3::func_decl ModelEmbedding::getFunc(iden name) const {
  auto iter = mapping.find(name);
  if (iter == mapping.end()) {
    cout << "couldn't find function " << iden_to_string(name) << endl;
    assert(false);
  }
  return iter->second;
}

z3::expr ModelEmbedding::value2expr(
    shared_ptr<Value> value)
{
  return value2expr(value, {}, {});
}


z3::expr ModelEmbedding::value2expr(
    shared_ptr<Value> value,
    std::unordered_map<iden, z3::expr> const& consts)
{
  return value2expr(value, consts, {});
}

z3::expr ModelEmbedding::value2expr_with_vars(
    shared_ptr<Value> value,
    std::unordered_map<iden, z3::expr> const& vars)
{
  return value2expr(value, {}, vars);
}

z3::expr ModelEmbedding::value2expr(
    shared_ptr<Value> v,
    std::unordered_map<iden, z3::expr> const& consts,
    std::unordered_map<iden, z3::expr> const& vars)
{
  assert(v.get() != NULL);
  if (Forall* value = dynamic_cast<Forall*>(v.get())) {
    z3::expr_vector vec_vars(ctx->ctx);
    std::unordered_map<iden, z3::expr> new_vars = vars;
    for (VarDecl decl : value->decls) {
      expr var = ctx->ctx.constant(name(decl.name).c_str(), ctx->getSort(decl.sort));
      vec_vars.push_back(var);
      new_vars.insert(make_pair(decl.name, var));
    }
    return z3::forall(vec_vars, value2expr(value->body, consts, new_vars));
  }
  else if (Exists* value = dynamic_cast<Exists*>(v.get())) {
    z3::expr_vector vec_vars(ctx->ctx);
    std::unordered_map<iden, z3::expr> new_vars = vars;
    for (VarDecl decl : value->decls) {
      expr var = ctx->ctx.constant(name(decl.name).c_str(), ctx->getSort(decl.sort));
      vec_vars.push_back(var);
      new_vars.insert(make_pair(decl.name, var));
    }
    return z3::exists(vec_vars, value2expr(value->body, consts, new_vars));
  }
  else if (NearlyForall* value = dynamic_cast<NearlyForall*>(v.get())) {
    z3::expr_vector vec_vars(ctx->ctx);
    z3::expr_vector all_eq(ctx->ctx);
    std::unordered_map<iden, z3::expr> new_vars1 = vars;
    std::unordered_map<iden, z3::expr> new_vars2 = vars;
    for (VarDecl decl : value->decls) {
      expr var1 = ctx->ctx.constant(name(decl.name).c_str(), ctx->getSort(decl.sort));
      expr var2 = ctx->ctx.constant(name(decl.name).c_str(), ctx->getSort(decl.sort));
      vec_vars.push_back(var1);
      vec_vars.push_back(var2);
      new_vars1.insert(make_pair(decl.name, var1));
      new_vars2.insert(make_pair(decl.name, var2));
      all_eq.push_back(var1 == var2);
    }
    z3::expr_vector vec_or(ctx->ctx);
    vec_or.push_back(value2expr(value->body, consts, new_vars1));
    vec_or.push_back(value2expr(value->body, consts, new_vars2));
    vec_or.push_back(z3::mk_and(all_eq));
    return z3::forall(vec_vars, z3::mk_or(vec_or));
  }
  else if (Var* value = dynamic_cast<Var*>(v.get())) {
    auto iter = vars.find(value->name);
    if (iter == vars.end()) {
      printf("couldn't find var: %s\n", iden_to_string(value->name).c_str());
      assert(false);
    }
    return iter->second;
  }
  else if (Const* value = dynamic_cast<Const*>(v.get())) {
    auto iter = consts.find(value->name);
    if (iter == consts.end()) {
      auto iter = mapping.find(value->name);
      if (iter == mapping.end()) {
        printf("could not find %s\n", iden_to_string(value->name).c_str());
        assert(false);
      }
      z3::func_decl fd = iter->second;
      return fd();
    } else {
      return iter->second;
    }
  }
  else if (Eq* value = dynamic_cast<Eq*>(v.get())) {
    return value2expr(value->left, consts, vars) == value2expr(value->right, consts, vars);
  }
  else if (Not* value = dynamic_cast<Not*>(v.get())) {
    return !value2expr(value->val, consts, vars);
  }
  else if (Implies* value = dynamic_cast<Implies*>(v.get())) {
    return !value2expr(value->left, consts, vars) || value2expr(value->right, consts, vars);
  }
  else if (Apply* value = dynamic_cast<Apply*>(v.get())) {
    z3::expr_vector args(ctx->ctx);
    for (shared_ptr<Value> arg : value->args) {
      args.push_back(value2expr(arg, consts, vars));
    }
    Const* func_value = dynamic_cast<Const*>(value->func.get());
    assert(func_value != NULL);
    return getFunc(func_value->name)(args);
  }
  else if (And* value = dynamic_cast<And*>(v.get())) {
    z3::expr_vector args(ctx->ctx);
    for (shared_ptr<Value> arg : value->args) {
      args.push_back(value2expr(arg, consts, vars));
    }
    return mk_and(args);
  }
  else if (Or* value = dynamic_cast<Or*>(v.get())) {
    z3::expr_vector args(ctx->ctx);
    for (shared_ptr<Value> arg : value->args) {
      args.push_back(value2expr(arg, consts, vars));
    }
    return mk_or(args);
  }
  else if (IfThenElse* value = dynamic_cast<IfThenElse*>(v.get())) {
    return z3::ite(
        value2expr(value->cond, consts, vars),
        value2expr(value->then_value, consts, vars),
        value2expr(value->else_value, consts, vars)
      );
  }
  else {
    //printf("value2expr got: %s\n", v->to_string().c_str());
    assert(false && "value2expr does not support this case");
  }
}

/*
 * InductionContext
 */

InductionContext::InductionContext(
    z3::context& z3ctx,
    std::shared_ptr<Module> module,
    int action_idx)
    : ctx(new BackgroundContext(z3ctx, module)), action_idx(action_idx)
{
  assert(-1 <= action_idx && action_idx < (int)module->actions.size());

  this->e1 = ModelEmbedding::makeEmbedding(ctx, module);

  shared_ptr<Action> action = action_idx == -1
      ? shared_ptr<Action>(new ChoiceAction(module->actions))
      : module->actions[action_idx];
  ActionResult res = applyAction(this->e1, action, {});
  this->e2 = res.e;

  // Add the relation between the two states
  ctx->solver.add(res.constraint);

  // Add the axioms
  for (shared_ptr<Value> axiom : module->axioms) {
    ctx->solver.add(this->e1->value2expr(axiom, {}));
  }
}

/*
 * ChainContext
 */

ChainContext::ChainContext(
    z3::context& z3ctx,
    std::shared_ptr<Module> module,
    int numTransitions)
    : ctx(new BackgroundContext(z3ctx, module))
{
  this->es.resize(numTransitions + 1);
  this->es[0] = ModelEmbedding::makeEmbedding(ctx, module);

  shared_ptr<Action> action = shared_ptr<Action>(new ChoiceAction(module->actions));

  for (int i = 0; i < numTransitions; i++) {
    ActionResult res = applyAction(this->es[i], action, {});
    this->es[i+1] = res.e;

    // Add the relation between the two states
    ctx->solver.add(res.constraint);
  }

  // Add the axioms
  for (shared_ptr<Value> axiom : module->axioms) {
    ctx->solver.add(this->es[0]->value2expr(axiom, {}));
  }
}


ActionResult do_if_else(
    shared_ptr<ModelEmbedding> e,
    shared_ptr<Value> condition,
    shared_ptr<Action> then_body,
    shared_ptr<Action> else_body,
    unordered_map<iden, z3::expr> const& consts)
{
  vector<shared_ptr<Action>> seq1;
  seq1.push_back(shared_ptr<Action>(new Assume(condition)));
  seq1.push_back(then_body);

  vector<shared_ptr<Action>> seq2;
  seq2.push_back(shared_ptr<Action>(new Assume(shared_ptr<Value>(new Not(condition)))));
  if (else_body.get() != nullptr) {
    seq2.push_back(else_body);
  }

  vector<shared_ptr<Action>> choices = {
    shared_ptr<Action>(new SequenceAction(seq1)),
    shared_ptr<Action>(new SequenceAction(seq2))
  };

  return applyAction(e, shared_ptr<Action>(new ChoiceAction(choices)), consts);
}

expr funcs_equal(z3::context& ctx, func_decl a, func_decl b) {
  z3::expr_vector args(ctx);
  for (int i = 0; i < a.arity(); i++) {
    z3::sort arg_sort = a.domain(i);
    args.push_back(ctx.constant(name("arg").c_str(), arg_sort));
  }
  if (args.size() == 0) {
    return a(args) == b(args);
  } else {
    return z3::forall(args, a(args) == b(args));
  }
}

ActionResult applyAction(
    shared_ptr<ModelEmbedding> e,
    shared_ptr<Action> a,
    unordered_map<iden, expr> const& consts)
{
  shared_ptr<BackgroundContext> ctx = e->ctx;

  if (LocalAction* action = dynamic_cast<LocalAction*>(a.get())) {
    unordered_map<iden, expr> new_consts(consts);
    for (VarDecl decl : action->args) {
      func_decl d = ctx->ctx.function(name(decl.name).c_str(), 0, 0, ctx->getSort(decl.sort));
      expr ex = d();
      new_consts.insert(make_pair(decl.name, ex));
    }
    return applyAction(e, action->body, new_consts);
  }
  else if (SequenceAction* action = dynamic_cast<SequenceAction*>(a.get())) {
    z3::expr_vector parts(ctx->ctx);
    for (shared_ptr<Action> sub_action : action->actions) {
      ActionResult res = applyAction(e, sub_action, consts);
      e = res.e;
      parts.push_back(res.constraint);
    }
    expr ex = z3::mk_and(parts);
    return ActionResult(e, z3::mk_and(parts));
  }
  else if (Assume* action = dynamic_cast<Assume*>(a.get())) {
    return ActionResult(e, e->value2expr(action->body, consts));
  }
  else if (If* action = dynamic_cast<If*>(a.get())) {
    return do_if_else(e, action->condition, action->then_body, nullptr, consts);
  }
  else if (IfElse* action = dynamic_cast<IfElse*>(a.get())) {
    return do_if_else(e, action->condition, action->then_body, action->else_body, consts);
  }
  else if (ChoiceAction* action = dynamic_cast<ChoiceAction*>(a.get())) {
    int len = action->actions.size();
    vector<shared_ptr<ModelEmbedding>> es;
    vector<z3::expr> constraints;
    for (int i = 0; i < len; i++) {
      ActionResult res = applyAction(e, action->actions[i], consts);
      es.push_back(res.e);
      constraints.push_back(res.constraint);
    }
    unordered_map<iden, z3::func_decl> mapping;

    for (auto p : e->mapping) {
      iden func_name = p.first;

      bool is_ident = true;
      func_decl new_func_decl = e->getFunc(func_name);
      for (int i = 0; i < len; i++) {
        if (es[i]->getFunc(func_name).name() != e->getFunc(func_name).name()) {
          is_ident = false;
          new_func_decl = es[i]->mapping.find(func_name)->second;
          break;
        }
      }
      mapping.insert(make_pair(func_name, new_func_decl));
      if (!is_ident) {
        for (int i = 0; i < len; i++) {
          if (es[i]->getFunc(func_name).name() != new_func_decl.name()) {
            constraints[i] =
                funcs_equal(ctx->ctx, es[i]->getFunc(func_name), new_func_decl) &&
                constraints[i];
          }
        }
      }
    }

    z3::expr_vector constraints_vec(ctx->ctx);
    for (int i = 0; i < len; i++) {
      constraints_vec.push_back(constraints[i]);
    }

    return ActionResult(
        shared_ptr<ModelEmbedding>(new ModelEmbedding(e->ctx, mapping)),
        z3::mk_or(constraints_vec)
      );
  }
  else if (Assign* action = dynamic_cast<Assign*>(a.get())) {
    Value* left = action->left.get();
    Apply* apply = dynamic_cast<Apply*>(left);
    //assert(apply != NULL);

    Const* func_const = dynamic_cast<Const*>(apply != NULL ? apply->func.get() : left);
    assert(func_const != NULL);
    func_decl orig_func = e->getFunc(func_const->name);

    z3::sort_vector domain(ctx->ctx);
    for (int i = 0; i < orig_func.arity(); i++) {
      domain.push_back(orig_func.domain(i));
    }
    string new_name = name(func_const->name);
    func_decl new_func = ctx->ctx.function(new_name.c_str(),
        domain, orig_func.range());

    z3::expr_vector qvars(ctx->ctx);
    z3::expr_vector all_eq_parts(ctx->ctx);
    unordered_map<iden, z3::expr> vars;
    for (int i = 0; i < orig_func.arity(); i++) {
      assert(apply != NULL);
      shared_ptr<Value> arg = apply->args[i];
      if (Var* arg_var = dynamic_cast<Var*>(arg.get())) {
        expr qvar = ctx->ctx.constant(name(arg_var->name).c_str(), ctx->getSort(arg_var->sort));
        qvars.push_back(qvar);
        vars.insert(make_pair(arg_var->name, qvar));
      } else {
        expr qvar = ctx->ctx.constant(name("arg").c_str(), domain[i]);
        qvars.push_back(qvar);
        all_eq_parts.push_back(qvar == e->value2expr(arg, consts));
      }
    }

    unordered_map<iden, z3::func_decl> new_mapping = e->mapping;
    new_mapping.erase(func_const->name);
    new_mapping.insert(make_pair(func_const->name, new_func));
    ModelEmbedding* new_e = new ModelEmbedding(ctx, new_mapping);

    z3::expr inner = new_func(qvars) == z3::ite(
          z3::mk_and(all_eq_parts),
          e->value2expr(action->right, consts, vars),
          orig_func(qvars));
    z3::expr outer = qvars.size() == 0 ? inner : z3::forall(qvars, inner);

    ActionResult ar(shared_ptr<ModelEmbedding>(new_e), outer);

    return ar;
  }
  else if (Havoc* action = dynamic_cast<Havoc*>(a.get())) {
    Value* left = action->left.get();
    Apply* apply = dynamic_cast<Apply*>(left);
    //assert(apply != NULL);

    Const* func_const = dynamic_cast<Const*>(apply != NULL ? apply->func.get() : left);
    assert(func_const != NULL);
    func_decl orig_func = e->getFunc(func_const->name);

    z3::sort_vector domain(ctx->ctx);
    for (int i = 0; i < orig_func.arity(); i++) {
      domain.push_back(orig_func.domain(i));
    }
    string new_name = name(func_const->name);
    func_decl new_func = ctx->ctx.function(new_name.c_str(),
        domain, orig_func.range());

    z3::expr_vector qvars(ctx->ctx);
    z3::expr_vector all_eq_parts(ctx->ctx);
    unordered_map<iden, z3::expr> vars;
    for (int i = 0; i < orig_func.arity(); i++) {
      assert(apply != NULL);
      shared_ptr<Value> arg = apply->args[i];
      if (Var* arg_var = dynamic_cast<Var*>(arg.get())) {
        expr qvar = ctx->ctx.constant(name(arg_var->name).c_str(), ctx->getSort(arg_var->sort));
        qvars.push_back(qvar);
        vars.insert(make_pair(arg_var->name, qvar));
      } else {
        expr qvar = ctx->ctx.constant(name("arg").c_str(), domain[i]);
        qvars.push_back(qvar);
        all_eq_parts.push_back(qvar == e->value2expr(arg, consts));
      }
    }

    unordered_map<iden, z3::func_decl> new_mapping = e->mapping;
    new_mapping.erase(func_const->name);
    new_mapping.insert(make_pair(func_const->name, new_func));
    ModelEmbedding* new_e = new ModelEmbedding(ctx, new_mapping);

    z3::expr inner = z3::implies(
          !z3::mk_and(all_eq_parts),
          new_func(qvars) == orig_func(qvars));
    z3::expr outer = qvars.size() == 0 ? inner : z3::forall(qvars, inner);

    ActionResult ar(shared_ptr<ModelEmbedding>(new_e), outer);

    return ar;
  }
  else {
    assert(false && "applyAction does not implement this unknown case");
  }
}

void ModelEmbedding::dump() {
  for (auto p : mapping) {
    printf("%s -> %s\n", iden_to_string(p.first).c_str(), p.second.name().str().c_str());
  }
}

/*
 * BasicContext
 */

BasicContext::BasicContext(
    z3::context& z3ctx,
    std::shared_ptr<Module> module)
    : ctx(new BackgroundContext(z3ctx, module))
{
  this->e = ModelEmbedding::makeEmbedding(ctx, module);

  // Add the axioms
  for (shared_ptr<Value> axiom : module->axioms) {
    ctx->solver.add(this->e->value2expr(axiom));
  }
}

/*
 * InitContext
 */

InitContext::InitContext(
    z3::context& z3ctx,
    std::shared_ptr<Module> module)
    : ctx(new BackgroundContext(z3ctx, module))
{
  this->e = ModelEmbedding::makeEmbedding(ctx, module);

  // Add the axioms
  for (shared_ptr<Value> axiom : module->axioms) {
    ctx->solver.add(this->e->value2expr(axiom));
  }
  // Add the inits
  for (shared_ptr<Value> init : module->inits) {
    ctx->solver.add(this->e->value2expr(init));
  }
}

/*
 * ConjectureContext
 */

ConjectureContext::ConjectureContext(
    z3::context& z3ctx,
    std::shared_ptr<Module> module)
    : ctx(new BackgroundContext(z3ctx, module))
{
  this->e = ModelEmbedding::makeEmbedding(ctx, module);

  // Add the axioms
  for (shared_ptr<Value> axiom : module->axioms) {
    ctx->solver.add(this->e->value2expr(axiom));
  }

  // Add the conjectures
  shared_ptr<Value> all_conjectures = shared_ptr<Value>(new And(module->conjectures));
  shared_ptr<Value> not_conjectures = shared_ptr<Value>(new Not(all_conjectures));
  ctx->solver.add(this->e->value2expr(not_conjectures));
}

/*
 * InvariantsContext
 */

InvariantsContext::InvariantsContext(
    z3::context& z3ctx,
    std::shared_ptr<Module> module)
    : ctx(new BackgroundContext(z3ctx, module))
{
  this->e = ModelEmbedding::makeEmbedding(ctx, module);

  // Add the axioms
  for (shared_ptr<Value> axiom : module->axioms) {
    ctx->solver.add(this->e->value2expr(axiom));
  }
}

bool is_satisfiable(shared_ptr<Module> module, value candidate)
{
  z3::context ctx;  
  BasicContext basicctx(ctx, module);
  z3::solver& solver = basicctx.ctx->solver;
  solver.add(basicctx.e->value2expr(candidate));
  z3::check_result res = solver.check();
  assert (res == z3::sat || res == z3::unsat);
  return res == z3::sat;
}

bool is_complete_invariant(shared_ptr<Module> module, value candidate) {
  z3::context ctx;  

  {
    InitContext initctx(ctx, module);
    z3::solver& init_solver = initctx.ctx->solver;
    init_solver.add(initctx.e->value2expr(v_not(candidate)));
    z3::check_result init_res = init_solver.check();
    assert (init_res == z3::sat || init_res == z3::unsat);
    if (init_res == z3::sat) {
      return false;
    }
  }

  {
    ConjectureContext conjctx(ctx, module);
    z3::solver& conj_solver = conjctx.ctx->solver;
    conj_solver.add(conjctx.e->value2expr(candidate));
    z3::check_result conj_res = conj_solver.check();
    assert (conj_res == z3::sat || conj_res == z3::unsat);
    if (conj_res == z3::sat) {
      return false;
    }
  }

  {
    InductionContext indctx(ctx, module);
    z3::solver& solver = indctx.ctx->solver;
    solver.add(indctx.e1->value2expr(candidate));
    solver.add(indctx.e2->value2expr(v_not(candidate)));
    z3::check_result res = solver.check();
    assert (res == z3::sat || res == z3::unsat);
    if (res == z3::sat) {
      return false;
    }
  }

  return true;
}

bool is_itself_invariant(shared_ptr<Module> module, value candidate) {
  z3::context ctx;  

  {
    InitContext initctx(ctx, module);
    z3::solver& init_solver = initctx.ctx->solver;
    init_solver.add(initctx.e->value2expr(v_not(candidate)));
    z3::check_result init_res = init_solver.check();
    assert (init_res == z3::sat || init_res == z3::unsat);
    if (init_res == z3::sat) {
      return false;
    }
  }

  {
    InductionContext indctx(ctx, module);
    z3::solver& solver = indctx.ctx->solver;
    solver.add(indctx.e1->value2expr(candidate));
    solver.add(indctx.e2->value2expr(v_not(candidate)));
    z3::check_result res = solver.check();
    assert (res == z3::sat || res == z3::unsat);
    if (res == z3::sat) {
      return false;
    }
  }

  return true;
}

bool is_invariant_with_conjectures(std::shared_ptr<Module> module, value v) {
  return is_itself_invariant(module, v_and({v, v_and(module->conjectures)}));
}

bool is_invariant_with_conjectures(std::shared_ptr<Module> module, vector<value> v) {
  for (value c : module->conjectures) {
    v.push_back(c);
  }
  return is_itself_invariant(module, v);
}

bool is_itself_invariant(shared_ptr<Module> module, vector<value> candidates) {
  //printf("is_itself_invariant\n");

  z3::context ctx;  

  value full = v_and(candidates);

  for (value candidate : candidates) {
    //printf("%s\n", candidate->to_string().c_str());

    {
      InitContext initctx(ctx, module);
      z3::solver& init_solver = initctx.ctx->solver;
      init_solver.add(initctx.e->value2expr(v_not(candidate)));
      //printf("checking init condition...\n");
      z3::check_result init_res = init_solver.check();
      assert (init_res == z3::sat || init_res == z3::unsat);
      if (init_res == z3::sat) {
        return false;
      }
    }

    for (int i = 0; i < module->actions.size(); i++) {
      InductionContext indctx(ctx, module, i);
      z3::solver& solver = indctx.ctx->solver;
      solver.add(indctx.e1->value2expr(full));
      solver.add(indctx.e2->value2expr(v_not(candidate)));
      //printf("checking invariant condition...\n");
      z3::check_result res = solver.check();
      assert (res == z3::sat || res == z3::unsat);
      if (res == z3::sat) {
        //cout << "failed with action " << module->action_names[i] << endl;
        //cout << "failed with candidate " << candidate->to_string() << endl;

        /*shared_ptr<Model> m1 = Model::extract_model_from_z3(
            indctx.ctx->ctx, solver, module, *indctx.e1);
        shared_ptr<Model> m2 = Model::extract_model_from_z3(
            indctx.ctx->ctx, solver, module, *indctx.e2);
        m1->dump();
        m2->dump();*/

        /*auto m = Model::extract_minimal_models_from_z3(
            indctx.ctx->ctx,
            solver, module, {indctx.e1, indctx.e2}, nullptr);
        m[0]->dump();
        m[1]->dump();*/
        //cout << "hey " << m[0]->eval_predicate(candidates[2]) << endl;*/

        return false;
      }
    }
  }

  return true;
}

bool is_wpr_itself_inductive(shared_ptr<Module> module, value candidate, int wprIter) {
  z3::context ctx;

  shared_ptr<Action> action = shared_ptr<Action>(new ChoiceAction(module->actions));

  value wpr_candidate = candidate;
  for (int j = 0; j < wprIter; j++) {
    wpr_candidate = wpr(wpr_candidate, action)->simplify();
  }

  for (int i = 1; i <= wprIter + 1; i++) {
    cout << "is_wpr_itself_inductive: " << i << endl;

    ChainContext chainctx(ctx, module, i);
    z3::solver& solver = chainctx.ctx->solver;

    solver.add(chainctx.es[0]->value2expr(wpr_candidate));
    solver.add(chainctx.es[i]->value2expr(v_not(candidate)));
    z3::check_result res = solver.check();
    assert (res == z3::sat || res == z3::unsat);
    if (res == z3::sat) {
      return false;
    }
  }

  return true;
}

bool is_invariant_wrt(shared_ptr<Module> module, value invariant_so_far, value candidate) {
  z3::context ctx;  

  {
    InitContext initctx(ctx, module);
    z3::solver& init_solver = initctx.ctx->solver;
    init_solver.add(initctx.e->value2expr(v_not(candidate)));
    z3::check_result init_res = init_solver.check();
    assert (init_res == z3::sat || init_res == z3::unsat);
    if (init_res == z3::sat) {
      return false;
    }
  }

  for (int i = 0; i < module->actions.size(); i++) {
    InductionContext indctx(ctx, module, i);
    z3::solver& solver = indctx.ctx->solver;
    solver.add(indctx.e1->value2expr(invariant_so_far));
    solver.add(indctx.e1->value2expr(candidate));
    solver.add(indctx.e2->value2expr(v_not(candidate)));
    z3::check_result res = solver.check();
    assert (res == z3::sat || res == z3::unsat);
    if (res == z3::sat) {
      return false;
    }
  }

  return true;
}

void z3_set_timeout(z3::context& ctx, int ms) {
  ctx.set("timeout", ms);
}
