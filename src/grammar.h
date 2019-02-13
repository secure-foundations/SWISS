#ifndef GRAMMAR_H
#define GRAMMAR_H

#include <vector>
#include <unordered_map>
#include <string>
#include <cassert>
#include "z3++.h"
#include "logic.h"

class Type;
class Grammar;

class GrammarVar {
public:
  std::string name;
  std::string type;
  GrammarVar(std::string name, std::string type) : name(name), type(type) { }
};

Grammar createGrammarFromModule(
    std::shared_ptr<Module> module,
    std::vector<GrammarVar> vars
);

class Type {

	private:
		std::string _type;

  public:
  	Type() {
  		_type = "";
  	}

  	Type(std::string type) {
  		_type = type;
  	}

  	std::string getType() {
  		return _type;
  	}

  	lsort toSort() {
  	  if (_type == "bool") {
  	    return s_bool();
  	  } else {
  	    return s_uninterp(_type);
  	  }
  	}
};

class Function {

private: 
	std::string _name;
	std::vector<Type> _inputs;
	Type _output;

public:
	Function(std::string name, std::vector<Type> inputs, Type output){
		_inputs = inputs;
		_output = output;
		_name = name;
	}

	std::string getName() {
		return _name;
	}
	
	std::vector<Type> getInputs(){
		return _inputs;
	}

	Type getOutput(){
		return _output;
	}

  lsort toSort() {
    if (_inputs.size() == 0) {
      return _output.toSort();
    } else {
      std::vector<lsort> inps;
      for (Type& t : _inputs) {
        inps.push_back(t.toSort());
      }
      return s_fun(inps, _output.toSort());
    }
  }
};

class Grammar {

private:
	std::vector<Type> _types;
	std::vector<Function> _functions;

public:
	Grammar(){

	}

	Grammar(std::vector<Type> types, std::vector<Function> functions){
		_types = types;
		_functions = functions;
	}

	std::vector<Function> getFunctions() {
		return _functions;
	}

  Function getFunctionByName(std::string const& name) {
    for (Function f : _functions) {
      if (f.getName() == name) {
        return f;
      }
    }
    assert(false);
  }
};

#endif
