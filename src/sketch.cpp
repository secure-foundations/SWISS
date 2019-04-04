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

  printf("depth = %d, arity = %d\n", depth, arity);

  for (string sort_name : module->sorts) {
    lsort s = s_uninterp(sort_name);
    this->sorts.push_back(s);
  }

  this->functions = module->functions;

  lsort bool_sort = s_bool();

  vector<lsort> bool_sorts_of_arity;
  for (int i = 0; i < arity; i++) {
    bool_sorts_of_arity.push_back(bool_sort);
  }

  this->node_types = {
    NodeType("true", NTT::True, -1, {}, bool_sort),
    NodeType("false", NTT::False, -1, {}, bool_sort),
    NodeType("and", NTT::And, -1, bool_sorts_of_arity, bool_sort),
    NodeType("or", NTT::Or, -1, bool_sorts_of_arity, bool_sort),
    NodeType("not", NTT::Not, -1, {bool_sort}, bool_sort)
  };
  for (lsort s : this->sorts) {
    this->node_types.push_back(NodeType(
        "eq_" + dynamic_cast<UninterpretedSort*>(s.get())->name,
        NTT::Eq, -1, {s, s}, bool_sort));
  }
  for (int i = 0; i < module->functions.size(); i++) {
    VarDecl const& decl = module->functions[i];
    if (decl.sort->get_domain_as_function().size() <= arity) {
      this->node_types.push_back(NodeType("func_" + iden_to_string(decl.name),
        NTT::Func, i,
        decl.sort->get_domain_as_function(),
        decl.sort->get_range_as_function()));
    }
  }
  for (int i = 0; i < free_vars.size(); i++) {
    VarDecl& decl = free_vars[i];
    this->node_types.push_back(NodeType("var_" + iden_to_string(decl.name),
        NTT::Var, i, {}, decl.sort));
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

  nodes[0].name = "r";
  for (int i = 0; i < num_total_nodes; i++) {
    nodes[i].is_leaf = (i >= num_branch_nodes);
    if (!nodes[i].is_leaf) {
      for (int j = 0; j < arity; j++) {
        assert(i * arity + j < nodes.size());
        int c = i * arity + j + 1;
        nodes[i].children.push_back(&nodes[c]);
        nodes[c].name = nodes[i].name + to_string(j);
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
  solver.add(expr_is_sort(root, bool_sort));

  //add_constraint_for_no_outer_negation();
  //add_lex_symmetries();
}

void SketchFormula::make_sort_bools(SFNode* node) {
  for (int i = 0; i < sorts.size() + 1; i++) {
    string na = name(node->name + (i == sorts.size() ? "sort_bool" : "sort_" +
        dynamic_cast<UninterpretedSort*>(sorts[i].get())->name));
    node->sort_bools.push_back(ctx.bool_const(na.c_str()));
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
    string na = node->name + "_nt_" + node_types[i].name;
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
  switch (nt.ntt) {
    case NTT::True: {
      return v_true();
    }
    case NTT::False: {
      return v_false();
    }
    case NTT::And:
    case NTT::Or: {
      assert(!node->is_leaf);
      vector<value> args;
      for (int i = 0; i < nt.domain.size(); i++) {
        args.push_back(node_to_value(node->children[i]));
      }
      return (nt.ntt == NTT::And ? v_and(args) : v_or(args));
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
      return v_var(free_vars[nt.index].name, free_vars[nt.index].sort);
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
  if (iter == bool_map.end()) {
    //printf("name: %s\n", name.c_str());
    //assert(false);
    return true;
  }
  return iter->second;
}

z3::expr z3_and(z3::context& ctx, z3::expr a, z3::expr b) {
  z3::expr_vector v(ctx);
  v.push_back(a);
  v.push_back(b);
  return z3::mk_and(v);
}

z3::expr z3_or(z3::context& ctx, z3::expr a, z3::expr b) {
  z3::expr_vector v(ctx);
  v.push_back(a);
  v.push_back(b);
  return z3::mk_or(v);
}

struct ValueVector {
  z3::expr is_true;
  vector<vector<z3::expr>> is_obj;

  ValueVector(z3::expr is_true) : is_true(is_true) { }
};

z3::expr SketchFormula::interpret(
    std::shared_ptr<Model> model,
    std::vector<object_value> const& vars)
{
  assert(vars.size() == free_vars.size());
  vector<vector<z3::expr>> var_exps;
  z3::expr const_true = ctx.bool_val(true);
  z3::expr const_false = ctx.bool_val(false);
  for (int i = 0; i < vars.size(); i++) {
    vector<z3::expr> v;
    int dsize = model->get_domain_size(free_vars[i].sort);
    for (int j = 0; j < dsize; j++) {
      v.push_back(j == vars[i] ? const_true : const_false);
    }
    var_exps.push_back(v);
  }
  ValueVector vv = to_value_vector(root, model, var_exps);
  return vv.is_true;
}

z3::expr SketchFormula::interpret_not_forall(
    std::shared_ptr<Model> model)
{
  vector<vector<z3::expr>> var_exps;
  for (int i = 0; i < free_vars.size(); i++) {
    vector<z3::expr> v;
    int dsize = model->get_domain_size(free_vars[i].sort);
    for (int j = 0; j < dsize; j++) {
      z3::expr e = ctx.bool_const(name("existential_var_" + iden_to_string(free_vars[i].name) +
          "_eq_" + to_string(j)).c_str());
      v.push_back(e);
    }

    z3::expr_vector vec(ctx);
    for (int j = 0; j < dsize; j++) {
      vec.push_back(v[j]);
      for (int k = j+1; k < dsize; k++) {
        solver.add(!(z3_and(ctx, v[j], v[k])));
      }
    }
    solver.add(z3::mk_or(vec));

    var_exps.push_back(v);
  }
  ValueVector vv = to_value_vector(root, model, var_exps);
  return !vv.is_true;
}

ValueVector SketchFormula::to_value_vector(
    SFNode* node,
    std::shared_ptr<Model> model,
    std::vector<std::vector<z3::expr>> const& var_exprs)
{
  vector<ValueVector> children;
  for (SFNode* child : node->children) {
    children.push_back(to_value_vector(child, model, var_exprs));
  }

  z3::expr const_true = ctx.bool_val(true);
  z3::expr const_false = ctx.bool_val(false);

  vector<z3::expr> v_is_true;
  vector<vector<vector<z3::expr>>> v_objs;
  vector<int> domain_sizes;
  for (int i = 0; i < sorts.size(); i++) {
    int domain_size = model->get_domain_size(sorts[i]);
    domain_sizes.push_back(domain_size);
    v_objs.push_back({});
    for (int j = 0; j < domain_size; j++) {
      v_objs[v_objs.size() - 1].push_back({});
    }
  }
  
  for (int nt_i = 0; nt_i < node_types.size(); nt_i++) {
    NodeType& nt = node_types[nt_i];

    if (node->is_leaf && nt.domain.size() > 0) {
      v_is_true.push_back(const_false);
      for (int i = 0; i < sorts.size(); i++) {
        for (int j = 0; j < domain_sizes[i]; j++) {
          v_objs[i][j].push_back(const_false);
        }
      }
      continue;
    }

    switch (nt.ntt) {
      case NTT::True:
      case NTT::False: {
        v_is_true.push_back(nt.ntt == NTT::True ? const_true : const_false);
        for (int i = 0; i < sorts.size(); i++) {
          for (int j = 0; j < domain_sizes[i]; j++) {
            v_objs[i][j].push_back(const_false);
          }
        }
        
        break;
      }

      case NTT::And:
      case NTT::Or: {
        z3::expr_vector vec(ctx);
        for (int i = 0; i < nt.domain.size(); i++) {
          vec.push_back(children[i].is_true);
        }

        v_is_true.push_back(nt.ntt == NTT::And ? z3::mk_and(vec) : z3::mk_or(vec));
        
        for (int i = 0; i < sorts.size(); i++) {
          for (int j = 0; j < domain_sizes[i]; j++) {
            v_objs[i][j].push_back(const_false);
          }
        }

        break;
      }

      case NTT::Not: {
        v_is_true.push_back(!children[0].is_true);
        
        for (int i = 0; i < sorts.size(); i++) {
          for (int j = 0; j < domain_sizes[i]; j++) {
            v_objs[i][j].push_back(const_false);
          }
        }

        break;
      }

      case NTT::Eq: {
        int sort_index = get_sort_index(nt.domain[0]);
        z3::expr_vector possibilities(ctx);
        for (int j = 0; j < domain_sizes[sort_index]; j++) {
            possibilities.push_back(z3_and(
                ctx, children[0].is_obj[sort_index][j], children[1].is_obj[sort_index][j]));
        }

        v_is_true.push_back(z3::mk_or(possibilities));

        for (int i = 0; i < sorts.size(); i++) {
          for (int j = 0; j < domain_sizes[i]; j++) {
            v_objs[i][j].push_back(const_false);
          }
        }

        break;
      }

      case NTT::Func: {
        if (dynamic_cast<BooleanSort*>(nt.range.get())) {
          z3::expr_vector arg_possibilities(ctx);
          for (FunctionEntry const& e : model->getFunctionEntries(this->functions[nt.index].name)) {
            if (e.res == 1) {
              z3::expr_vector arg_conjuncts(ctx);
              assert(e.args.size() == nt.domain.size());
              for (int i = 0; i < e.args.size(); i++) {
                arg_conjuncts.push_back(get_vector_value_entry(
                  children[i], nt.domain[i], e.args[i]));
              }
              arg_possibilities.push_back(z3::mk_and(arg_conjuncts));
            }
          }

          v_is_true.push_back(z3::mk_or(arg_possibilities));

          for (int i = 0; i < sorts.size(); i++) {
            for (int j = 0; j < domain_sizes[i]; j++) {
              v_objs[i][j].push_back(const_false);
            }
          }
        } else {
          int sort_index = get_sort_index(nt.range);
          int dsize = domain_sizes[sort_index];
          vector<z3::expr_vector> arg_possibilities;
          for (int i = 0; i < dsize; i++) {
            arg_possibilities.push_back(z3::expr_vector(ctx));
          }

          for (FunctionEntry const& e : model->getFunctionEntries(this->functions[nt.index].name)) {
            z3::expr_vector arg_conjuncts(ctx);
            assert(e.args.size() == nt.domain.size());
            for (int i = 0; i < e.args.size(); i++) {
              arg_conjuncts.push_back(get_vector_value_entry(
                children[i], nt.domain[i], e.args[i]));
            }
            arg_possibilities[e.res].push_back(z3::mk_and(arg_conjuncts));
          }

          v_is_true.push_back(const_false); 

          for (int i = 0; i < sorts.size(); i++) {
            for (int j = 0; j < domain_sizes[i]; j++) {
              v_objs[i][j].push_back(i == sort_index ?
                  z3::mk_or(arg_possibilities[j]) : const_false);
            }
          }
        }

        break;
      }

      case NTT::Var: {
        int sort_index = get_sort_index(nt.range);

        assert(nt.index < var_exprs.size());

        v_is_true.push_back(const_false);
        for (int i = 0; i < sorts.size(); i++) {
          for (int j = 0; j < domain_sizes[i]; j++) {
            if (i == sort_index) {
              v_objs[i][j].push_back(var_exprs[nt.index][j]);
            } else {
              v_objs[i][j].push_back(const_false);
            }
          }
        }

        break;
      }

      default:
        assert(false);

    }
  }

  ValueVector vv(new_const(case_by_node_type(node, v_is_true),
      node->name + "_is_true"));
  for (int i = 0; i < sorts.size(); i++) {
    vv.is_obj.push_back({});
    for (int j = 0; j < domain_sizes[i]; j++) {
      vv.is_obj[i].push_back(new_const(case_by_node_type(node, v_objs[i][j]),
        node->name + "_is_" + dynamic_cast<UninterpretedSort*>(sorts[i].get())->name + "_" + to_string(j)
      ));
    }
  }

  return vv;
}

z3::expr SketchFormula::case_by_node_type(SFNode* node, std::vector<z3::expr> const& args)
{
  assert(args.size() == node_types.size());
  assert(node->nt_bools.size() == node_types.size() - 1);
  z3::expr res = args[args.size() - 1];
  for (int i = args.size() - 2; i >= 0; i--) {
    res = z3::ite(node->nt_bools[i], args[i], res);
  }
  return res;
}

z3::expr SketchFormula::new_const(z3::expr e, string const& na) {
  z3::expr b = ctx.bool_const(name(na).c_str());
  solver.add(b == e);
  return b;
}

z3::expr SketchFormula::get_vector_value_entry(ValueVector& vv,
    lsort s, object_value o)
{
  if (dynamic_cast<BooleanSort*>(s.get())) {
    if (o == 1) {
      return vv.is_true;
    } else {
      return !vv.is_true;
    }
  }
  else if (dynamic_cast<UninterpretedSort*>(s.get())) {
    int sort_index = get_sort_index(s);
    
    assert(sort_index != -1);
    assert(sort_index < vv.is_obj.size());
    assert(o < vv.is_obj[sort_index].size());
    return vv.is_obj[sort_index][o];
  }
  else {
    assert(false);
  }
}

int SketchFormula::get_sort_index(lsort s) {
  UninterpretedSort* usort = dynamic_cast<UninterpretedSort*>(s.get());
  assert(usort != NULL);
  for (int i = 0; i < sorts.size(); i++) {
    UninterpretedSort* usort2 = dynamic_cast<UninterpretedSort*>(sorts[i].get());
    assert(usort2 != NULL);
    if (usort2->name == usort->name) {
      return i;
    }
  }
  assert(false);
}

/*
void SketchFormula::add_constraint_for_no_outer_negation() {
  for (int idx = 0; idx < nodes.size(); idx++) {
    SFNode* node = &nodes[idx];
    if (!node->is_leaf) {
      solver.add(z3::implies(
        node_is_nt_not(node),
        !z3_or(ctx,
          node_is_nt_true(node->children[0]),
          node_is_nt_false(node->children[0]),
          node_is_nt_and(node->children[0]),
          node_is_nt_or(node->children[0])
        )));
    }
  }
}

void SketchFormula::add_lex_constraints() {
  for (int idx = 0; idx < nodes.size(); idx++) {
    SFNode* node = &nodes[idx];
    if (!node->is_leaf) {
      solver.add(z3::implies(
        node_is_nt_eq(node),
        nodes_le(node->children[0], node->children[1])
      );
      solver.add(z3::implies(
        node_is_nt_and(node),
        children_ascending(node)
      );
    }
  }
}

z3::expr SketchFormula::children_ascending(SFNode* node) {
  assert(!node->is_leaf);
  z3::expr_vector vec(ctx);
  for (int i = 1; i < node->children.size(); i++) {
    vec.push_back(nodes_le(node->children[i-1], node->children[i]));
  }
  return z3::mk_and(vec);
}

z3::expr SketchFormula::nodes_le(SFNode* a, SFNode* b) {
  assert(!(a->is_leaf ^ b->is_leaf));

  vector<SFNode*> as;
  vector<SFNode*> bs;
  get_pairs(a, b, as, bs);

  for (int i 
  z3::expr_vector vec(ctx);
  z3::expr_vector vec_eq(ctx);
  for (int i = 0; i < a->nt_bools.size(); i++) {
    vec.push_back(z3::implies(a->nt_bools[i], b->nt_bools[i]));
    if (!a->is_leaf) {
      vec_eq.push_back(a->nt_bools[i], !b->nt_bools[i]);
    }
  }
  if (a->is_leaf) {
    return z3::mk_and(vec);
  } else {
    vec_eq.push_back(a->nt_bools[nt_bools.size() - 1]);
    vec_eq.push_back(!b->nt_bools[0]);
    z3::expr eq_types z3::mk_or(vec_eq);
    return z3_and(ctx,
      z3::mk_and(vec),
      z3::implies(eq_types, 
    );
  }
}
*/
