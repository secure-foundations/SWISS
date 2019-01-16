#include "smt.h"
#include <deque> 
#include <unordered_map>

void SMT::blockModel(int id){

	// clean the previous invariant
	for (int i = 0; i < _output_nodes[id].size(); i++){
		_output_nodes[id][i].setFunction("");
	}

	expr_vector ctr(*_z3_ctx);
	 model m = _z3_solvers[id].get_model();
   // traversing the model
 	for (unsigned i = 0; i < m.size(); i++) {
 		func_decl v = m[i];
 		// this problem contains only constants
 		assert(v.arity() == 0); 
 		int var = std::stoi(v.name().str().substr(v.name().str().find("node") + 4));
 		ctr.push_back(_variables[id][var-1] != m.get_const_interp(v));
 		int value = m.get_const_interp(v).get_numeral_int();
 		if (value != _empty_production){
 			//std::cout << _grammar.getFunctions()[value].getName() << std::endl;
 			_output_nodes[id][var-1].setFunction(_grammar.getFunctions()[value].getName());
 		}
 	}
 	_z3_solvers[id].push();
 	_z3_solvers[id].add(mk_or(ctr));
 	_num_models[id]++;

}

std::string SMT::treeToString(int id){
	std::string invariant = "";
	for (int i = 0; i < _output_nodes[id].size(); i++){
		if (_output_nodes[id][i].getFunction().compare("")){
			//std::cout << _output_nodes[id][i].getFunction() << std::endl;
			invariant += _output_nodes[id][i].getFunction() + " ";
		}
	}
	return invariant;
}

bool SMT::solve(){

	if (_level == 2){
		check_result res = _z3_solvers[2].check();
		if (res == sat){
			blockModel(2);
			return true;
		} else {
			_level--;
			for (int i = 0; i < _num_models[2]; i++)
				_z3_solvers[2].pop();
			_num_models[2] = 0;
			return solve();
		}
	} else {
		if (_level == 0){
			check_result res = _z3_solvers[0].check();
			if (res == sat){
				blockModel(0);
				_level++;
				return solve();
			} else {
				return false;
			}
		} else {
			check_result res = _z3_solvers[1].check();
			if (res == sat){
				blockModel(1);
				_level++;
				return solve();
			} else {
				_level--;
				for (int i = 0; i < _num_models[1]; i++)
				_z3_solvers[1].pop();
				_num_models[1] = 0;
				return solve();
			}
		}
	}

	// check_result res = sat;
	// int program = 1;
	// while (res == sat){
	// 	printf("program #=%d\n",program);
	// 	res = _z3_solvers[0].check();
	// 	if (res == sat){
	// 		blockModel(0);
	// 		std::cout << "Program = " << treeToString(0) << std::endl;
	// 	}
	// 	program++;
	// }
	
	// TODO: high level algorithm to avoid repetition of models
	/*
	int level = 0;
	check_result res = sat;
	while (res != unsat){
		check_result r = _z3_solvers[level].check();
		// go to the next level
		if (r == sat){
			if (level < 2){
				// block model in the higher levels
				level++;
			}
		} else {
			// we are the firt level and exhausted the search space
			if (level == 0){
				res = unsat;
			} else {
				// go back one level
				level--;
				// remove the previous blocking
			}
		}
		*/

	// simpler version with repetitions just to start the ball rolling
	// level0:
	// 	check_result r1 = _z3_solvers[0].check();
	// 	if (r1 == unsat) goto level3;

	// level1:
	// 	check_result r2 = _z3_solvers[1].check();
	// 	if (r2 == unsat) {
	// 		// block current model in solver0
	// 		// pop all blocking models in solver1
	// 		goto level0;
	// 	} else {
	// 		level2:
	// 		check_result r3 = _z3_solvers[2].check();
	// 		if (r3 == unsat){
	// 			// pop all blocking models in solver2
	// 			// block current model in solver1
	// 			goto level1;
	// 		} else {
	// 			// block current model in solver2
	// 			// build and return current solution
	// 			//goto level2;
	// 			return true;
	// 		}
	// 	}

	// level3:
	// return false;

}

void SMT::buildTree(std::vector<Node>& nodes, int num_children, int max_depth){
	Node root = Node(1, 1);
	int id = 1;
	std::deque<Node> work;
	work.push_back(root);
	while(!work.empty()){
		Node current = work.front();
		work.pop_front();
		if (current.getDepth() != max_depth){
			for (int i = 0; i < num_children; i++){
				id++;
				std::vector<Node> next_children;
				Node next = Node(id, current.getDepth()+1);
				current.addChild(next);
				work.push_back(next);
			}
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
						 	_z3_solvers[i].add(implies(_variables[i][j] == z, _variables[i][id_node] == _empty_production));
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


