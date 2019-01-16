#ifndef SMT_H
#define SMT_H

#include <vector>
#include <unordered_map>
#include "z3++.h"
#include "grammar.h"

using namespace z3;

class Node {

private:
	int _id;
	int _depth;
	std::string _function;
	std::vector<Node> _children;

public:

	Node(){}

	Node(int id, int depth){
		_id = id;
		_depth = depth;
		_function = "";
	}

	std::vector<Node> getChildren(){
		return _children;
	}

	void addChild(Node child){
		_children.push_back(child);
	}

	int getDepth(){
		return _depth;
	}

	int getId(){
		return _id;
	}

	std::string getFunction(){
		return _function;
	}

	void setFunction(std::string function){
		_function = function;
	}
};

class SMT {

private:
	//std::vector<context*> z3_context;
	std::vector<int> _num_models;
	std::vector<solver> _z3_solvers;
	std::vector<std::vector<expr>> _variables;
	int _num_solvers;
	int _num_children;
	Grammar _grammar;
	Type _output;
	std::vector<Node> _solution;
	context *_z3_ctx;
	std::vector<Node> _nodes;
	std::vector<std::vector<Node>> _output_nodes;
	std::unordered_map<int, int> _node2var;
	int _empty_production;
	int _level;

	
public:
	SMT(){
	}

	SMT(int num_solvers, Grammar grammar, context &z3_ctx){
		_z3_ctx = &z3_ctx;
		_num_solvers = num_solvers;
		_grammar = grammar;
		_num_children = findNumChildren();
		_output = Type("bool");
		_level = 0;
		// FIXME: compute the necessary depth on the fly
		buildTree(_nodes, _num_children, 3);

		for (int i = 0; i < 3; i++){
			std::vector<Node> tmp;
			_output_nodes.push_back(tmp);
			buildTree(_output_nodes[i], _num_children, 3);

			_num_models.push_back(0);
		}

		// empty production
		_empty_production = 0;
		for (int z = 0; z < _grammar.getFunctions().size(); z++){
			std::cout << _grammar.getFunctions()[z].getOutput().getType() << std::endl;
			if (!_grammar.getFunctions()[z].getOutput().getType().compare("empty")){
				_empty_production = z;
				break;
			}
		}

		createVariables();
		createConstraints();
		//std::cout << _z3_solvers[0].to_smt2() << "\n";
		//solve();

	}

	bool solve();
	std::string treeToString(int id);
	std::vector<std::vector<Node>> getOutput(){
		return _output_nodes;
	} 

private:
	int findNumChildren();
	void createVariables();
	void createConstraints();
	void buildTree(std::vector<Node>& nodes, int num_children, int max_depth);
	void blockModel(int id);


};

#endif