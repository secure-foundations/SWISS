#include "logic.h"
#include "lib/json11/json11.hpp"

#include <cassert>
#include <iostream>

using namespace std;
using namespace json11;

shared_ptr<Module> json2module(Json j);
vector<string> json2string_array(Json j);
vector<shared_ptr<Value>> json2value_array(Json j);
vector<shared_ptr<Action>> json2action_array(Json j);
vector<shared_ptr<Action>> json2action_array_from_map(Json j);
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
  return shared_ptr<Module>(new Module(
      json2string_array(j["sorts"]),
      json2decl_array(j["functions"]),
      json2value_array(j["axioms"]),
      json2value_array(j["inits"]),
      json2value_array(j["conjectures"]),
      json2action_array_from_map(j["actions"])));
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

vector<shared_ptr<Action>> json2action_array_from_map(Json j) {
  assert(j.is_object());
  vector<shared_ptr<Action>> res;
  for (auto p : j.object_items()) {
    res.push_back(json2action(p.second));
  }
  return res;
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
    res.push_back(VarDecl(elem[1].string_value(), json2sort(elem[2])));
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
  else if (type == "exists") {
    assert (j.array_items().size() == 3);
    return shared_ptr<Value>(new Exists(json2decl_array(j[1]), json2value(j[2])));
  }
  else if (type == "var") {
    assert (j.array_items().size() == 3);
    assert (j[1].is_string());
    return shared_ptr<Value>(new Var(j[1].string_value(), json2sort(j[2])));
  }
  else if (type == "const") {
    assert (j.array_items().size() == 3);
    assert (j[1].is_string());
    return shared_ptr<Value>(new Const(j[1].string_value(), json2sort(j[2])));
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
  else {
    printf("value type: %s\n", type.c_str());
    assert(false && "unrecognized Value type");
  }
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

string Forall::to_string() {
  string res = "forall ";
  for (int i = 0; i < decls.size(); i++) {
    if (i > 0) {
      res += ", ";
    }
    res += decls[i].name;
  }
  res += " . (" + body->to_string() + ")";
  return res;
}

string Exists::to_string() {
  string res = "exists ";
  for (int i = 0; i < decls.size(); i++) {
    if (i > 0) {
      res += ", ";
    }
    res += decls[i].name;
  }
  res += " . (" + body->to_string() + ")";
  return res;
}

string Var::to_string() {
  return name;
}

string Const::to_string() {
  return name;
}

string Eq::to_string() {
  return "(" + left->to_string() + ") = (" + right->to_string() + ")";
}

string Not::to_string() {
  return "~(" + value->to_string() + ")";
}

string Implies::to_string() {
  return "(" + left->to_string() + ") -> (" + right->to_string() + ")";
}

string And::to_string() {
  string res = "";
  for (int i = 0; i < args.size(); i++) {
    if (i > 0) {
      res += " & ";
    }
    res += "(" + args[i]->to_string() + ")";
  }
  return res;
}

string Or::to_string() {
  string res = "";
  for (int i = 0; i < args.size(); i++) {
    if (i > 0) {
      res += " | ";
    }
    res += "(" + args[i]->to_string() + ")";
  }
  return res;
}

string Apply::to_string() {
  string res = func->to_string() + "(";
  for (int i = 0; i < args.size(); i++) {
    if (i > 0) {
      res += ", ";
    }
    res += args[i]->to_string();
  }
  return res;
}
