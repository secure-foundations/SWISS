#include "logic.h"

#include <cassert>
#include <iostream>
#include <algorithm>

#include "lib/json11/json11.hpp"
#include "benchmarking.h"

using namespace std;
using namespace json11;

shared_ptr<Module> json2module(Json j);
vector<string> json2string_array(Json j);
vector<shared_ptr<Value>> json2value_array(Json j);
vector<shared_ptr<Action>> json2action_array(Json j);
pair<vector<shared_ptr<Action>>, vector<string>> json2action_array_from_map(Json j);
vector<shared_ptr<Sort>> json2sort_array(Json j);
vector<VarDecl> json2decl_array(Json j);
shared_ptr<Value> json2value(Json j);
shared_ptr<Sort> json2sort(Json j);
shared_ptr<Action> json2action(Json j);

shared_ptr<Module> parse_module(string const& src) {
  string err;
  Json j = Json::parse(src, err);
  return json2module(j);
}

shared_ptr<Module> json2module(Json j) {
  assert(j.is_object());
  auto p = json2action_array_from_map(j["actions"]);
  return shared_ptr<Module>(new Module(
      json2string_array(j["sorts"]),
      json2decl_array(j["functions"]),
      json2value_array(j["axioms"]),
      json2value_array(j["inits"]),
      json2value_array(j["conjectures"]),
      json2value_array(j["templates"]),
      p.first,
      p.second));
}

vector<string> json2string_array(Json j) {
  assert(j.is_array());
  vector<string> res;
  for (Json elem : j.array_items()) {
    assert(elem.is_string());
    res.push_back(elem.string_value());
  }
  return res;
}

vector<shared_ptr<Value>> json2value_array(Json j) {
  assert(j.is_array());
  vector<shared_ptr<Value>> res;
  for (Json elem : j.array_items()) {
    res.push_back(json2value(elem));
  }
  return res;
}

vector<shared_ptr<Action>> json2action_array(Json j) {
  assert(j.is_array());
  vector<shared_ptr<Action>> res;
  for (Json elem : j.array_items()) {
    res.push_back(json2action(elem));
  }
  return res;
}

pair<vector<shared_ptr<Action>>, vector<string>> json2action_array_from_map(Json j) {
  assert(j.is_object());
  vector<shared_ptr<Action>> res;
  vector<string> names;
  for (auto p : j.object_items()) {
    res.push_back(json2action(p.second));
    names.push_back(p.first);
  }
  return make_pair(res, names);
}

vector<shared_ptr<Sort>> json2sort_array(Json j) {
  assert(j.is_array());
  vector<shared_ptr<Sort>> res;
  for (Json elem : j.array_items()) {
    res.push_back(json2sort(elem));
  }
  return res;
}

vector<VarDecl> json2decl_array(Json j) {
  assert(j.is_array());
  vector<VarDecl> res;
  for (Json elem : j.array_items()) {
    assert(elem.is_array());
    assert(elem.array_items().size() == 3);
    assert(elem[0] == "const" || elem[0] == "var");
    assert(elem[1].is_string());
    res.push_back(VarDecl(string_to_iden(elem[1].string_value()), json2sort(elem[2])));
  }
  return res;
}

shared_ptr<Value> json2value(Json j) {
  assert(j.is_array());
  assert(j.array_items().size() >= 1);
  assert(j[0].is_string());
  string type = j[0].string_value();
  if (type == "forall") {
    assert (j.array_items().size() == 3);
    return shared_ptr<Value>(new Forall(json2decl_array(j[1]), json2value(j[2])));
  }
  if (type == "nearlyforall") {
    assert (j.array_items().size() == 3);
    return shared_ptr<Value>(new NearlyForall(json2decl_array(j[1]), json2value(j[2])));
  }
  else if (type == "exists") {
    assert (j.array_items().size() == 3);
    return shared_ptr<Value>(new Exists(json2decl_array(j[1]), json2value(j[2])));
  }
  else if (type == "var") {
    assert (j.array_items().size() == 3);
    assert (j[1].is_string());
    return shared_ptr<Value>(new Var(string_to_iden(j[1].string_value()), json2sort(j[2])));
  }
  else if (type == "const") {
    assert (j.array_items().size() == 3);
    assert (j[1].is_string());
    return shared_ptr<Value>(new Const(string_to_iden(j[1].string_value()), json2sort(j[2])));
  }
  else if (type == "implies") {
    assert (j.array_items().size() == 3);
    return shared_ptr<Value>(new Implies(json2value(j[1]), json2value(j[2])));
  }
  else if (type == "eq") {
    assert (j.array_items().size() == 3);
    return shared_ptr<Value>(new Eq(json2value(j[1]), json2value(j[2])));
  }
  else if (type == "not") {
    assert (j.array_items().size() == 2);
    return shared_ptr<Value>(new Not(json2value(j[1])));
  }
  else if (type == "apply") {
    assert (j.array_items().size() == 3);
    return shared_ptr<Value>(new Apply(json2value(j[1]), json2value_array(j[2])));
  }
  else if (type == "and") {
    assert (j.array_items().size() == 2);
    return shared_ptr<Value>(new And(json2value_array(j[1])));
  }
  else if (type == "or") {
    assert (j.array_items().size() == 2);
    return shared_ptr<Value>(new Or(json2value_array(j[1])));
  }
  else if (type == "__wild") {
    assert (j.array_items().size() == 1);
    return shared_ptr<Value>(new TemplateHole());
  }
  else {
    printf("value type: %s\n", type.c_str());
    assert(false && "unrecognized Value type");
  }
}

shared_ptr<Value> Value::from_json(Json j) {
  return json2value(j);
}

shared_ptr<Sort> json2sort(Json j) {
  assert(j.is_array());
  assert(j.array_items().size() >= 1);
  assert(j[0].is_string());
  string type = j[0].string_value();
  if (type == "booleanSort") {
    return shared_ptr<Sort>(new BooleanSort());
  }
  else if (type == "uninterpretedSort") {
    assert (j.array_items().size() == 2);
    return shared_ptr<Sort>(new UninterpretedSort(j[1].string_value()));
  }
  else if (type == "functionSort") {
    return shared_ptr<FunctionSort>(
      new FunctionSort(json2sort_array(j[1]), json2sort(j[2])));
  }
  else {
    assert(false && "unrecognized Sort type");
  }
}

shared_ptr<Action> json2action(Json j) {
  assert(j.is_array());
  assert(j.array_items().size() >= 1);
  assert(j[0].is_string());
  string type = j[0].string_value();
  if (type == "localAction") {
    assert(j.array_items().size() == 3);
    return shared_ptr<Action>(new LocalAction(json2decl_array(j[1]), json2action(j[2])));
  }
  else if (type == "sequence") {
    assert(j.array_items().size() == 2);
    return shared_ptr<Action>(new SequenceAction(json2action_array(j[1])));
  }
  else if (type == "choice") {
    assert(j.array_items().size() == 2);
    return shared_ptr<Action>(new ChoiceAction(json2action_array(j[1])));
  }
  else if (type == "assume") {
    assert(j.array_items().size() == 2);
    return shared_ptr<Action>(new Assume(json2value(j[1])));
  }
  else if (type == "assign") {
    assert(j.array_items().size() == 3);
    return shared_ptr<Action>(new Assign(json2value(j[1]), json2value(j[2])));
  }
  else if (type == "havoc") {
    assert(j.array_items().size() == 2);
    return shared_ptr<Action>(new Havoc(json2value(j[1])));
  }
  else if (type == "if") {
    assert(j.array_items().size() == 3);
    return shared_ptr<Action>(new If(json2value(j[1]), json2action(j[2])));
  }
  else if (type == "ifelse") {
    assert(j.array_items().size() == 4);
    return shared_ptr<Action>(new IfElse(
        json2value(j[1]), json2action(j[2]), json2action(j[3])));
  }
  else {
    assert(false && "unrecognized Action type");
  }
}

vector<Json> sort_array_2_json(vector<lsort> const& sorts) {
  vector<Json> res;
  for (lsort s : sorts) {
    res.push_back(s->to_json());
  }
  return res;
}

vector<Json> value_array_2_json(vector<value> const& values) {
  vector<Json> res;
  for (value v : values) {
    res.push_back(v->to_json());
  }
  return res;
}

vector<Json> decl_array_2_json(vector<VarDecl> const& decls) {
  vector<Json> res;
  for (VarDecl const& decl : decls) {
    res.push_back(Json({
        Json("var"),
        Json(iden_to_string(decl.name)),
        decl.sort->to_json()
    }));
  }
  return res;
}

Json BooleanSort::to_json() const {
  return Json(vector<Json>{Json("booleanSort")});
}

Json UninterpretedSort::to_json() const {
  return Json(vector<Json>{Json("uninterpretedSort"), Json(name)});
}

Json FunctionSort::to_json() const {
  return Json(vector<Json>{Json("functionSort"), sort_array_2_json(domain), range->to_json()});
}

Json Forall::to_json() const {
  return Json(vector<Json>{Json("forall"), decl_array_2_json(decls), body->to_json()});
}

Json NearlyForall::to_json() const {
  return Json(vector<Json>{Json("nearlyforall"), decl_array_2_json(decls), body->to_json()});
}

Json Exists::to_json() const {
  return Json(vector<Json>{Json("exists"), decl_array_2_json(decls), body->to_json()});
}

Json Var::to_json() const {
  return Json(vector<Json>{Json("var"), Json(iden_to_string(name)), sort->to_json()});
}

Json Const::to_json() const {
  return Json(vector<Json>{Json("const"), Json(iden_to_string(name)), sort->to_json()});
}

Json Implies::to_json() const {
  return Json(vector<Json>{Json("implies"), left->to_json(), right->to_json()});
}

Json IfThenElse::to_json() const {
  return Json(vector<Json>{Json("ite"), cond->to_json(), then_value->to_json(), else_value->to_json()});
}

Json Eq::to_json() const {
  return Json(vector<Json>{Json("eq"), left->to_json(), right->to_json()});
}

Json Not::to_json() const {
  return Json(vector<Json>{Json("not"), val->to_json()});
}

Json Apply::to_json() const {
  return Json(vector<Json>{Json("apply"), func->to_json(), value_array_2_json(args)});
}

Json And::to_json() const {
  return Json(vector<Json>{Json("and"), value_array_2_json(args)});
}

Json Or::to_json() const {
  return Json(vector<Json>{Json("or"), value_array_2_json(args)});
}

Json TemplateHole::to_json() const {
  return Json(vector<Json>{Json("__wild")});
}

string BooleanSort::to_string() const {
  return "bool";
}

string UninterpretedSort::to_string() const {
  return name;
}

string FunctionSort::to_string() const {
  string res = "(";
  for (int i = 0; i < (int)domain.size(); i++) {
    if (i != 0) res += ", ";
    res += domain[i]->to_string();
  }
  return res + " -> " + range->to_string();
}

string Forall::to_string() const {
  string res = "forall ";
  for (int i = 0; i < (int)decls.size(); i++) {
    if (i > 0) {
      res += ", ";
    }
    res += iden_to_string(decls[i].name);
  }
  res += " . (" + body->to_string() + ")";
  return res;
}

string NearlyForall::to_string() const {
  string res = "nearlyforall ";
  for (int i = 0; i < (int)decls.size(); i++) {
    if (i > 0) {
      res += ", ";
    }
    res += iden_to_string(decls[i].name);
  }
  res += " . (" + body->to_string() + ")";
  return res;
}


string Exists::to_string() const {
  string res = "exists ";
  for (int i = 0; i < (int)decls.size(); i++) {
    if (i > 0) {
      res += ", ";
    }
    res += iden_to_string(decls[i].name);
  }
  res += " . (" + body->to_string() + ")";
  return res;
}

string Var::to_string() const {
  return iden_to_string(name);
}

string Const::to_string() const {
  return iden_to_string(name);
}

string Eq::to_string() const {
  return "(" + left->to_string() + ") = (" + right->to_string() + ")";
}

string Not::to_string() const {
  if (Eq* e = dynamic_cast<Eq*>(val.get())) {
    return "(" + e->left->to_string() + ") ~= (" + e->right->to_string() + ")";
  } else {
    return "~(" + val->to_string() + ")";
  }
}

string Implies::to_string() const {
  return "(" + left->to_string() + ") -> (" + right->to_string() + ")";
}

string And::to_string() const {
  if (args.size() == 0) return "true";
  string res = "";
  if (args.size() <= 1) res += "AND[";
  for (int i = 0; i < (int)args.size(); i++) {
    if (i > 0) {
      res += " & ";
    }
    res += "(" + args[i]->to_string() + ")";
  }
  if (args.size() <= 1) res += "]";
  return res;
}

string Or::to_string() const {
  if (args.size() == 0) return "false";
  string res = "";
  if (args.size() <= 1) res += "OR[";
  for (int i = 0; i < (int)args.size(); i++) {
    if (i > 0) {
      res += " | ";
    }
    res += "(" + args[i]->to_string() + ")";
  }
  if (args.size() <= 1) res += "]";
  return res;
}

string IfThenElse::to_string() const {
  return cond->to_string() + " ? " + then_value->to_string() + " : " + else_value->to_string();
}

string Apply::to_string() const {
  string res = func->to_string() + "(";
  for (int i = 0; i < (int)args.size(); i++) {
    if (i > 0) {
      res += ", ";
    }
    res += args[i]->to_string();
  }
  return res + ")";
}

string TemplateHole::to_string() const {
  return "WILD";
}

lsort bsort = s_bool();

lsort Forall::get_sort() const { return bsort; }
lsort NearlyForall::get_sort() const { return bsort; }
lsort Exists::get_sort() const { return bsort; }
lsort Var::get_sort() const { return sort; }
lsort Const::get_sort() const { return sort; }
lsort IfThenElse::get_sort() const { return then_value->get_sort(); }
lsort Eq::get_sort() const { return bsort; }
lsort Not::get_sort() const { return bsort; }
lsort Implies::get_sort() const { return bsort; }
lsort Apply::get_sort() const {
  Const* f = dynamic_cast<Const*>(func.get());
  assert(f != NULL);
  return f->sort->get_range_as_function();
}
lsort And::get_sort() const { return bsort; }
lsort Or::get_sort() const { return bsort; }
lsort TemplateHole::get_sort() const { assert(false); }

value Forall::subst(iden x, value e) const {
  return v_forall(decls, body->subst(x, e)); 
}

value NearlyForall::subst(iden x, value e) const {
  return v_nearlyforall(decls, body->subst(x, e)); 
}

value Exists::subst(iden x, value e) const {
  return v_exists(decls, body->subst(x, e)); 
}

value Var::subst(iden x, value e) const {
  if (x == name) {
    return e;
  } else {
    return v_var(name, sort);
  }
}

value Const::subst(iden x, value e) const {
  return v_const(name, sort);
}

value Eq::subst(iden x, value e) const {
  return v_eq(left->subst(x, e), right->subst(x, e));
}

value Not::subst(iden x, value e) const {
  return v_not(this->val->subst(x, e));
}

value Implies::subst(iden x, value e) const {
  return v_implies(left->subst(x, e), right->subst(x, e));
}

value IfThenElse::subst(iden x, value e) const {
  return v_if_then_else(cond->subst(x, e), then_value->subst(x, e), else_value->subst(x, e));
}

value Apply::subst(iden x, value e) const {
  vector<value> new_args;
  for (value const& arg : args) {
    new_args.push_back(arg->subst(x, e));
  }
  return v_apply(func->subst(x, e), move(new_args));
}

value And::subst(iden x, value e) const {
  vector<value> new_args;
  for (value const& arg : args) {
    new_args.push_back(arg->subst(x, e));
  }
  return v_and(move(new_args));
}

value Or::subst(iden x, value e) const {
  vector<value> new_args;
  for (value const& arg : args) {
    new_args.push_back(arg->subst(x, e));
  }
  return v_or(move(new_args));
}

value TemplateHole::subst(iden x, value e) const {
  return v_template_hole();
}

value Forall::subst_fun(iden func, vector<VarDecl> const& d, value e) const {
  return v_forall(decls, body->subst_fun(func, d, e)); 
}

value NearlyForall::subst_fun(iden func, vector<VarDecl> const& d, value e) const {
  return v_nearlyforall(decls, body->subst_fun(func, d, e)); 
}

value Exists::subst_fun(iden func, vector<VarDecl> const& d, value e) const {
  return v_exists(decls, body->subst_fun(func, d, e)); 
}

value Var::subst_fun(iden func, vector<VarDecl> const& d, value e) const {
  return v_var(name, sort);
}

value Const::subst_fun(iden func, vector<VarDecl> const& d, value e) const {
  assert(name != func && "not sure if this will happen");
  return v_const(name, sort);
}

value Eq::subst_fun(iden func, vector<VarDecl> const& d, value e) const {
  return v_eq(
    left->subst_fun(func, d, e),
    right->subst_fun(func, d, e));
}

value Not::subst_fun(iden func, vector<VarDecl> const& d, value e) const {
  return v_not(
    val->subst_fun(func, d, e));
}

value Implies::subst_fun(iden func, vector<VarDecl> const& d, value e) const {
  return v_implies(
    left->subst_fun(func, d, e),
    right->subst_fun(func, d, e));
}

value Apply::subst_fun(iden f, vector<VarDecl> const& d, value e) const {
  vector<value> new_args;
  for (value v : args) {
    new_args.push_back(v->subst_fun(f, d, e));
  }

  Const* c = dynamic_cast<Const*>(func.get());
  assert (c != NULL);
  if (c->name == f) {
    value res = e;
    assert (d.size() == args.size());
    for (int i = 0; i < (int)d.size(); i++) {
      res = res->subst(d[i].name, args[i]);
    }
    return res;
  } else {
    return v_apply(func, new_args);
  }
}

value And::subst_fun(iden func, vector<VarDecl> const& d, value e) const {
  vector<value> new_args;
  for (value v : args) {
    new_args.push_back(v->subst_fun(func, d, e));
  }
  return v_and(new_args);
}

value Or::subst_fun(iden func, vector<VarDecl> const& d, value e) const {
  vector<value> new_args;
  for (value v : args) {
    new_args.push_back(v->subst_fun(func, d, e));
  }
  return v_or(new_args);
}

value IfThenElse::subst_fun(iden func, vector<VarDecl> const& d, value e) const {
  return v_if_then_else(
    cond->subst_fun(func, d, e),
    then_value->subst_fun(func, d, e),
    else_value->subst_fun(func, d, e));
}

value TemplateHole::subst_fun(iden func, vector<VarDecl> const& d, value e) const {
  assert(false);
}


value Forall::negate() const {
  return v_exists(decls, body->negate());
}

value NearlyForall::negate() const {
  assert(false && "NearlyForall::negate() not implemented");
}

value Exists::negate() const {
  return v_forall(decls, body->negate());
}

value Var::negate() const {
  return v_not(v_var(name, sort));
}

value Const::negate() const {
  return v_not(v_const(name, sort));
}

value Eq::negate() const {
  return v_not(v_eq(left, right));
}

value Not::negate() const {
  return val;
}

value Implies::negate() const {
  return v_and({left, right->negate()});
}

value IfThenElse::negate() const {
  return v_if_then_else(cond, then_value->negate(), else_value->negate());
}

value Apply::negate() const {
  return v_not(v_apply(func, args));
}

value And::negate() const {
  vector<value> new_args;
  for (value const& arg : args) {
    new_args.push_back(arg->negate());
  }
  return v_or(move(new_args));
}

value Or::negate() const {
  vector<value> new_args;
  for (value const& arg : args) {
    new_args.push_back(arg->negate());
  }
  return v_and(move(new_args));
}

value TemplateHole::negate() const {
  return v_not(v_template_hole());
}

bool Forall::uses_var(iden name) const { return body->uses_var(name); }
bool Exists::uses_var(iden name) const { return body->uses_var(name); }
bool NearlyForall::uses_var(iden name) const { return body->uses_var(name); }
bool Var::uses_var(iden name) const { return this->name == name; }
bool Const::uses_var(iden name) const { return false; }
bool Not::uses_var(iden name) const { return val->uses_var(name); }
bool Eq::uses_var(iden name) const { return left->uses_var(name) || right->uses_var(name); }
bool Implies::uses_var(iden name) const { return left->uses_var(name) || right->uses_var(name); }
bool IfThenElse::uses_var(iden name) const { return cond->uses_var(name) || then_value->uses_var(name) || else_value->uses_var(name); }
bool And::uses_var(iden name) const {
  for (value arg : args) {
    if (arg->uses_var(name)) return true;
  }
  return false;
}
bool Or::uses_var(iden name) const {
  for (value arg : args) {
    if (arg->uses_var(name)) return true;
  }
  return false;
}
bool Apply::uses_var(iden name) const {
  if (func->uses_var(name)) return true;
  for (value arg : args) {
    if (arg->uses_var(name)) return true;
  }
  return false;
}
bool TemplateHole::uses_var(iden name) const { assert(false); }

value Forall::simplify() const {
  return v_forall(decls, body->simplify());
}

value NearlyForall::simplify() const {
  return v_nearlyforall(decls, body->simplify());
}

value Exists::simplify() const {
  return v_exists(decls, body->simplify());
}

value Var::simplify() const {
  return v_var(name, sort);
}

value Const::simplify() const {
  return v_const(name, sort);
}

value Eq::simplify() const {
  value l = left->simplify();
  value r = right->simplify();
  if (values_equal(l, r)) {
    return v_true();
  } else {
    return v_eq(l, r);
  }
}

value Not::simplify() const {
  return val->simplify()->negate();
}

value Implies::simplify() const {
  return v_or({v_not(left), right})->simplify();
}

bool is_const_true(value v) {
  And* a = dynamic_cast<And*>(v.get());
  return a != NULL && a->args.size() == 0;
}

bool is_const_false(value v) {
  Or* a = dynamic_cast<Or*>(v.get());
  return a != NULL && a->args.size() == 0;
}

template <typename T>
void extend(vector<T> & a, vector<T> const& b) {
  for (T const& t : b) {
    a.push_back(t);
  }
}

vector<value> remove_redundant(vector<value> const& vs) {
  vector<value> t;
  for (value v : vs) {
    bool redun = false;
    for (value w : t) {
      if (values_equal(v, w)) {
        redun = true;
        break;
      }
    }
    if (!redun) {
      t.push_back(v);
    }
  }
  return t;
}

value And::simplify() const {
  vector<value> a;
  for (value v : args) {
    v = v->simplify();
    if (And* inner = dynamic_cast<And*>(v.get())) {
      extend(a, inner->args);
    }
    else if (is_const_false(v)) {
      return v_false();
    }
    else {
      a.push_back(v);
    }
  }

  a = remove_redundant(a);

  value res = a.size() == 1 ? a[0] : v_and(a);
  return res;
}

value Or::simplify() const {
  vector<value> a;
  for (value v : args) {
    v = v->simplify();
    if (Or* inner = dynamic_cast<Or*>(v.get())) {
      extend(a, inner->args);
    }
    else if (is_const_true(v)) {
      return v_true();
    }
    else {
      a.push_back(v);
    }
  }

  a = remove_redundant(a);

  value res = a.size() == 1 ? a[0] : v_or(a);
  return res;
}

value IfThenElse::simplify() const {
  value c = cond->simplify();
  value a = then_value->simplify();
  value b = else_value->simplify();

  if (is_const_true(c)) return a;
  if (is_const_false(c)) return b;

  if (is_const_true(a)) {
    if (is_const_true(b)) {
      return v_true();
    } else if (is_const_false(b)) {
      return c;
    } else {
      return v_or({c, b});
    }
  }
  else if (is_const_false(a)) {
    if (is_const_true(b)) {
      return c->negate();
    } else if (is_const_false(b)) {
      return v_false();
    } else {
      return v_and({c->negate(), b});
    }
  }
  else {
    if (is_const_true(b)) {
      return v_or({c->negate(), a});
    } else if (is_const_false(b)) {
      return v_and({c, a});
    } else {
      return v_if_then_else(c, a, b);
    }
  }
}

value Apply::simplify() const {
  vector<value> a;
  for (value v : args) {
    a.push_back(v->simplify());
  }
  return v_apply(func, a);
}

value TemplateHole::simplify() const {
  return v_template_hole();
}

uint32_t uid = 0;
uint32_t new_var_id() {
  return (uid++);
}

value Forall::uniquify_vars(map<iden, iden> const& m) const {
  map<iden, iden> new_m = m;
  vector<VarDecl> new_decls;
  for (VarDecl const& decl : this->decls) {
    iden new_name = new_var_id();
    new_m[decl.name] = new_name;
    new_decls.push_back(VarDecl(new_name, decl.sort));
  }
  return v_forall(new_decls, body->uniquify_vars(new_m));
}

value NearlyForall::uniquify_vars(map<iden, iden> const& m) const {
  assert(false && "NearlyForall::uniquify_vars not implemented");
}

value Exists::uniquify_vars(map<iden, iden> const& m) const {
  map<iden, iden> new_m = m;
  vector<VarDecl> new_decls;
  for (VarDecl const& decl : this->decls) {
    iden new_name = new_var_id();
    new_m[decl.name] = new_name;
    new_decls.push_back(VarDecl(new_name, decl.sort));
  }
  return v_forall(new_decls, body->uniquify_vars(new_m));
}

value Var::uniquify_vars(map<iden, iden> const& m) const {
  auto iter = m.find(this->name);
  assert(iter != m.end());
  return v_var(iter->second, this->sort);
}

value Const::uniquify_vars(map<iden, iden> const& m) const {
  return v_const(name, sort);
}

value Eq::uniquify_vars(map<iden, iden> const& m) const {
  return v_eq(left->uniquify_vars(m), right->uniquify_vars(m));
}

value Not::uniquify_vars(map<iden, iden> const& m) const {
  return v_not(this->val->uniquify_vars(m));
}

value Implies::uniquify_vars(map<iden, iden> const& m) const {
  return v_implies(left->uniquify_vars(m), right->uniquify_vars(m));
}

value IfThenElse::uniquify_vars(map<iden, iden> const& m) const {
  return v_if_then_else(cond->uniquify_vars(m), then_value->uniquify_vars(m), else_value->uniquify_vars(m));
}

value Apply::uniquify_vars(map<iden, iden> const& m) const {
  vector<value> new_args;
  for (value const& arg : args) {
    new_args.push_back(arg->uniquify_vars(m));
  }
  return v_apply(func->uniquify_vars(m), move(new_args));
}

value And::uniquify_vars(map<iden, iden> const& m) const {
  vector<value> new_args;
  for (value const& arg : args) {
    new_args.push_back(arg->uniquify_vars(m));
  }
  return v_and(move(new_args));
}

value Or::uniquify_vars(map<iden, iden> const& m) const {
  vector<value> new_args;
  for (value const& arg : args) {
    new_args.push_back(arg->uniquify_vars(m));
  }
  return v_or(move(new_args));
}

value TemplateHole::uniquify_vars(map<iden, iden> const& m) const {
  return v_template_hole();
}

void sort_decls(vector<VarDecl>& decls) {
  sort(decls.begin(), decls.end(), [](VarDecl const& a, VarDecl const& b) {
    string a_name, b_name;
    if (dynamic_cast<BooleanSort*>(a.sort.get())) {
      a_name = "";
    } else if (auto v = dynamic_cast<UninterpretedSort*>(a.sort.get())) {
      a_name = v->name;
    }
    if (dynamic_cast<BooleanSort*>(b.sort.get())) {
      b_name = "";
    } else if (auto v = dynamic_cast<UninterpretedSort*>(b.sort.get())) {
      b_name = v->name;
    }
    return a_name < b_name;
  });
}

value Forall::structurally_normalize_() const {
  value b = this->body->structurally_normalize_();
  if (Forall* inner = dynamic_cast<Forall*>(b.get())) {
    vector<VarDecl> new_decls = this->decls;
    extend(new_decls, inner->decls);
    sort_decls(new_decls);
    return v_forall(new_decls, inner->body);
  } else {
    return v_forall(this->decls, b);
  }
}

value NearlyForall::structurally_normalize_() const {
  assert (false && "NearlyForall::structurally_normalize_ not implemented");
}


value Exists::structurally_normalize_() const {
  value b = this->body->structurally_normalize_();
  if (Exists* inner = dynamic_cast<Exists*>(b.get())) {
    vector<VarDecl> new_decls = this->decls;
    extend(new_decls, inner->decls);
    sort_decls(new_decls);
    return v_exists(new_decls, inner->body);
  } else {
    return v_exists(this->decls, b);
  }
}

value Var::structurally_normalize_() const {
  return v_var(name, sort);
}

value Const::structurally_normalize_() const {
  return v_const(name, sort);
}

value Eq::structurally_normalize_() const {
  return v_eq(left->structurally_normalize_(), right->structurally_normalize_());
}

value Not::structurally_normalize_() const {
  return val->structurally_normalize_()->negate();
}

value Implies::structurally_normalize_() const {
  return v_or({v_not(left), right})->structurally_normalize_();
}

value Apply::structurally_normalize_() const {
  vector<value> new_args;
  for (value const& arg : args) {
    new_args.push_back(arg->structurally_normalize_());
  }
  return v_apply(func->structurally_normalize_(), move(new_args));
}

value structurally_normalize_and_or_or(Value const * the_value) {
  And const * the_and = dynamic_cast<And const*>(the_value);
  Or const * the_or = dynamic_cast<Or const*>(the_value);

  vector<value> args = the_and ? the_and->args : the_or->args;

  vector<vector<VarDecl>> forall_decl_lists;
  vector<vector<VarDecl>> exists_decl_lists;

  while (true) {
    vector<VarDecl> forall_decls;
    vector<VarDecl> exists_decls;
    vector<value> new_args;
    bool found = false;
    for (value arg : args) {
      if (Forall* forall = dynamic_cast<Forall*>(arg.get())) {
        extend(forall_decls, forall->decls);
        arg = forall->body;
        found = true;
      }
      if (Exists* exists = dynamic_cast<Exists*>(arg.get())) {
        extend(exists_decls, exists->decls);
        arg = exists->body;
        found = true;
      }
      new_args.push_back(arg);
    }
    forall_decl_lists.push_back(move(forall_decls));
    exists_decl_lists.push_back(move(exists_decls));
    args = move(new_args);
    if (!found) {
      break;
    }
  }

  vector<value> new_args;
  for (value arg : args) {
    And* sub_and;
    Or* sub_or;
    if (the_and && (sub_and = dynamic_cast<And*>(arg.get()))) {
      extend(new_args, sub_and->args);
    } else if (the_or && (sub_or = dynamic_cast<Or*>(arg.get()))) {
      extend(new_args, sub_or->args);
    } else {
      new_args.push_back(arg);
    }
  }

  value res = the_and ? v_and(new_args) : v_or(new_args);
  for (int i = forall_decl_lists.size() - 1; i >= 0; i--) {
    vector<VarDecl> const& f_decls = forall_decl_lists[i];
    vector<VarDecl> const& e_decls = exists_decl_lists[i];
    if (e_decls.size() > 0) {
      res = v_exists(e_decls, res);
    }
    if (f_decls.size() > 0) {
      res = v_exists(f_decls, res);
    }
  }

  return res;
}

value And::structurally_normalize_() const {
  return structurally_normalize_and_or_or(this);
}

value Or::structurally_normalize_() const {
  return structurally_normalize_and_or_or(this);
}

value IfThenElse::structurally_normalize_() const {
  assert(false && "unimplemented");
}

value TemplateHole::structurally_normalize_() const {
  return v_template_hole();
}

struct ScopeState {
  vector<VarDecl> decls;
};

bool lt_value(value a_, value b_, ScopeState const& ss_a, ScopeState const& ss_b);

int cmp_sort(lsort a, lsort b) {
  if (dynamic_cast<BooleanSort*>(a.get())) {
    if (dynamic_cast<BooleanSort*>(b.get())) {
      return 0;
    }
    else if (dynamic_cast<UninterpretedSort*>(b.get())) {
      return -1;
    }
    else {
      assert(false);
    }
  }
  else if (UninterpretedSort* usort = dynamic_cast<UninterpretedSort*>(b.get())) {
    if (dynamic_cast<BooleanSort*>(a.get())) {
      return 1;
    }
    else if (UninterpretedSort* usort2 = dynamic_cast<UninterpretedSort*>(b.get())) {
      string const& a_name = usort->name;
      string const& b_name = usort2->name;
      if (a_name < b_name) return -1;
      if (a_name > b_name) return 1;
      return 0;
    }
    else {
      assert(false);
    }
  }
  else {
    assert(false);
  }
}

bool eq_sort(lsort a, lsort b) {
  return cmp_sort(a, b) == 0;
}

vector<iden> get_front_quantifier_order(value body, vector<VarDecl> const& decls,
    set<iden> const& vars_used);

value forall_exists_normalize_symmetries(
    vector<VarDecl> const& decls,
    set<iden> const& vars_used,
    value body,
    bool is_forall,
    int idx,
    ScopeState const& ss)
{
  if (idx == (int)decls.size()) {
    return body->normalize_symmetries(ss, vars_used);
  }

  int idx_end = idx + 1;
  while (idx_end < (int)decls.size() && eq_sort(decls[idx].sort, decls[idx_end].sort)) {
    idx_end++;
  }

  vector<iden> front_vars = get_front_quantifier_order(body, decls, vars_used);

  vector<int> perm;
  vector<int> perm_mid;
  vector<int> perm_back;
  for (iden name : front_vars) {
    int this_idx = -1;
    for (int i = idx; i < idx_end; i++) {
      if (decls[i].name == name) {
        this_idx = i;
        break;
      }
    }
    assert(this_idx != -1);
    perm.push_back(this_idx);
  }
  for (int i = idx; i < idx_end; i++) {
    if (vars_used.find(decls[i].name) == vars_used.end()) {
      perm_back.push_back(i);
    } else {
      bool in_perm = false;
      for (int j : perm) {
        if (j == i) {
          in_perm = true;
          break;
        }
      }
      if (!in_perm) {
        perm_mid.push_back(i);
      }
    }
  }

  // We know what's at the front and back. Iterate over permutations
  // of the middle segment.
  int perm_start = perm.size();
  int perm_end = perm.size() + perm_mid.size();
  extend(perm, perm_mid);
  extend(perm, perm_back);
  /*
  for (int p : perm) {
    printf("%d ", p);
  }
  printf("   :   %d -- %d\n\n", perm_start, perm_end);
  */

  value smallest;
  vector<VarDecl> smallest_decls;
  ScopeState ss_smallest;
  do {
    ScopeState ss_ = ss;
    for (int i : perm) {
      ss_.decls.push_back(decls[i]);
    }
    value inner = forall_exists_normalize_symmetries(decls, vars_used, body, is_forall, idx_end, ss_);
    if (smallest == nullptr || lt_value(inner, smallest, ss_, ss_smallest)) {
      smallest = inner;
      ss_smallest = move(ss_);

      smallest_decls.clear();
      for (int i : perm) {
        smallest_decls.push_back(decls[i]);
      }
    }
  } while (next_permutation(perm.begin() + perm_start, perm.begin() + perm_end));

  if (idx_end < (int)decls.size()) {
    if (Forall* inner = dynamic_cast<Forall*>(smallest.get())) {
      smallest = inner->body;
      extend(smallest_decls, inner->decls);
    }
    else if (Exists* inner = dynamic_cast<Exists*>(smallest.get())) {
      smallest = inner->body;
      extend(smallest_decls, inner->decls);
    }
    else {
      assert(false);
    }
  }

  value res = is_forall ? v_forall(smallest_decls, smallest)
                        : v_exists(smallest_decls, smallest);
  return res;
}

value Forall::normalize_symmetries(ScopeState const& ss, set<iden> const& vars_used) const {
  //printf("%s\n", this->to_string().c_str());
  return forall_exists_normalize_symmetries(this->decls, vars_used, this->body, true, 0, ss);
}

value NearlyForall::normalize_symmetries(ScopeState const& ss, set<iden> const& vars_used) const {
  assert(false && "NearlyForall::normalize_symmetries not implemented");
}

value Exists::normalize_symmetries(ScopeState const& ss, set<iden> const& vars_used) const {
  return forall_exists_normalize_symmetries(this->decls, vars_used, this->body, false, 0, ss);
}

value Var::normalize_symmetries(ScopeState const& ss, set<iden> const& vars_used) const {
  return v_var(name, sort);
}

value Const::normalize_symmetries(ScopeState const& ss, set<iden> const& vars_used) const {
  return v_const(name, sort);
}

value IfThenElse::normalize_symmetries(ScopeState const& ss, set<iden> const& vars_used) const {
  assert(false);
}

value Eq::normalize_symmetries(ScopeState const& ss, set<iden> const& vars_used) const {
  value a = left->normalize_symmetries(ss, vars_used);
  value b = right->normalize_symmetries(ss, vars_used);
  return lt_value(a, b, ss, ss) ? v_eq(a, b) : v_eq(b, a);
}

value Not::normalize_symmetries(ScopeState const& ss, set<iden> const& vars_used) const {
  return v_not(val->normalize_symmetries(ss, vars_used));
}

value Implies::normalize_symmetries(ScopeState const& ss, set<iden> const& vars_used) const {
  assert(false && "implies should have been replaced by |");
}

value Apply::normalize_symmetries(ScopeState const& ss, set<iden> const& vars_used) const {
  vector<value> new_args;
  for (value arg : args) {
    new_args.push_back(arg->normalize_symmetries(ss, vars_used));
  }
  return v_apply(func->normalize_symmetries(ss, vars_used), move(new_args));
}

void sort_values(ScopeState const& ss, vector<value> & values) {
  sort(values.begin(), values.end(), [&ss](value const& a, value const& b) {
    return lt_value(a, b, ss, ss);
  });
}

value And::normalize_symmetries(ScopeState const& ss, set<iden> const& vars_used) const {
  vector<value> new_args;
  for (value arg : args) {
    new_args.push_back(arg->normalize_symmetries(ss, vars_used));
  }
  sort_values(ss, new_args);
  return v_and(new_args);
}

value Or::normalize_symmetries(ScopeState const& ss, set<iden> const& vars_used) const {
  vector<value> new_args;
  for (value arg : args) {
    new_args.push_back(arg->normalize_symmetries(ss, vars_used));
  }
  sort_values(ss, new_args);
  return v_or(new_args);
}

value TemplateHole::normalize_symmetries(ScopeState const& ss, set<iden> const& vars_used) const {
  return v_template_hole();
}

int cmp_expr(value a_, value b_, ScopeState const& ss_a, ScopeState const& ss_b) {
  int a_id = a_->kind_id();
  int b_id = b_->kind_id();
  if (a_id != b_id) return a_id < b_id ? -1 : 1;

  if (Forall* a = dynamic_cast<Forall*>(a_.get())) {
    Forall* b = dynamic_cast<Forall*>(b_.get());
    assert(b != NULL);
  
    if (a->decls.size() < b->decls.size()) return -1;
    if (a->decls.size() > b->decls.size()) return 1;
    ScopeState ss_a_new = ss_a;
    ScopeState ss_b_new = ss_b;
    for (int i = 0; i < (int)a->decls.size(); i++) {
      if (int c = cmp_sort(a->decls[i].sort, b->decls[i].sort)) {
        return c;
      }
      ss_a_new.decls.push_back(a->decls[i]);
      ss_b_new.decls.push_back(b->decls[i]);
    }
    return cmp_expr(a->body, b->body, ss_a_new, ss_b_new);
  }

  if (Exists* a = dynamic_cast<Exists*>(a_.get())) {
    Exists* b = dynamic_cast<Exists*>(b_.get());
    assert(b != NULL);
  
    if (a->decls.size() < b->decls.size()) return -1;
    if (a->decls.size() > b->decls.size()) return 1;
    ScopeState ss_a_new = ss_a;
    ScopeState ss_b_new = ss_b;
    for (int i = 0; i < (int)a->decls.size(); i++) {
      if (int c = cmp_sort(a->decls[i].sort, b->decls[i].sort)) {
        return c;
      }
      ss_a_new.decls.push_back(a->decls[i]);
      ss_b_new.decls.push_back(b->decls[i]);
    }
    return cmp_expr(a->body, b->body, ss_a_new, ss_b_new);
  }

  if (Var* a = dynamic_cast<Var*>(a_.get())) {
    Var* b = dynamic_cast<Var*>(b_.get());
    assert(b != NULL);

    int a_idx = -1, b_idx = -1;
    for (int i = 0; i < (int)ss_a.decls.size(); i++) {
      if (ss_a.decls[i].name == a->name) {
        a_idx = i;
        break;
      }
    }
    for (int i = 0; i < (int)ss_b.decls.size(); i++) {
      if (ss_b.decls[i].name == b->name) {
        b_idx = i;
        break;
      }
    }

    if (a_idx == -1) {
      if (b_idx == -1) {
        // free variables: compare by name
        string a_name = iden_to_string(a->name);
        string b_name = iden_to_string(b->name);
        return a_name < b_name ? -1 : (a_name == b_name ? 0 : 1);
      } else {
        // free var comes first
        return -1;
      }
    } else {
      if (b_idx == -1) {
        return 1;
      } else {
        // compare by index
        return a_idx < b_idx ? -1 : (a_idx == b_idx ? 0 : 1);
      }
    }
  }

  if (Const* a = dynamic_cast<Const*>(a_.get())) {
    Const* b = dynamic_cast<Const*>(b_.get());
    assert(b != NULL);

    return a->name < b->name ? -1 : (a->name == b->name ? 0 : 1);
  }

  if (Eq* a = dynamic_cast<Eq*>(a_.get())) {
    Eq* b = dynamic_cast<Eq*>(b_.get());
    assert(b != NULL);

    if (int c = cmp_expr(a->left, b->left, ss_a, ss_b)) return c;
    return cmp_expr(a->right, b->right, ss_a, ss_b);
  }

  if (Not* a = dynamic_cast<Not*>(a_.get())) {
    Not* b = dynamic_cast<Not*>(b_.get());
    assert(b != NULL);

    return cmp_expr(a->val, b->val, ss_a, ss_b);
  }

  if (Implies* a = dynamic_cast<Implies*>(a_.get())) {
    Implies* b = dynamic_cast<Implies*>(b_.get());
    assert(b != NULL);

    if (int c = cmp_expr(a->left, b->left, ss_a, ss_b)) return c;
    return cmp_expr(a->right, b->right, ss_a, ss_b);
  }

  if (Apply* a = dynamic_cast<Apply*>(a_.get())) {
    Apply* b = dynamic_cast<Apply*>(b_.get());
    assert(b != NULL);

    if (int c = cmp_expr(a->func, b->func, ss_a, ss_b)) return c;

    if (a->args.size() < b->args.size()) return -1;
    if (a->args.size() > b->args.size()) return 1;

    for (int i = 0; i < (int)a->args.size(); i++) {
      if (int c = cmp_expr(a->args[i], b->args[i], ss_a, ss_b)) {
        return c;
      }
    }

    return 0;
  }

  if (And* a = dynamic_cast<And*>(a_.get())) {
    And* b = dynamic_cast<And*>(b_.get());
    assert(b != NULL);

    if (a->args.size() < b->args.size()) return -1;
    if (a->args.size() > b->args.size()) return 1;

    for (int i = 0; i < (int)a->args.size(); i++) {
      if (int c = cmp_expr(a->args[i], b->args[i], ss_a, ss_b)) {
        return c;
      }
    }

    return 0;
  }

  if (Or* a = dynamic_cast<Or*>(a_.get())) {
    Or* b = dynamic_cast<Or*>(b_.get());
    assert(b != NULL);

    if (a->args.size() < b->args.size()) return -1;
    if (a->args.size() > b->args.size()) return 1;

    for (int i = 0; i < (int)a->args.size(); i++) {
      if (int c = cmp_expr(a->args[i], b->args[i], ss_a, ss_b)) {
        return c;
      }
    }

    return 0;
  }

  assert(false);
}

bool lt_value(value a_, value b_, ScopeState const& ss_a, ScopeState const& ss_b) {
  return cmp_expr(a_, b_, ss_a, ss_b) < 0;
}

bool lt_value(value a_, value b_) {
  ScopeState ss_a;
  ScopeState ss_b;
  return cmp_expr(a_, b_, ss_a, ss_b) < 0;
}

int cmp_expr_def(value a_, value b_) {
  int a_id = a_->kind_id();
  int b_id = b_->kind_id();
  if (a_id != b_id) return a_id < b_id ? -1 : 1;

  if (Forall* a = dynamic_cast<Forall*>(a_.get())) {
    Forall* b = dynamic_cast<Forall*>(b_.get());
    assert(b != NULL);
    return cmp_expr_def(a->body, b->body);
  }

  if (Exists* a = dynamic_cast<Exists*>(a_.get())) {
    Exists* b = dynamic_cast<Exists*>(b_.get());
    assert(b != NULL);
  
    return cmp_expr_def(a->body, b->body);
  }

  if (dynamic_cast<Var*>(a_.get())) {
    return 0;
  }

  if (Const* a = dynamic_cast<Const*>(a_.get())) {
    Const* b = dynamic_cast<Const*>(b_.get());
    assert(b != NULL);

    return a->name < b->name ? -1 : (a->name == b->name ? 0 : 1);
  }

  if (dynamic_cast<Eq*>(a_.get())) {
    Eq* b = dynamic_cast<Eq*>(b_.get());
    assert(b != NULL);

    return 0;
  }

  if (Not* a = dynamic_cast<Not*>(a_.get())) {
    Not* b = dynamic_cast<Not*>(b_.get());
    assert(b != NULL);

    return cmp_expr_def(a->val, b->val);
  }

  if (dynamic_cast<Implies*>(a_.get())) {
    assert(false);
  }

  if (Apply* a = dynamic_cast<Apply*>(a_.get())) {
    Apply* b = dynamic_cast<Apply*>(b_.get());
    assert(b != NULL);

    if (int c = cmp_expr_def(a->func, b->func)) return c;

    if (a->args.size() < b->args.size()) return -1;
    if (a->args.size() > b->args.size()) return 1;

    for (int i = 0; i < (int)a->args.size(); i++) {
      if (int c = cmp_expr_def(a->args[i], b->args[i])) {
        return c;
      }
    }

    return 0;
  }

  if (dynamic_cast<And*>(a_.get())) {
    return 0;
  }

  if (dynamic_cast<Or*>(a_.get())) {
    return 0;
  }

  assert(false);
}

bool get_certain_variable_order(
    value a_,
    vector<VarDecl> const& d,
    vector<iden> & res,
    int n)
{
  if (Forall* a = dynamic_cast<Forall*>(a_.get())) {
    return get_certain_variable_order(a->body, d, res, n);
  }

  else if (Exists* a = dynamic_cast<Exists*>(a_.get())) {
    return get_certain_variable_order(a->body, d, res, n);
  }

  else if (Var* a = dynamic_cast<Var*>(a_.get())) {
    for (int i = 0; i < (int)d.size(); i++) {
      if (d[i].name == a->name) {
        bool contains = false;
        for (int j = 0; j < (int)res.size(); j++) {
          if (res[j] == a->name) {
            contains = true;
            break;
          }
        }
        if (!contains) {
          res.push_back(a->name);
        }

        break;
      }
    }
    return true;
  }

  else if (dynamic_cast<Const*>(a_.get())) {
    return true;
  }

  else if (Eq* a = dynamic_cast<Eq*>(a_.get())) {
    int c = cmp_expr_def(a->left, a->right);
    if (c == -1) {
      if (!get_certain_variable_order(a->left, d, res, n)) return false;
      return get_certain_variable_order(a->right, d, res, n);
    }
    else if (c == 1) {
      if (!get_certain_variable_order(a->right, d, res, n)) return false;
      return get_certain_variable_order(a->left, d, res, n);
    }
    else {
      // If we don't know which order the == goes in, but there's only one
      // additional variable anyway, it's fine.
      int cur_size = res.size();
      bool okay = get_certain_variable_order(a->left, d, res, n);
      if (okay) {
        okay = get_certain_variable_order(a->right, d, res, n);
      }
      if (okay && (int)res.size() <= cur_size + 1) {
        return true;
      } else if (okay && (int)res.size() == n && cur_size == n - 2 &&
          dynamic_cast<Var*>(a->left.get()) && dynamic_cast<Var*>(a->right.get())) {
        // A=B case where A and B are the last two 
        // In this case, we learn nothing about the ordering from this term.
        res.resize(cur_size);
        return true;
      } else {
        res.resize(cur_size);
        return false;
      }
    }
  }

  if (Not* a = dynamic_cast<Not*>(a_.get())) {
    return get_certain_variable_order(a->val, d, res, n);
  }

  if (dynamic_cast<Implies*>(a_.get())) {
    assert(false);
  }

  if (Apply* a = dynamic_cast<Apply*>(a_.get())) {
    if (!get_certain_variable_order(a->func, d, res, n)) return false;
    for (value arg : a->args) {
      if (!get_certain_variable_order(arg, d, res, n)) return false;
    }
    return true;
  }

  if (And* a = dynamic_cast<And*>(a_.get())) {
    for (value arg : a->args) {
      if (!get_certain_variable_order(arg, d, res, n)) return false;
    }
    return true;
  }

  if (Or* a = dynamic_cast<Or*>(a_.get())) {
    for (value arg : a->args) {
      if (!get_certain_variable_order(arg, d, res, n)) return false;
    }
    return true;
  }

  assert(false);
}

vector<iden> get_front_quantifier_order(
    value body,
    vector<VarDecl> const& decls,
    set<iden> const& vars_used)
{
  while (true) {
    if (Forall* b = dynamic_cast<Forall*>(body.get())) {
      body = b->body;
    }
    else if (Exists* b = dynamic_cast<Exists*>(body.get())) {
      body = b->body;
    }
    else {
      break;
    }
  }

  vector<value> juncts;
  if (And* b = dynamic_cast<And*>(body.get())) {
    juncts = b->args;
  }
  else if (Or* b = dynamic_cast<Or*>(body.get())) {
    juncts = b->args;
  }
  else {
    juncts.push_back(body);
  }

  if (juncts.size() == 0) {
    vector<iden> res;
    return res;
  }

  sort(juncts.begin(), juncts.end(), [](value const& a, value const& b) {
    return cmp_expr_def(a, b) < 0;
  });

  int certain = 0;
  while (certain < (int)juncts.size() - 1 && cmp_expr_def(juncts[certain], juncts[certain+1]) < 0) {
    certain++;
  }
  if (certain == (int)juncts.size() - 1) {
    certain++;
  }

  vector<iden> used_decl_names;
  for (int i = 0; i < (int)decls.size(); i++) {
    if (vars_used.count(decls[i].name)) {
      used_decl_names.push_back(decls[i].name);
    }
  }

  vector<iden> certain_order;
  bool failed = false;
  for (int i = 0; i < certain; i++) {
    if (!get_certain_variable_order(juncts[i], decls, certain_order, used_decl_names.size())) {
      failed = true;
      break;
    }
  }
  if (!failed && certain == (int)juncts.size()) {
    for (iden name : used_decl_names) {
      bool used = false;
      for (iden s : certain_order) {
        if (s == name) {
          used = true;
          break;
        }
      }
      if (!used) {
        certain_order.push_back(name);
      }
    }
  }

  return certain_order;
}

value Forall::indexify_vars(map<iden, iden> const& m) const {
  map<iden, iden> new_m = m;
  vector<VarDecl> new_decls;
  for (VarDecl const& decl : this->decls) {
    iden new_name = new_m.size();
    new_m[decl.name] = new_name;
    new_decls.push_back(VarDecl(new_name, decl.sort));
  }
  return v_forall(new_decls, body->indexify_vars(new_m));
}

value NearlyForall::indexify_vars(map<iden, iden> const& m) const {
  assert(false && "NearlyForall::indexify_vars not implemented");
}

value Exists::indexify_vars(map<iden, iden> const& m) const {
  map<iden, iden> new_m = m;
  vector<VarDecl> new_decls;
  for (VarDecl const& decl : this->decls) {
    iden new_name = new_m.size();
    new_m[decl.name] = new_name;
    new_decls.push_back(VarDecl(new_name, decl.sort));
  }
  return v_forall(new_decls, body->indexify_vars(new_m));
}

value Var::indexify_vars(map<iden, iden> const& m) const {
  auto iter = m.find(this->name);
  assert(iter != m.end());
  return v_var(iter->second, this->sort);
}

value Const::indexify_vars(map<iden, iden> const& m) const {
  return v_const(name, sort);
}

value Eq::indexify_vars(map<iden, iden> const& m) const {
  return v_eq(left->indexify_vars(m), right->indexify_vars(m));
}

value Not::indexify_vars(map<iden, iden> const& m) const {
  return v_not(this->val->indexify_vars(m));
}

value Implies::indexify_vars(map<iden, iden> const& m) const {
  return v_implies(left->indexify_vars(m), right->indexify_vars(m));
}

value Apply::indexify_vars(map<iden, iden> const& m) const {
  vector<value> new_args;
  for (value const& arg : args) {
    new_args.push_back(arg->indexify_vars(m));
  }
  return v_apply(func->indexify_vars(m), move(new_args));
}

value And::indexify_vars(map<iden, iden> const& m) const {
  vector<value> new_args;
  for (value const& arg : args) {
    new_args.push_back(arg->indexify_vars(m));
  }
  return v_and(move(new_args));
}

value Or::indexify_vars(map<iden, iden> const& m) const {
  vector<value> new_args;
  for (value const& arg : args) {
    new_args.push_back(arg->indexify_vars(m));
  }
  return v_or(move(new_args));
}

value IfThenElse::indexify_vars(map<iden, iden> const& m) const {
  return v_if_then_else(cond->indexify_vars(m), then_value->indexify_vars(m), else_value->indexify_vars(m));
}

value TemplateHole::indexify_vars(map<iden, iden> const& m) const {
  return v_template_hole();
}

void Forall::get_used_vars(set<iden>& s) const {
  body->get_used_vars(s);
}
void NearlyForall::get_used_vars(set<iden>& s) const {
  body->get_used_vars(s);
}
void Exists::get_used_vars(set<iden>& s) const {
  body->get_used_vars(s);
}
void Var::get_used_vars(set<iden>& s) const {
  s.insert(name);
}
void Const::get_used_vars(set<iden>& s) const {
}
void Eq::get_used_vars(set<iden>& s) const {
  left->get_used_vars(s);
  right->get_used_vars(s);
}
void Not::get_used_vars(set<iden>& s) const {
  val->get_used_vars(s);
}
void Implies::get_used_vars(set<iden>& s) const {
  left->get_used_vars(s);
  right->get_used_vars(s);
}
void Apply::get_used_vars(set<iden>& s) const {
  func->get_used_vars(s);
  for (value arg : args) {
    arg->get_used_vars(s);
  }
}
void And::get_used_vars(set<iden>& s) const {
  for (value arg : args) {
    arg->get_used_vars(s);
  }
}
void Or::get_used_vars(set<iden>& s) const {
  for (value arg : args) {
    arg->get_used_vars(s);
  }
}
void IfThenElse::get_used_vars(set<iden>& s) const {
  cond->get_used_vars(s);
  then_value->get_used_vars(s);
  else_value->get_used_vars(s);
}

void TemplateHole::get_used_vars(set<iden>& s) const {
}

//int counter = 0;
//Benchmarking bench;

value Value::totally_normalize() const {

  ScopeState ss;

  //bench.start("structurally_normalize");
  value res = this->structurally_normalize();
  //bench.end();

  set<iden> vars_used;
  res->get_used_vars(vars_used);

  //bench.start("normalize_symmetries");
  res = res->normalize_symmetries(ss, vars_used);
  //bench.end();

  //bench.start("indexify_vars");
  res = res->indexify_vars({});
  //bench.end();

  /*
  counter++;
  if (counter % 1000 == 0) {
    printf("count = %d\n", counter);
    bench.dump();
  }
  */

  return res;
}

vector<string> iden_to_string_map;
map<string, iden> string_to_iden_map;

std::string iden_to_string(iden id) {
  if (id < iden_to_string_map.size()) {
    return iden_to_string_map[id];
  } else {
    return "UNKNOWN__" + to_string(id);
  }
}

iden string_to_iden(std::string const& s) {
  auto iter = string_to_iden_map.find(s);
  if (iter == string_to_iden_map.end()) {
    iden id = iden_to_string_map.size();
    iden_to_string_map.push_back(s);
    string_to_iden_map.insert(make_pair(s, id));
    return id;
  } else {
    return iter->second;
  }
}

std::vector<std::shared_ptr<Sort>> BooleanSort::get_domain_as_function() const {
  return {};
}

std::shared_ptr<Sort> BooleanSort::get_range_as_function() const {
  return s_bool();
}

std::vector<std::shared_ptr<Sort>> UninterpretedSort::get_domain_as_function() const {
  return {};
}

std::shared_ptr<Sort> UninterpretedSort::get_range_as_function() const {
  return s_uninterp(name);
}

std::vector<std::shared_ptr<Sort>> FunctionSort::get_domain_as_function() const {
  return domain;
}

std::shared_ptr<Sort> FunctionSort::get_range_as_function() const {
  return range;
}

bool sorts_eq(lsort s, lsort t) {
  if (dynamic_cast<BooleanSort*>(s.get())) {
    return dynamic_cast<BooleanSort*>(t.get()) != NULL;
  }
  else if (UninterpretedSort* u1 = dynamic_cast<UninterpretedSort*>(s.get())) {
    UninterpretedSort* u2 = dynamic_cast<UninterpretedSort*>(t.get());
    return u2 != NULL && u1->name == u2->name; 
  }
  else if (FunctionSort* u1 = dynamic_cast<FunctionSort*>(s.get())) {
    FunctionSort* u2 = dynamic_cast<FunctionSort*>(t.get());
    if (u2 == NULL) return false;
    if (u1->domain.size() != u2->domain.size()) return false;
    if (!sorts_eq(u1->range, u2->range)) return false;
    for (int i = 0; i < (int)u1->domain.size(); i++) {
      if (!sorts_eq(u1->domain[i], u2->domain[i])) return false;
    }
    return true;
  }
  else {
    assert(false);
  }
}

bool values_equal(value a, value b) {
  ScopeState ss_a;
  ScopeState ss_b;
  int c = cmp_expr(a, b, ss_a, ss_b);
  return c == 0;
}

value remove_unneeded_quants(Value const * v) {
  assert(v != NULL);
  if (Forall const* value = dynamic_cast<Forall const*>(v)) {
    vector<VarDecl> decls;
    for (VarDecl const& decl : value->decls) {
      if (value->uses_var(decl.name)) {
        decls.push_back(decl);
      }
    }
    auto b = remove_unneeded_quants(value->body.get());
    return decls.size() > 0 ? v_forall(decls, b) : b;
  }
  else if (Exists const* value = dynamic_cast<Exists const*>(v)) {
    vector<VarDecl> decls;
    for (VarDecl const& decl : value->decls) {
      if (value->uses_var(decl.name)) {
        decls.push_back(decl);
      }
    }
    auto b = remove_unneeded_quants(value->body.get());
    return decls.size() > 0 ? v_exists(decls, b) : b;
  }
  else if (NearlyForall const* value = dynamic_cast<NearlyForall const*>(v)) {
    return v_nearlyforall(value->decls, remove_unneeded_quants(value->body.get()));
  }
  else if (Var const* value = dynamic_cast<Var const*>(v)) {
    return v_var(value->name, value->sort);
  }
  else if (Const const* value = dynamic_cast<Const const*>(v)) {
    return v_const(value->name, value->sort);
  }
  else if (Eq const* value = dynamic_cast<Eq const*>(v)) {
    return v_eq(
        remove_unneeded_quants(value->left.get()),
        remove_unneeded_quants(value->right.get()));
  }
  else if (Not const* value = dynamic_cast<Not const*>(v)) {
    return v_not(remove_unneeded_quants(value->val.get()));
  }
  else if (Implies const* value = dynamic_cast<Implies const*>(v)) {
    return v_implies(
        remove_unneeded_quants(value->left.get()),
        remove_unneeded_quants(value->right.get()));
  }
  else if (Apply const* value = dynamic_cast<Apply const*>(v)) {
    vector<shared_ptr<Value>> args;
    for (auto a : value->args) {
      args.push_back(remove_unneeded_quants(a.get()));
    }
    return v_apply(value->func, args);
  }
  else if (And const* value = dynamic_cast<And const*>(v)) {
    vector<shared_ptr<Value>> args;
    for (auto a : value->args) {
      args.push_back(remove_unneeded_quants(a.get()));
    }
    return v_and(args);
  }
  else if (Or const* value = dynamic_cast<Or const*>(v)) {
    vector<shared_ptr<Value>> args;
    for (auto a : value->args) {
      args.push_back(remove_unneeded_quants(a.get()));
    }
    return v_or(args);
  }
  else {
    //printf("value2expr got: %s\n", v->to_string().c_str());
    assert(false && "value2expr does not support this case");
  }
}

bool vec_subset(vector<VarDecl> const& a, vector<VarDecl> const& b) {
  for (VarDecl const& decl : a) {
    for (VarDecl const& decl2 : b) {
      if (decl.name == decl2.name) {
        goto found;
      }
    }
    return false;
    found: {}
  }
  return true;
}

value factor_quants(value v) {
  value body = v;
  vector<VarDecl> decls;
  while (true) {
    if (Forall* f = dynamic_cast<Forall*>(body.get())) {
      extend(decls, f->decls);
      body = f->body;
    } else {
      break;
    }
  }

  if (decls.size() == 0) return v;

  vector<value> disj;
  if (Or* o = dynamic_cast<Or*>(body.get())) {
    disj = o->args;
  } else {
    return v;
  }

  if (disj.size() < 2) {
    return v;
  }

  vector<vector<VarDecl>> used;
  for (value d : disj) {
    vector<VarDecl> used_decls;
    for (VarDecl const& decl : decls) {
      if (d->uses_var(decl.name)) {
        used_decls.push_back(decl);
      }
    }
    used.push_back(used_decls);
  }

  int min_i = 0;
  for (int i = 1; i < (int)used.size(); i++) {
    if (used[i].size() < used[min_i].size()) min_i = i;
  }

  vector<VarDecl> first_decls = used[min_i];

  if (first_decls.size() == decls.size()) {
    return v;
  }

  vector<VarDecl> second_decls;
  for (VarDecl const& decl : decls) {
    bool in_first = false;
    for (VarDecl const& fd : first_decls) {
      if (fd.name == decl.name) {
        in_first = true;
        break;
      }
    }
    if (!in_first) {
      second_decls.push_back(decl);
    }
  }

  vector<value> first_args;  
  vector<value> second_args;  

  for (int i = 0; i < (int)disj.size(); i++) {
    if (vec_subset(used[i], used[min_i])) {
      first_args.push_back(disj[i]);
    } else {
      second_args.push_back(disj[i]);
    }
  }

  if (second_args.size() == 0) {
    assert(first_decls.size() > 0);
    return v_forall(first_decls, v_or(first_args));
  } else {
    assert(second_decls.size() > 0);
    assert(first_args.size() > 0);
    value inner = factor_quants(v_forall(second_decls, v_or(second_args)));
    vector<value> all_args = first_args;
    all_args.push_back(inner);
    return first_decls.size() == 0 ? v_or(all_args) : v_forall(first_decls, v_or(all_args));
  }
}

value Value::reduce_quants() const {
  return factor_quants(remove_unneeded_quants(this));
}

int freshVarDeclCounter = 0;

VarDecl freshVarDecl(lsort sort) {
  string name = "_freshVarDecl_" + to_string(freshVarDeclCounter);
  freshVarDeclCounter++;
  return VarDecl(string_to_iden(name), sort);
}

value Forall::replace_const_with_var(map<iden, iden> const& x) const {
  return v_forall(decls, body->replace_const_with_var(x)); 
}

value NearlyForall::replace_const_with_var(map<iden, iden> const& x) const {
  return v_nearlyforall(decls, body->replace_const_with_var(x)); 
}

value Exists::replace_const_with_var(map<iden, iden> const& x) const {
  return v_exists(decls, body->replace_const_with_var(x)); 
}

value Var::replace_const_with_var(map<iden, iden> const& x) const {
  return v_var(name, sort);
}

value Const::replace_const_with_var(map<iden, iden> const& x) const {
  auto it = x.find(name);
  if (it != x.end()) {
    return v_var(it->second, sort);
  } else {
    return v_const(name, sort);
  }
}

value Eq::replace_const_with_var(map<iden, iden> const& x) const {
  return v_eq(left->replace_const_with_var(x), right->replace_const_with_var(x));
}

value Not::replace_const_with_var(map<iden, iden> const& x) const {
  return v_not(this->val->replace_const_with_var(x));
}

value Implies::replace_const_with_var(map<iden, iden> const& x) const {
  return v_implies(left->replace_const_with_var(x), right->replace_const_with_var(x));
}

value IfThenElse::replace_const_with_var(map<iden, iden> const& x) const {
  return v_if_then_else(cond->replace_const_with_var(x), then_value->replace_const_with_var(x), else_value->replace_const_with_var(x));
}

value Apply::replace_const_with_var(map<iden, iden> const& x) const {
  vector<value> new_args;
  for (value const& arg : args) {
    new_args.push_back(arg->replace_const_with_var(x));
  }
  return v_apply(func->replace_const_with_var(x), move(new_args));
}

value And::replace_const_with_var(map<iden, iden> const& x) const {
  vector<value> new_args;
  for (value const& arg : args) {
    new_args.push_back(arg->replace_const_with_var(x));
  }
  return v_and(move(new_args));
}

value Or::replace_const_with_var(map<iden, iden> const& x) const {
  vector<value> new_args;
  for (value const& arg : args) {
    new_args.push_back(arg->replace_const_with_var(x));
  }
  return v_or(move(new_args));
}

value TemplateHole::replace_const_with_var(map<iden, iden> const& x) const {
  return v_template_hole();
}

template <typename A>
vector<A> concat_vector(vector<A> const& a, vector<A> const& b) {
  vector<A> res = a;
  for (A const& x : b) {
    res.push_back(x);
  }
  return res;
}

template <typename A>
void append_vector(vector<A>& a, vector<A> const& b) {
  for (A const& x : b) {
    a.push_back(x);
  }
}


vector<value> aggressively_split_into_conjuncts(value v)
{
  assert(v.get() != NULL);
  if (Forall* val = dynamic_cast<Forall*>(v.get())) {
    vector<value> vs = aggressively_split_into_conjuncts(val->body);
    vector<value> res;
    for (int i = 0; i < (int)vs.size(); i++) {
      res.push_back(v_forall(val->decls, vs[i]));
    }
    return res;
  }
  else if (dynamic_cast<Exists*>(v.get())) {
    return {v};
  }
  else if (dynamic_cast<NearlyForall*>(v.get())) {
    return {v};
  }
  else if (dynamic_cast<Var*>(v.get())) {
    return {v};
  }
  else if (dynamic_cast<Const*>(v.get())) {
    return {v};
  }
  else if (dynamic_cast<Eq*>(v.get())) {
    return {v};
  }
  else if (Not* val = dynamic_cast<Not*>(v.get())) {
    value w = val->val->negate();
    vector<value> res;
    if (dynamic_cast<Not*>(w.get())) {
      res = {w};
    } else {
      res = aggressively_split_into_conjuncts(w);
    }
    return res;
  }
  else if (Implies* val = dynamic_cast<Implies*>(v.get())) {
    return aggressively_split_into_conjuncts(v_or({v_not(val->left), val->right}));
  }
  else if (dynamic_cast<Apply*>(v.get())) {
    return {v};
  }
  else if (And* val = dynamic_cast<And*>(v.get())) {
    vector<value> res;
    for (value c : val->args) {
      append_vector(res, aggressively_split_into_conjuncts(c));
    }
    return res;
  }
  else if (Or* val = dynamic_cast<Or*>(v.get())) {
    vector<vector<value>> a;
    vector<int> inds;
    for (value c : val->args) {
      a.push_back(aggressively_split_into_conjuncts(c));
      inds.push_back(0);

      if (a[a.size() - 1].size() == 0) {
        return {};
      }
    }
    vector<value> res;
    while (true) {
      vector<value> ors;
      for (int i = 0; i < (int)inds.size(); i++) {
        ors.push_back(a[i][inds[i]]);
      }
      res.push_back(v_or(ors));

      int i;
      for (i = 0; i < (int)inds.size(); i++) {
        inds[i]++;
        if (inds[i] == (int)a[i].size()) {
          inds[i] = 0;
        } else {
          break;
        }
      }
      if (i == (int)inds.size()) {
        break;
      }
    }
    cout << "v: " << v->to_string() << endl;
    for (value r : res) {
      cout << "res: " << r->to_string() << endl;
    }
    return res;
  }
  else if (IfThenElse* val = dynamic_cast<IfThenElse*>(v.get())) {
    return concat_vector(
      aggressively_split_into_conjuncts(v_implies(val->cond, val->then_value)),
      aggressively_split_into_conjuncts(v_implies(v_not(val->cond), val->else_value))
    );
  }
  else {
    assert(false && "aggressively_split_into_conjuncts does not support this case");
  }
}
