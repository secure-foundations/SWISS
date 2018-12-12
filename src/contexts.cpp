#include "contexts.h"

using namespace std;
using z3::context;
using z3::solver;
using z3::sort;
using z3::func_decl;
using z3::expr;

BackgroundContext::BackgroundContext(z3::context& ctx, std::shared_ptr<Module> module)
    : ctx(ctx),
      solver(ctx)
{
  for (string sort : module->sorts) {
    this->sorts.insert(make_pair(sort, ctx.uninterpreted_sort(sort.c_str())));
  }
}

z3::sort BackgroundContext::getUninterpretedSort(std::string name) {
  return sorts.find(name)->second;
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

shared_ptr<ModelEmbedding> ModelEmbedding::makeEmbedding(
    shared_ptr<BackgroundContext> ctx,
    shared_ptr<Module> module)
{
  unordered_map<string, func_decl> mapping;
  for (VarDecl decl : module->functions) {
    Sort* s = decl.sort.get();
    if (FunctionSort* fsort = dynamic_cast<FunctionSort*>(s)) {
      vector<z3::sort> domain;
      for (std::shared_ptr<Sort> domain_sort : fsort->domain) {
        domain.push_back(ctx->getSort(domain_sort));
      }
      z3::sort range = ctx->getSort(fsort->range);
      mapping.insert(make_pair(decl.name, ctx->ctx.function(
          decl.name.c_str(), domain.size(),
          &domain[0], range)));
    } else {
      mapping.insert(make_pair(decl.name, ctx->ctx.function(decl.name.c_str(), 0, NULL,
          ctx->getSort(decl.sort))));
    }
  }

  return shared_ptr<ModelEmbedding>(new ModelEmbedding(ctx, mapping));
}

struct ActionResult {
  std::shared_ptr<ModelEmbedding> e;
  expr constraint;

  ActionResult(
    std::shared_ptr<ModelEmbedding> e,
    expr constraint)
  : e(e), constraint(constraint) { }
};

ActionResult applyAction(
    shared_ptr<ModelEmbedding> e,
    shared_ptr<Action> action,
    unordered_map<string, expr> const& consts);

InductionContext::InductionContext(
    std::shared_ptr<BackgroundContext> ctx,
    std::shared_ptr<Module> module)
    : ctx(ctx)
{
  this->e1 = ModelEmbedding::makeEmbedding(ctx, module);

  shared_ptr<Action> action = shared_ptr<Action>(new ChoiceAction(module->actions));
  ActionResult res = applyAction(this->e1, action, {});
  this->e2 = res.e;
  ctx->solver.add(res.constraint);
}

ActionResult do_if_else(
    shared_ptr<ModelEmbedding> e,
    shared_ptr<Value> condition,
    shared_ptr<Action> then_body,
    shared_ptr<Action> else_body,
    unordered_map<string, z3::expr> const& consts)
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

ActionResult applyAction(
    shared_ptr<ModelEmbedding> e,
    shared_ptr<Action> a,
    unordered_map<string, expr> const& consts)
{
  shared_ptr<BackgroundContext> ctx = e->ctx;

  if (LocalAction* action = dynamic_cast<LocalAction*>(a.get())) {
    unordered_map<string, expr> new_consts(consts);
    for (VarDecl decl : action->args) {
      new_consts.insert(make_pair(decl.name,
          ctx->ctx.constant(decl.name.c_str(), ctx->getSort(decl.sort))));
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
    z3::expr_vector parts(ctx->ctx);
    for (int i = 0; i < len; i++) {
      ActionResult res = applyAction(e, action->actions[i], consts);
      es.push_back(res.e);
      parts.push_back(res.constraint);
    }
    for (auto p : e->mapping) {
      string func_name = p.first;

      bool is_ident = true;
      func_decl new_func_decl = e->getFunc(func_name);
      for (int i = 0; i < len; i++) {
        if (es[i]->getFunc(func_name).name() != e->getFunc(func_name).name()) {
          is_ident = false;
          new_func_decl = es[i]->mapping.find(func_name)->second;
          break;
        }
      }
      if (!is_ident) {
        for (int i = 0; i < len; i++) {
          if (es[i]->getFunc(func_name).name() != e->getFunc(func_name).name()) {
            parts[i] = and2(
                funcs_equal(es[i]->getFunc(func_name), new_func_decl),
                parts[i]);
            es[i]->getFunc(func_name) = new_func_decl;
          }
        }
      }
    }
    return ActionResult(es[0], z3::mk_or(parts));
  }
  else if (ChoiceAction* action = dynamic_cast<ChoiceAction*>(a.get())) {
    assert(false && "not implemented");
  }
  else {
    assert(false && "applyAction does not implement this unknown case");
  }
}
