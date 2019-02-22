#include "grammar.h"

#include "logic.h"

using namespace std;

Type sort_to_type(lsort s) {
  if (dynamic_cast<BooleanSort*>(s.get())) {
    return Type("bool");
  } else if (UninterpretedSort* usort = dynamic_cast<UninterpretedSort*>(s.get())) {
    assert(usort->name != "empty");
    assert(usort->name != "bool");
    return Type(usort->name);
  } else {
    assert(false);
  }
}

Grammar createGrammarFromModule(shared_ptr<Module> module, vector<GrammarVar> vars) {
  //int numChildren = 2; // FIXME

  Type bool_type = Type("bool");
  Type empty_type = Type("empty");
  vector<Type> types{bool_type, empty_type};
  for (string sortname : module->sorts) {
    types.push_back(Type(sortname));
  }

  vector<Function> functions;

  // given functions/relations
  for (VarDecl decl : module->functions) {
    lsort s = decl.sort;
    if (!dynamic_cast<FunctionSort*>(s.get())) {
      s = s_fun({}, s);
    }
    FunctionSort* fun_sort = dynamic_cast<FunctionSort*>(s.get());

    vector<Type> inputs;
    //assert(fun_sort->domain.size() <= numChildren);
    for (int i = 0; i < fun_sort->domain.size(); i++) {
      inputs.push_back(sort_to_type(fun_sort->domain[i]));
    }
    /*
    while (inputs.size() < numChildren) {
      inputs.push_back(empty_type);
    }
    */
    Type output = sort_to_type(fun_sort->range);

    functions.push_back(Function(iden_to_string(decl.name), inputs, output));
    if (output.getType() == "bool") {
      functions.push_back(Function("~" + iden_to_string(decl.name), inputs, output));
    }
  }

  // = and ~=
  for (string sortname : module->sorts) {
    functions.push_back(Function("=." + sortname, {Type(sortname), Type(sortname)}, bool_type));
    functions.push_back(Function("~=." + sortname, {Type(sortname), Type(sortname)}, bool_type));
  }

  // empty
  functions.push_back(Function("empty", {empty_type}, empty_type));

  // variables
  for (GrammarVar v : vars) {
    functions.push_back(Function(v.name, {empty_type}, Type(v.type)));
  }

  // and
  functions.push_back(Function("and", {bool_type, bool_type, bool_type}, bool_type));

  return Grammar(types, functions);
}
