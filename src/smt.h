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
	solver *_z3_solver;
	std::vector<expr> _variables;
	std::vector<std::vector<expr>> _tree_variables;
	int _num_children;
	Grammar _grammar;
	Type _output;
	std::vector<Node> _solution;
	context *_z3_ctx;
	std::vector<Node> _nodes;
	std::vector<std::vector<int>> _and_nodes;
	std::unordered_map<int, int> _node2var;
	int _empty_production;
	std::unordered_map<std::string, int> _name2production;
	std::unordered_map<int, std::string> _production2name;
	std::vector<std::vector<int>> _result;
	int _num_and;

	
public:
	SMT(){
		context c;
		solver s(c);
		_z3_solver = &s;
	}

	SMT(Grammar grammar, context &z3_ctx, solver &z3_solver, int num_and){
		_z3_ctx = &z3_ctx;
		_z3_solver = &z3_solver;
		_grammar = grammar;
		_num_children = findNumChildren();
		_output = Type("bool");
		_num_and = num_and;
		// FIXME: compute the necessary depth on the fly
		buildTree(_nodes, _num_children, 4);
		
		for (int i = 1; i <= _num_and; i++){
			std::vector<int> tmp;
			getAndNodes(_nodes[i], tmp);
			_and_nodes.push_back(tmp);
		}

		// empty production
		_empty_production = 0;
		for (int z = 0; z < _grammar.getFunctions().size(); z++){
			_name2production[_grammar.getFunctions()[z].getName()] = z;
			_production2name[z] = _grammar.getFunctions()[z].getName();

			//std::cout << _grammar.getFunctions()[z].getOutput().getType() << std::endl;
			if (!_grammar.getFunctions()[z].getOutput().getType().compare("empty")){
				_empty_production = z;
			}
		}

		createVariables();
		createConstraints();

		// FIXME: generalize this to n children
		int a[_num_and];
		for (int i = 0; i < _num_and; i++){
			a[i] = i+1;
		}
		int n = sizeof a/sizeof a[0]; 
		generatePermutations(a, n, n, _result);

		// createBinaryAssociativeConstraints("=");
		// createBinaryAssociativeConstraints("~=");

		// createAllDiffConstraints("=");
		// createAllDiffConstraints("~=");

		// createAllDiffGrandChildrenConstraints("le");
		// createAllDiffGrandChildrenConstraints("~le");
	
		// breakSymmetries("btw", "A", "B", "C");
		// breakSymmetries("~btw", "A", "B", "C");

	}

	bool solve();
	std::string solutionToString();
	// std::vector<std::vector<Node>> getOutput(){
	// 	return _output_nodes;
	// } 

	void createBinaryAssociativeConstraints(std::string name);
	void createAllDiffConstraints(std::string name);
	void createAllDiffGrandChildrenConstraints(std::string name);
	void breakOccurrences(std::string name, std::string a, std::string b, std::string c);

private:
	int findNumChildren();
	void createVariables();
	void createConstraints();
	void buildTree(std::vector<Node>& nodes, int num_children, int max_depth);
	void blockModel();
	void getAndNodes(Node root, std::vector<int>& ids);
	void generatePermutations(int a[], int size, int n, std::vector<std::vector<int>>& permut);
	void breakSymmetries(std::string name, std::string a, std::string b, std::string c);


};

#endif