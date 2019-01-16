#include "smt.h"
#include <deque> 
#include <unordered_map>

void SMT::buildTree(std::vector<Node>& nodes, int num_children, int max_depth){
	Node root = Node(1, 1);
	int id = 1;
	std::deque<Node> work;
	work.push_back(root);
	while(!work.empty()){
		Node current = work.front();
		work.pop_front();
		for (int i = 0; i < num_children; i++){
			id++;
			std::vector<Node> next_children;
			Node next = Node(id, current.getDepth()+1);
			current.addChild(next);
			if (next.getDepth() < max_depth){
				work.push_back(next);
			} else
			  nodes.push_back(next);
		}
		nodes.push_back(current);
	}
}

int SMT::findNumChildren(){
	int max = 0;
	for (int i = 0; i < _grammar.getFunctions().size(); i++){
		int children = _grammar.getFunctions()[i].getInputs().size();
		if (children > max) max = children;
	}
	return max;
}

void SMT::createVariables(){

	for (int i = 0; i < _num_solvers; i++){
		std::vector<expr> var;
		for (int j = 0; j < _nodes.size(); j++){
			std::string name = "node" + std::to_string(_nodes[j].getId());
			expr x = _z3_ctx->int_const(name.c_str());
			var.push_back(x);
			_node2var[_nodes[j].getId()] = var.size()-1;
		}
		_variables.push_back(var);
	}
	
	for (int i = 0; i < _num_solvers; i++){
    solver s(*_z3_ctx);
    _z3_solvers.push_back(s);
	}
}

void SMT::createConstraints(){
	// domain of each integer variable
	int domain = _grammar.getFunctions().size();
	for (int i = 0; i < _num_solvers; i++){
		for (int j = 0; j < _nodes.size(); j++){
		  expr_vector ctr(*_z3_ctx);
		  ctr.push_back(_variables[i][j] >= 0);
		  ctr.push_back(_variables[i][j] < domain);
			_z3_solvers[i].add(mk_and(ctr));
		}
	}

	// empty production
	int empty_value = 0;
	for (int z = 0; z < domain; z++){
		if (!_grammar.getFunctions()[z].getOutput().getType().compare("empty")){
			empty_value = z;
			break;
		}
	}

	// parent to children relation
	// position 0 is the root of this tree with single depth
	for (int i = 0; i < _num_solvers; i++){
		for (int j = 0; j < _nodes.size(); j++){
			if (_nodes[j].getChildren().empty()){
				// leaf node
				expr_vector ctr(*_z3_ctx);
				for (int z = 0; z < domain; z++){
					if (_grammar.getFunctions()[z].getInputs().size() == 1 && 
						!_grammar.getFunctions()[z].getInputs()[0].getType().compare("empty")){
						ctr.push_back(_variables[i][j] == z);
					}
				}
				_z3_solvers[i].add(mk_or(ctr));
			} else {
				// parent-child constraint
				for (int z = 0; z < domain; z++){
					for (int w = 0; w < _num_children; w++){
						if (_grammar.getFunctions()[z].getInputs().size() < w+1){
							// if the number of children is smaller than the node will be empty
							int id_node = _node2var[_nodes[j].getChildren()[w].getId()];
						 	_z3_solvers[i].add(implies(_variables[i][j] == z, _variables[i][id_node] == empty_value));
						} else {
							expr_vector rhs(*_z3_ctx);
							std::string expected_type = _grammar.getFunctions()[z].getInputs()[w].getType();
							for (int x = 0; x < domain; x++){
								// find functions of the same type
								int id_node = _node2var[_nodes[j].getChildren()[w].getId()];
								if (!_grammar.getFunctions()[x].getOutput().getType().compare(expected_type)){
									rhs.push_back(_variables[i][id_node] == x);
								}
							}
							_z3_solvers[i].add(implies(_variables[i][j] == z, mk_or(rhs)));
						}
					}
				}
			}
		} 
	}
	
	// restrict the root parent productions to a given type
	for (int i = 0; i < _num_solvers; i++){
		expr_vector ctr(*_z3_ctx);
		for (int j = 0; j < domain; j++){
			if (!_grammar.getFunctions()[j].getOutput().getType().compare(_output.getType())){
				ctr.push_back(_variables[i][0] == j);
			}
		}
		_z3_solvers[i].add(mk_or(ctr));
	}

}


