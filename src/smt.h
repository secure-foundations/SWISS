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
	std::vector<Node> _children;

public:

	Node(){}

	Node(int id, int depth){
		_id = id;
		_depth = depth;
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

};

class SMT {

private:
	//std::vector<context*> z3_context;
	std::vector<solver> _z3_solvers;
	std::vector<std::vector<expr>> _variables;
	int _num_solvers;
	int _num_children;
	Grammar _grammar;
	Type _output;
	context *_z3_ctx;
	std::vector<Node> _nodes;
	std::unordered_map<int, int> _node2var;
	
public:
	SMT(){
	}

	SMT(int num_solvers, Grammar grammar, context &z3_ctx){
		_z3_ctx = &z3_ctx;
		_num_solvers = num_solvers;
		_grammar = grammar;
		_num_children = findNumChildren();
		_output = Type("bool");
		// FIXME: compute the necessary depth on the fly
		buildTree(_nodes, _num_children, 3);
		createVariables();
		createConstraints();
		std::cout << _z3_solvers[0].to_smt2() << "\n";

	}

	int findNumChildren();
	void createVariables();
	void createConstraints();
	void buildTree(std::vector<Node>& nodes, int num_children, int max_depth);

};

#endif