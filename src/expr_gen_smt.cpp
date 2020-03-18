#include "expr_gen_smt.h"
#include <deque> 
#include <unordered_map>
#include <set>

using namespace std;

void SMT::generatePermutations(int a[], int size, int n, std::vector<std::vector<int>>& permutation){ 
    // if size becomes 1 then prints the obtained 
    // permutation 
	if (size == 1) 
	{ 
		std::vector<int> perm;
		for (int i=0; i<n; i++){
        //std::cout << a[i] << " "; 
			perm.push_back(a[i]);
		} 
		permutation.push_back(perm);
			//printf("\n"); 
		return; 
	} 

	for (int i=0; i<size; i++) 
	{ 
		generatePermutations(a,size-1,n, permutation); 

        // if size is odd, swap first and last 
        // element 
		if (size%2==1) 
			std::swap(a[0], a[size-1]); 

        // If size is even, swap ith and last 
        // element 
		else
			std::swap(a[i], a[size-1]); 
	} 
}

void SMT::blockModel(){

	bool no_string = false;

	if (!no_string){
		for (int i = 0; i < (int)_nodes.size(); i++){
			_nodes[i].setFunction("");
		}
	}

	std::unordered_map<int, int> assignment;

	//expr_vector gctr(*_z3_ctx);
	
	model m = _z3_solver->get_model();
	for (unsigned i = 0; i < m.size(); i++) {
		func_decl v = m[i];
		assert(v.arity() == 0); 
		int var = std::stoi(v.name().str().substr(v.name().str().find("node") + 4));
		int value = m.get_const_interp(v).get_numeral_int();
		if (value != _empty_production){
			if (!no_string)
				_nodes[var-1].setFunction(_grammar.getFunctions()[value].getName());
			//gctr.push_back(_variables[var-1] != m.get_const_interp(v).get_numeral_int());
		}
		// else
		// 	_nodes[var-1].setFunction("");

		assignment[var] = m.get_const_interp(v).get_numeral_int();
	}

	std::vector<std::vector<int>> values;
	std::vector<std::vector<int>> ids;
	for (int i = 0; i < (int)_and_nodes.size(); i++){
		std::vector<int> v;
		std::vector<int> id;
		values.push_back(v);
		ids.push_back(id);
		for (int j = 0; j < (int)_and_nodes[i].size(); j++){
			int var = _and_nodes[i][j];
				//std::cout << "v* = " << var <<  " " << " value* = " << assignment[var] << std::endl;
			values[values.size()-1].push_back(assignment[var]);
			ids[ids.size()-1].push_back(var-1);
		}
	}

	// std::cout << _nodes[1].getFunction() << std::endl;
	// std::cout << _nodes[2].getFunction() << std::endl;
	// std::cout << _nodes[3].getFunction() << std::endl;

	for (int i = 0; i < (int)_result.size(); i++){
		expr_vector ctr(*_z3_ctx);
		for (int j = 0; j < (int)_result[i].size(); j++){
			int x = _result[i][j]-1;
			int y = j;
			for (int z = 0; z < (int)ids[x].size(); z++){
				assert (ids[x].size() == values[y].size());
				int id = ids[x][z];
				int value = values[y][z];
				if (value != _empty_production) {
					expr xp = _z3_ctx->int_val(value);
					ctr.push_back(_variables[id] != xp);	
				}
			}
		}
		_z3_solver->add(mk_or(ctr));
	}	



	// for (int i = 0; i < result.size(); i++){
	// 	expr_vector ctr(*_z3_ctx);
	// 	for (int j = 0; j < result[i].size(); j++){
	// 		printf("pos=%d\n",result[i][j]);
	// 		int p = result[i][j]-1;
	// 		for (int z = 0; z < _and_nodes[p].size(); z++){
	// 			if (_nodes[_and_nodes[i][j].getId()-1].getFunction().compare("")){

	// 			}
	// 		}
	// 	}
	// }	

	//_z3_solver->add(mk_or(ctr));
	//assert(false);

}

std::string SMT::solutionToString(){
	return solutionToValue()->to_string();
}

value SMT::nodeToValue(Node const& node) {
  std::string func = node.getFunction();
  assert(func != "");

  lsort function_sort = _grammar.getFunctionByName(func).toSort();

  if (func.substr(0, 6) == "__var.") {
    return v_var(string_to_iden(func.substr(6)), function_sort);
  }

  if (dynamic_cast<FunctionSort*>(function_sort.get())) {
    std::vector<value> children;
    for (int j = 0; j < (int)_nodes[node.getId()-1].getChildren().size(); j++){
      // FIXME: fix the pointers in the _nodes
      if (_nodes[_nodes[node.getId()-1].getChildren()[j].getId()-1].getFunction().compare("")){						
        children.push_back(
            nodeToValue(_nodes[_nodes[node.getId()-1].getChildren()[j].getId()-1]));
      }
    }

    if (func.substr(0, 2) == "=.") {
      assert(children.size() == 2);
      return v_eq(children[0], children[1]);
    } else if (func.substr(0, 3) == "~=.") {
      assert(children.size() == 2);
      return v_not(v_eq(children[0], children[1]));
    } else { 
      if (func[0] == '~') {
        return v_not(v_apply(
            v_const(string_to_iden(func.substr(1)), function_sort),
            children));
      } else {
        return v_apply(
            v_const(string_to_iden(func), function_sort),
            children);
      }
    }
  } else {
    if (func[0] == '~') {
      return v_not(
        v_const(string_to_iden(func.substr(1)), function_sort)
      );
    } else {
      return v_const(string_to_iden(func), function_sort);
    }
  }
}

value SMT::solutionToValue() {
	std::vector<value> conjuncts;
	for (int i = 0; i < (int)_and_nodes.size(); i++){
	  Node& node = _nodes[_and_nodes[i][0]-1];
	  if (node.getFunction() != "") {
      conjuncts.push_back(nodeToValue(node));
    }
	}
	return v_not(v_and(conjuncts));
}

bool SMT::solve(){
	/*
	 * The setwise enumeration uses only one solver
	*/

	check_result res = _z3_solver->check();
	if (res == sat){
		blockModel();
		return true;
	} else return false;

}

void SMT::getAndNodes(Node root, std::vector<int>& ids){
	ids.clear();
	std::deque<Node> work;
	work.push_back(root);
	// /printf("root = %d\n",root.getId());
	while(!work.empty()){
		Node current = work.front();
		work.pop_front();
		ids.push_back(current.getId());
		//printf("id=%d\n",current.getId());
		for (int i = 0; i < (int) _nodes[current.getId()-1].getChildren().size(); i++){
			work.push_back(_nodes[current.getId()-1].getChildren()[i]);
		}
	}
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
	for (int i = 0; i < (int)_grammar.getFunctions().size(); i++){
		int children = (int)_grammar.getFunctions()[i].getInputs().size();
		if (children > max) max = children;
	}
	return max;
}

void SMT::createVariables(){

	for (int j = 0; j < (int)_nodes.size(); j++){
		std::string name = "node" + std::to_string(_nodes[j].getId());
		expr x = _z3_ctx->int_const(name.c_str());
		_variables.push_back(x);
		_node2var[_nodes[j].getId()] = (int)_variables.size()-1;
	}
}

// FIXME: generalize these constraints
void SMT::createBinaryAssociativeConstraints(std::string name){
	assert (_name2production.find(name) != _name2production.end());
	int children = _grammar.getFunctions()[_name2production[name]].getInputs().size();
	assert (children == 2);
	for (int i = 1; i <= _num_and; i++){
		_z3_solver->add(implies(_variables[i] == _z3_ctx->int_val(_name2production[name]),
			_variables[_nodes[i].getChildren()[0].getId()-1] < _variables[_nodes[i].getChildren()[1].getId()-1]
			));
	}
}

void SMT::createAllDiffConstraints(std::string name){
	assert (_name2production.find(name) != _name2production.end());
	int children = _grammar.getFunctions()[_name2production[name]].getInputs().size();
	for (int i = 1; i <= _num_and; i++){

		expr_vector rhs(*_z3_ctx);
		for (int j = 0; j < children; j++){
			for (int w = j+1; w < children; w++){
				rhs.push_back(_variables[_nodes[i].getChildren()[j].getId()-1] != 
					_variables[_nodes[i].getChildren()[w].getId()-1]);
			}
		}
		_z3_solver->add(implies(_variables[i] == _z3_ctx->int_val(_name2production[name]),
			mk_and(rhs)));
	}
}

// HACK for le: generalize afterwards
void SMT::createAllDiffGrandChildrenConstraints(std::string name){
	assert (_name2production.find(name) != _name2production.end());
	//int children = _grammar.getFunctions()[_name2production[name]].getInputs().size();
	for (int i = 1; i <= _num_and; i++){
		expr ctr = implies(_variables[i] == _z3_ctx->int_val(_name2production[name]),
			_variables[_nodes[_nodes[i].getChildren()[0].getId()-1].getChildren()[0].getId()-1] != 
			_variables[_nodes[_nodes[i].getChildren()[1].getId()-1].getChildren()[0].getId()-1]
			);
		_z3_solver->add(ctr);
	}
}

void SMT::breakOccurrences(std::string name, std::string a, std::string b, std::string c){
	assert (_name2production.find(name) != _name2production.end());
	for (int i = 1; i <= _num_and; i++){
		expr_vector o1(*_z3_ctx);
		o1.push_back(_variables[_nodes[i].getChildren()[0].getId()-1] == _z3_ctx->int_val(_name2production[a]));
		o1.push_back(_variables[_nodes[i].getChildren()[1].getId()-1] == _z3_ctx->int_val(_name2production[b]));
		o1.push_back(_variables[_nodes[i].getChildren()[2].getId()-1] == _z3_ctx->int_val(_name2production[c]));

		expr ctr = implies(_variables[i] == _z3_ctx->int_val(_name2production[name]), not(mk_and(o1)));
		//std::cout << "ctr= " << ctr << std::endl;
		_z3_solver->add(ctr);
	}
}

// HACK for btw: generalize afterwards
void SMT::breakSymmetries(std::string name, std::string a, std::string b, std::string c){
	// ABC -> BCA -> CAB
	// ACB -> CBA -> BAC
	// only allow ABC or ACB
	assert (_name2production.find(name) != _name2production.end());
	for (int i = 1; i <= _num_and; i++){
		expr_vector o1(*_z3_ctx);
		o1.push_back(_variables[_nodes[i].getChildren()[0].getId()-1] == _z3_ctx->int_val(_name2production[a]));
		o1.push_back(_variables[_nodes[i].getChildren()[1].getId()-1] == _z3_ctx->int_val(_name2production[b]));
		o1.push_back(_variables[_nodes[i].getChildren()[2].getId()-1] == _z3_ctx->int_val(_name2production[c]));

		expr_vector o2(*_z3_ctx);
		o2.push_back(_variables[_nodes[i].getChildren()[0].getId()-1] == _z3_ctx->int_val(_name2production[a]));
		o2.push_back(_variables[_nodes[i].getChildren()[1].getId()-1] == _z3_ctx->int_val(_name2production[c]));
		o2.push_back(_variables[_nodes[i].getChildren()[2].getId()-1] == _z3_ctx->int_val(_name2production[b]));

		expr_vector o3(*_z3_ctx);
		o3.push_back(mk_and(o1));
		o3.push_back(mk_and(o2));
		expr ctr = implies(_variables[i] == _z3_ctx->int_val(_name2production[name]),mk_or(o3));
		//std::cout << "ctr = " << ctr << std::endl;
		_z3_solver->add(ctr);
	}
}

void SMT::createConstraints(){
	// domain of each integer variable
	int domain = _grammar.getFunctions().size();
	for (int j = 0; j < (int)_nodes.size(); j++){
		expr_vector ctr(*_z3_ctx);
		ctr.push_back(_variables[j] >= 0);
		ctr.push_back(_variables[j] < domain);
		_z3_solver->add(mk_and(ctr));
	}

	// parent to children relation
	// position 0 is the root of this tree with single depth
	for (int j = 0; j < (int)_nodes.size(); j++){
		if (_nodes[j].getChildren().empty()){
				// leaf node
			expr_vector ctr(*_z3_ctx);
			for (int z = 0; z < domain; z++){
				if (_grammar.getFunctions()[z].getInputs().size() == 1 && 
					!_grammar.getFunctions()[z].getInputs()[0].getType().compare("empty")){
					ctr.push_back(_variables[j] == z);
			}
		}
		_z3_solver->add(mk_or(ctr));
	} else {
				// parent-child constraint
		for (int z = 0; z < domain; z++){
			for (int w = 0; w < _num_children; w++){
				if ((int)_grammar.getFunctions()[z].getInputs().size() < w+1){
							// if the number of children is smaller than the node will be empty
					int id_node = _node2var[_nodes[j].getChildren()[w].getId()];
					_z3_solver->add(implies(_variables[j] == z, _variables[id_node] == _empty_production));
				} else {
					expr_vector rhs(*_z3_ctx);
					std::string expected_type = _grammar.getFunctions()[z].getInputs()[w].getType();
					for (int x = 0; x < domain; x++){
								// find functions of the same type
						int id_node = _node2var[_nodes[j].getChildren()[w].getId()];
						if (!_grammar.getFunctions()[x].getOutput().getType().compare(expected_type)){
							rhs.push_back(_variables[id_node] == x);
						}
					}
					_z3_solver->add(implies(_variables[j] == z, mk_or(rhs)));
				}
			}
		}
	}
}

	// restrict the children of the root parent productions to a given type (and -> children)
expr_vector ctr(*_z3_ctx);
for (int j = 0; j < domain; j++){
	if (!_grammar.getFunctions()[j].getOutput().getType().compare(_output.getType())){
		ctr.push_back(_variables[0] == j);
	}
}
_z3_solver->add(mk_or(ctr));

	// restrict the root parent production to 'and' and disallow 'and' everywhere else
for (int j = 0; j < domain; j++){
	if (!_grammar.getFunctions()[j].getOutput().getType().compare(_output.getType())){
		std::string name = _grammar.getFunctions()[j].getName();
		if (!name.compare("and")){
			//std::cout << _grammar.getFunctions()[j].getName() << std::endl;
			for (int z = 0; z < (int)_variables.size(); z++){
				if (z == 0){
					expr_vector ctr(*_z3_ctx);
					ctr.push_back(_variables[z] == j);
					_z3_solver->add(mk_and(ctr));
				} else {
					expr_vector ctr(*_z3_ctx);
					ctr.push_back(_variables[z] != j);
					_z3_solver->add(mk_and(ctr));
				}
			}
		}
	}
}


}


