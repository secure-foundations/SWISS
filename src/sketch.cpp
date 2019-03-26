#include "sketch.h"

#include <cassert>

SketchFormula::SketchFormula(
    z3::context& ctx,
    z3::solver& solver,
    vector<VarDecl> free_vars,
    shared_ptr<Module> module,
    int arity, int depth)
  : ctx(ctx), solver(solver), free_vars(free_vars), arity(arith), depth(depth)
{
  branch_types = {
    BranchType(BTT::True, "", {}, bool_sort),
    BranchType(BTT::False, "", {}, bool_sort),
    BranchType(BTT::And, "", {bool_sort, bool_sort}, bool_sort),
    BranchType(BTT::Or, "", {bool_sort, bool_sort}, bool_sort),
    BranchType(BTT::Not, "", {bool_sort}, bool_sort)
  };
  for (lsort s : module->sorts) {
    branch_types.push_back(BranchType(BTT::Eq, "", {s, s}, bool_sort));
  }
  for (VarDecl decl : module->functions) {
    branch_types.push_back(BranchType(BTT::Func, decl.name,
      decl.sort->get_domain_as_function(),
      decl.sort->get_range_as_function()));
  }

  int num_branch_nodes = 0;
  int level, num_nodes_at_level;
  for (level = 0, num_nodes_at_level = 1; level < depth;
      level++, num_nodes_at_level *= arith) {
    num_branch_nodes += num_nodes_at_level;
  }
  int num_leaf_nodes = num_nodes_at_level;
  int num_total_nodes = num_branch_nodes + num_leaf_nodes;

  this->nodes.resize(num_total_nodes);
  this->root = &nodes[0];

  for (int i = 0; i < num_total_nodes) {
    nodes[i].is_leaf = (i >= num_branch_nodes);
    if (!nodes[i].is_leaf) {
      for (int j = 0; j < arity; j++) {
        assert(i * arity + j <= nodes.size());
        nodes[i].children.push_back(&nodes[i * arity + j]);
      }
    }
  }

  for (int i = 0; i < num_total_nodes; i++) {
    make_sort_bools(&nodes[i]);
    if (i < num_branch_nodes) {
      make_bt_bools(&nodes[i]);
    } else {
      make_lt_bools(&nodes[i]);
    }
  }

  for (int i = 0; i < num_total_nodes; i++) {
    if (i < num_branch_nodes) {
      make_sort_constraints_branch();
    } else {
      make_sort_constraints_leaf();
    }
  }
}

/*
z3::expr SketchFormula::interpret(
    std::shared_ptr<Model>,
    std::vector<object_value> const& vars)
{
  
}
*/

value SketchFormula::to_value() {
  return this->node_to_value(this->root);
}

value SketchFormula::node_to_value(Node* node) {
  if (node->is_leaf) {
  } else {
    for (int i = 0; i <   
  }
}
