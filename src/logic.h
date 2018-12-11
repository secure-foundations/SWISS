#ifndef LOGIC_H
#define LOGIC_H

#include <string>
#include <vector>

/* Sort */

class Sort {
};

class BooleanSort : public Sort {
};

class UninterpretedSort : public Sort {
public:
  std::string const name;

  UninterpretedSort(std::string const& name) : name(name) { }
};

class FunctionSort : public Sort {
public:
  std::vector<std::shared_ptr<Sort>> const domain;
  std::shared_ptr<Sort> range;

  FunctionSort(
      std::vector<std::shared_ptr<Sort>> const& domain,
      std::shared_ptr<Sort> range)
      : domain(domain), range(range) { }
};

/* VarDecl */

class VarDecl {
public:
  std::string const name;
  std::shared_ptr<Sort> const sort;

  VarDecl(
      std::string const& name,
      std::shared_ptr<Sort> sort)
      : name(name), sort(sort) { }
};

/* Value */

class Value {
};

class Forall : public Value {
public:
  std::vector<VarDecl> const decls;
  std::shared_ptr<Value> const body;

  Forall(
      std::vector<VarDecl> const& decls,
      std::shared_ptr<Value> body)
      : decls(decls), body(body) { }
};

class Var : public Value {
public:
  std::string const name;
  std::shared_ptr<Sort> sort;

  Var(
      std::string const& name,
      std::shared_ptr<Sort> sort)
      : name(name), sort(sort) { }
};

class Const : public Value {
public:
  std::string const name;
  std::shared_ptr<Sort> sort;

  Const(
      std::string const& name,
      std::shared_ptr<Sort> sort)
      : name(name), sort(sort) { }
};

class Eq : public Value {
public:
  std::shared_ptr<Value> left;
  std::shared_ptr<Value> right;

  Eq(
    std::shared_ptr<Value> left,
    std::shared_ptr<Value> right)
    : left(left), right(right) { }
};

class Not : public Value {
public:
  std::shared_ptr<Value> value;

  Not(
    std::shared_ptr<Value> value)
    : value(value) { }
};

class Implies : public Value {
public:
  std::shared_ptr<Value> left;
  std::shared_ptr<Value> right;

  Implies(
    std::shared_ptr<Value> left,
    std::shared_ptr<Value> right)
    : left(left), right(right) { }
};

class Apply : public Value {
public:
  std::shared_ptr<Value> func;
  std::vector<std::shared_ptr<Value>> args;

  Apply(
    std::shared_ptr<Value> func,
    std::vector<std::shared_ptr<Value>> const& args)
    : func(func), args(args) { }
};

class And : public Value {
public:
  std::vector<std::shared_ptr<Value>> args;

  And(
    std::vector<std::shared_ptr<Value>> const& args)
    : args(args) { }
};

class Or : public Value {
public:
  std::vector<std::shared_ptr<Value>> args;

  Or(
    std::vector<std::shared_ptr<Value>> const& args)
    : args(args) { }
};

/* Action */

class Action {
};

class LocalAction : public Action {
public:
  std::vector<VarDecl> args;
  std::shared_ptr<Action> body;

  LocalAction(
    std::vector<VarDecl> const& args,
    std::shared_ptr<Action> body)
    : args(args), body(body) { }
};

class SequenceAction : public Action {
public:
  std::vector<std::shared_ptr<Action>> actions;

  SequenceAction(
    std::vector<std::shared_ptr<Action>> const& actions)
    : actions(actions) { }
};

class ChoiceAction : public Action {
public:
  std::vector<std::shared_ptr<Action>> actions;

  ChoiceAction(
    std::vector<std::shared_ptr<Action>> const& actions)
    : actions(actions) { }
};

class Assume : public Action {
public:
  std::shared_ptr<Value> body;

  Assume(
    std::shared_ptr<Value> body)
    : body(body) { }
};

class Assign : public Action {
public:
  std::shared_ptr<Value> left;
  std::shared_ptr<Value> right;

  Assign(
    std::shared_ptr<Value> left,
    std::shared_ptr<Value> right)
    : left(left), right(right) { }
};

class IfElse : public Action {
public:
  std::shared_ptr<Value> condition;
  std::shared_ptr<Action> then_body;
  std::shared_ptr<Action> else_body;

  IfElse(
    std::shared_ptr<Value> condition,
    std::shared_ptr<Action> then_body,
    std::shared_ptr<Action> else_body)
    : condition(condition), then_body(then_body), else_body(else_body) { }
};

class If : public Action {
public:
  std::shared_ptr<Value> condition;
  std::shared_ptr<Action> then_body;

  If(
    std::shared_ptr<Value> condition,
    std::shared_ptr<Action> then_body)
    : condition(condition), then_body(then_body) { }
};

/* Module */

class Module {
public:
  std::vector<std::string> sorts;
  std::vector<VarDecl> functions;
  std::vector<std::shared_ptr<Value>> axioms;
  std::vector<std::shared_ptr<Value>> inits;
  std::vector<std::shared_ptr<Value>> conjectures;
  std::vector<std::shared_ptr<Action>> actions;

  Module(
    std::vector<std::string> const& sorts,
    std::vector<VarDecl> const& functions,
    std::vector<std::shared_ptr<Value>> const& axioms,
    std::vector<std::shared_ptr<Value>> const& inits,
    std::vector<std::shared_ptr<Value>> const& conjectures,
    std::vector<std::shared_ptr<Action>> const& actions)
    : sorts(sorts),
      functions(functions),
      axioms(axioms),
      inits(inits),
      conjectures(conjectures),
      actions(actions) { }
};

std::shared_ptr<Module> parse_module(std::string const& src);

#endif
