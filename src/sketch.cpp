#include "sketch.h"

#include <cassert>

using namespace std;

SketchFormula::SketchFormula(
    z3::context& ctx,
    z3::solver& solver,
    vector<VarDecl> free_vars,
    shared_ptr<Module> module,
    int arity, int depth)
  : ctx(ctx), solver(solver), free_vars(free_vars), arity(arity), depth(depth)
{
  assert (2 <= arity);

  for (string sort_name : module->sorts) {
    lsort s = s_uninterp(sort_name);
    this->sorts.push_back(s);
  }

  this->functions = module->functions;

  lsort bool_sort = s_bool();

  this->node_types = {
    NodeType(NTT::True, -1, {}, bool_sort),
    NodeType(NTT::False, -1, {}, bool_sort),
    NodeType(NTT::And, -1, {bool_sort, bool_sort}, bool_sort),
    NodeType(NTT::Or, -1, {bool_sort, bool_sort}, bool_sort),
    NodeType(NTT::Not, -1, {bool_sort}, bool_sort)
  };
  for (lsort s : this->sorts) {
    this->node_types.push_back(NodeType(NTT::Eq, -1, {s, s}, bool_sort));
  }
  for (int i = 0; i < module->functions.size(); i++) {
    VarDecl const& decl = module->functions[i];
    if (decl.sort->get_domain_as_function().size() <= arity) {
      this->node_types.push_back(NodeType(NTT::Func, i,
        decl.sort->get_domain_as_function(),
        decl.sort->get_range_as_function()));
    }
  }
  for (int i = 0; i < free_vars.size(); i++) {
    VarDecl& decl = free_vars[i];
    this->node_types.push_back(NodeType(NTT::Var, i, {}, decl.sort));
  }

  int num_branch_nodes = 0;
  int level, num_nodes_at_level;
  for (level = 0, num_nodes_at_level = 1; level < depth;
      level++, num_nodes_at_level *= arity) {
    num_branch_nodes += num_nodes_at_level;
  }
  int num_leaf_nodes = num_nodes_at_level;
  int num_total_nodes = num_branch_nodes + num_leaf_nodes;

  this->nodes.resize(num_total_nodes);
  this->root = &nodes[0];

  for (int i = 0; i < num_total_nodes; i++) {
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
    make_bt_bools(&nodes[i]);
  }

  for (int i = 0; i < num_total_nodes; i++) {
    make_sort_constraints(&nodes[i]);
  }
}

/*
z3::expr SketchFormula::interpret(
    std::shared_ptr<Model>,
    std::vector<object_value> const& vars)
{
  
}
*/

void SketchFormula::make_sort_bools(SFNode* node) {
  for (int i = 0; i < sorts.size() + 1; i++) {
    node->sort_bools.push_back(ctx.bool_const(name("sort").c_str()));
    for (int j = 0; j < i; j++) {
      z3::expr_vector vec(ctx);
      vec.push_back(node->sort_bools[i]);
      vec.push_back(node->sort_bools[j]);
      solver.add(!z3::mk_and(vec));
    }
  }
}

void SketchFormula::make_bt_bools(SFNode* node) {
  for (int i = 0; i < node_types.size() - 1; i++) {
    string na = name("bt");
    node->nt_bools.push_back(ctx.bool_const(na.c_str()));
    node->nt_bool_names.push_back(na);
  }
}

z3::expr SketchFormula::expr_is_sort(SFNode* node, lsort s) {
  if (dynamic_cast<BooleanSort*>(s.get())) {
    return node->sort_bools[node->sort_bools.size() - 1];
  } else if (UninterpretedSort* usort = dynamic_cast<UninterpretedSort*>(s.get())) {
    string name = usort->name;
    for (int i = 0; i < sorts.size(); i++) {
      if (dynamic_cast<UninterpretedSort*>(sorts[i].get())->name == name) {
        return node->sort_bools[i];
      }
    }
    assert(false);
  } else {
    assert(false);
  }
}

void SketchFormula::make_sort_constraints(SFNode* node) {
  vector<z3::expr> constraints;
  for (NodeType& nt : node_types) {
    if (node->is_leaf && nt.domain.size() > 0) {
      constraints.push_back(ctx.bool_val(false));
    } else {
      z3::expr_vector vec(ctx);
      vec.push_back(expr_is_sort(node, nt.range));
      for (int i = 0; i < nt.domain.size(); i++) {
        vec.push_back(expr_is_sort(node->children[i], nt.domain[i]));
      }
      constraints.push_back(z3::mk_and(vec));
    }
  }

  assert(constraints.size() >= 1);
  z3::expr e = constraints[constraints.size() - 1];
  
  for (int i = node_types.size() - 2; i >= 0; i--) {
    e = z3::ite(node->nt_bools[i], constraints[i], e);
  }

  solver.add(e);
}

value SketchFormula::to_value(z3::model& m) {
  get_bools(m);
  return this->node_to_value(this->root);
}

value SketchFormula::node_to_value(SFNode* node) {
  int j = node_types.size() - 1;
  for (int i = 0; i < node->nt_bools.size(); i++) {
    if (is_true(node->nt_bool_names[i])) {
      j = i;
      break;
    }
  }

  NodeType const& nt = node_types[j];
  switch (nt.btt) {
    case NTT::True: {
      return v_true();
    }
    case NTT::False: {
      return v_false();
    }
    case NTT::And: {
      assert(!node->is_leaf);
      return v_and({
        node_to_value(node->children[0]),
        node_to_value(node->children[1])});
    }
    case NTT::Or: {
      assert(!node->is_leaf);
      return v_or({
        node_to_value(node->children[0]),
        node_to_value(node->children[1])});
    }
    case NTT::Not: {
      assert(!node->is_leaf);
      return v_not(node_to_value(node->children[0]));
    }
    case NTT::Eq: {
      assert(!node->is_leaf);
      return v_eq(
        node_to_value(node->children[0]),
        node_to_value(node->children[1]));
    }
    case NTT::Func: {
      assert(!node->is_leaf);
      vector<value> args;
      assert(nt.domain.size() <= node->children.size());
      for (int i = 0; i < nt.domain.size(); i++) {
        args.push_back(node_to_value(node->children[i]));
      }
      return v_apply(
          v_const(functions[nt.index].name, functions[nt.index].sort),
          args);
    }
    case NTT::Var: {
      assert(!node->is_leaf);
      return v_const(free_vars[nt.index].name, free_vars[nt.index].sort);
    }

    default:
      assert(false);
  }
}

void SketchFormula::get_bools(z3::model model) {
  bool_map.clear();
  int n = model.num_consts();
  z3::expr e_true = ctx.bool_val(true);
  z3::expr e_false = ctx.bool_val(false);
  for (int i = 0; i < n; i++) {
    z3::func_decl fd = model.get_const_decl(i);
    z3::expr res = model.get_const_interp(fd);
    if (eq(res, e_true)) {
      bool_map.insert(make_pair(fd.name().str(), true));
    } else if (eq(res, e_false)) {
      bool_map.insert(make_pair(fd.name().str(), false));
    } else {
      assert(false);
    }
  }
}

bool SketchFormula::is_true(std::string name) {
  auto iter = bool_map.find(name);
  assert(iter != bool_map.end());
  return iter->second;
}
