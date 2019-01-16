#ifndef GRAMMAR_H
#define GRAMMAR_H

#include <vector>
#include <unordered_map>
#include <string>
#include "z3++.h"

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

};

#endif