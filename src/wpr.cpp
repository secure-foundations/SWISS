#include "wpr.h"

#include <cassert>

using namespace std;

value wpr(value v, shared_ptr<Action> a)
{
  if (LocalAction* action = dynamic_cast<LocalAction*>(a.get())) {
    map<iden, iden> newLocals;
    vector<VarDecl> forallDecls;
    for (auto arg : action->args) {
      VarDecl d = freshVarDecl(arg.sort);
      newLocals.insert(make_pair(arg.name, d.name));
      forallDecls.push_back(d);
    }

    return v_forall(forallDecls, wpr(v, action->body)->replace_const_with_var(newLocals));
  }
  else if (SequenceAction* action = dynamic_cast<SequenceAction*>(a.get())) {
    for (int i = action->actions.size() - 1; i >= 0; i--) {
      v = wpr(v, action->actions[i]);
    }
    return v;
  }
  else if (Assume* action = dynamic_cast<Assume*>(a.get())) {
    return v_implies(action->body, v);
  }
  else if (If* action = dynamic_cast<If*>(a.get())) {
    vector<value> values;
    values.push_back(v_implies(action->condition, wpr(v, action->then_body)));
    values.push_back(v_implies(v_not(action->condition), v));
    return v_and(values);
  }
  else if (IfElse* action = dynamic_cast<IfElse*>(a.get())) {
    vector<value> values;
    values.push_back(v_implies(action->condition, wpr(v, action->then_body)));
    values.push_back(v_implies(v_not(action->condition), wpr(v, action->else_body)));
    return v_and(values);
  }
  else if (ChoiceAction* action = dynamic_cast<ChoiceAction*>(a.get())) {
    vector<value> values;
    for (int i = 0; i < (int)action->actions.size(); i++) {
      values.push_back(wpr(v, action->actions[i]));
    }
    return v_and(values);
  }
  else if (Assign* action = dynamic_cast<Assign*>(a.get())) {
    Apply* apply = dynamic_cast<Apply*>(action->left.get());
    assert(apply != NULL);

    vector<VarDecl> decls;
    vector<value> eqs;
    vector<value> args;

    Const* c = dynamic_cast<Const*>(apply->func.get());
    assert(c != NULL);

    for (int i = 0; i < (int)apply->args.size(); i++) {
      value arg = apply->args[i];
      if (Var* arg_var = dynamic_cast<Var*>(arg.get())) {
        args.push_back(arg);
        decls.push_back(VarDecl(arg_var->name, arg_var->sort));
      } else {
        VarDecl arg_decl = freshVarDecl(c->sort->get_domain_as_function()[i]);
        decls.push_back(arg_decl);
        value placeholder = v_var(arg_decl.name, arg_decl.sort);
        args.push_back(placeholder);
        eqs.push_back(v_eq(placeholder, arg));
      }
    }

    value expr = v_if_then_else(
      v_and(eqs),
      action->right,
      v_apply(apply->func, args));

    return v->subst_fun(c->name, decls, expr);
  }
  else if (dynamic_cast<Havoc*>(a.get())) {
    assert(false);
  }
  else {
    assert(false && "applyAction does not implement this unknown case");
  }
  assert(false);
}
