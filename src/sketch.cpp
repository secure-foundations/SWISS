#include "sketch.h"

#include <cassert>

#include "sketch_model.h"

using namespace std;

#define NEGATE_FUNCS true
#define USE_FTREE true

SketchFormula::SketchFormula(
    SatSolver& solver,
    TopQuantifierDesc const& tqd,
    shared_ptr<Module> module,
    int arity, int depth)
  : solver(solver), free_vars(tqd.decls()), tqd(tqd), bool_count(0)
{
  assert (2 <= arity);
  for (int i = 0; i < depth; i++) {
    arity_at_depth.push_back(arity);
  }
  int max_arity = arity;
  printf("depth = %d, arity = %d\n", depth, arity);

  for (string sort_name : module->sorts) {
    lsort s = s_uninterp(sort_name);
    this->sorts.push_back(s);
  }

  this->functions = module->functions;

  lsort bool_sort = s_bool();

  vector<lsort> bool_sorts_of_max_arity;
  for (int i = 0; i < max_arity; i++) {
    bool_sorts_of_max_arity.push_back(bool_sort);
  }

  this->node_types = {
    NodeType("true", NTT::True, -1, {}, bool_sort),
    NodeType("false", NTT::False, -1, {}, bool_sort),
    NodeType("and", NTT::And, -1, bool_sorts_of_max_arity, bool_sort),
    NodeType("or", NTT::Or, -1, bool_sorts_of_max_arity, bool_sort),
  };
  if (!NEGATE_FUNCS) {
    this->node_types.push_back(NodeType("not", NTT::Not, -1, {bool_sort}, bool_sort));
  }
  for (lsort s : this->sorts) {
    this->node_types.push_back(NodeType(
        "eq_" + dynamic_cast<UninterpretedSort*>(s.get())->name,
        NTT::Eq, -1, {s, s}, bool_sort));

    this->node_types.push_back(NodeType(
        "ne_" + dynamic_cast<UninterpretedSort*>(s.get())->name,
        NTT::Eq, -1, {s, s}, bool_sort));
    this->node_types[this->node_types.size() - 1].negated_function = true;
  }
  for (int i = 0; i < (int)module->functions.size(); i++) {
    VarDecl const& decl = module->functions[i];
    if ((int)decl.sort->get_domain_as_function().size() <= arity) {
      this->node_types.push_back(NodeType("func_" + iden_to_string(decl.name),
        NTT::Func, i,
        decl.sort->get_domain_as_function(),
        decl.sort->get_range_as_function()));
    }
  }
  for (int i = 0; i < (int)module->functions.size(); i++) {
    VarDecl const& decl = module->functions[i];
    if ((int)decl.sort->get_domain_as_function().size() <= arity &&
        dynamic_cast<BooleanSort*>(decl.sort->get_range_as_function().get())
        ) {
      this->node_types.push_back(NodeType("func_negated_" + iden_to_string(decl.name),
        NTT::Func, i,
        decl.sort->get_domain_as_function(),
        decl.sort->get_range_as_function()));
      this->node_types[this->node_types.size() - 1].negated_function = true;
    }
  }
  for (int i = 0; i < (int)free_vars.size(); i++) {
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
  nodes[0].parent = NULL;
  for (int i = 0; i < num_total_nodes; i++) {
    nodes[i].is_leaf = (i >= num_branch_nodes);
    if (!nodes[i].is_leaf) {
      for (int j = 0; j < arity; j++) {
        assert(i * arity + j < (int)nodes.size());
        int c = i * arity + j + 1;
        nodes[i].children.push_back(&nodes[c]);
        nodes[c].name = nodes[i].name + to_string(j);
        nodes[c].parent = &nodes[i];
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

  add_constraint_for_no_outer_negation();
  add_lex_constraints();
  //add_variable_ordering_constraints();
  //constrain_conj_disj_form();
}

void SketchFormula::make_sort_bools(SFNode* node) {
  for (int i = 0; i < (int)sorts.size() + 1; i++) {
    string na = name(node->name + (i == (int)sorts.size() ? "sort_bool" : "sort_" +
        dynamic_cast<UninterpretedSort*>(sorts[i].get())->name));
    node->sort_bools.push_back(bool_const(na));
    for (int j = 0; j < i; j++) {
      vector<sat_expr> vec;
      vec.push_back(node->sort_bools[i]);
      vec.push_back(node->sort_bools[j]);
      solver.add(sat_not(sat_and(vec)));
    }
  }
}

void SketchFormula::make_bt_bools(SFNode* node) {
  for (int i = 0; i < (int)node_types.size() - 1; i++) {
    string na = node->name + "_nt_" + node_types[i].name;
    node->nt_bools.push_back(bool_const(na));
  }
}

sat_expr SketchFormula::expr_is_sort(SFNode* node, lsort s) {
  if (dynamic_cast<BooleanSort*>(s.get())) {
    return node->sort_bools[node->sort_bools.size() - 1];
  } else if (UninterpretedSort* usort = dynamic_cast<UninterpretedSort*>(s.get())) {
    string name = usort->name;
    for (int i = 0; i < (int)sorts.size(); i++) {
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
  vector<sat_expr> constraints;
  for (NodeType& nt : node_types) {
    int arity = get_arity(nt, node);
    if (arity == -1) {
      constraints.push_back(sat_false());
    } else {
      vector<sat_expr> vec;
      vec.push_back(expr_is_sort(node, nt.range));
      for (int i = 0; i < arity; i++) {
        vec.push_back(expr_is_sort(node->children[i], nt.domain[i]));
      }
      constraints.push_back(sat_and(vec));
    }
  }

  assert(constraints.size() >= 1);
  sat_expr e = constraints[constraints.size() - 1];
  
  for (int i = node_types.size() - 2; i >= 0; i--) {
    e = sat_ite(node->nt_bools[i], constraints[i], e);
  }

  solver.add(e);
}

value SketchFormula::to_value() {
  return this->node_to_value(this->root);
}

value SketchFormula::node_to_value(SFNode* node) {
  int j = node_types.size() - 1;
  for (int i = 0; i < (int)node->nt_bools.size(); i++) {
    if (solver.get(node->nt_bools[i])) {
      j = i;
      break;
    }
  }

  NodeType const& nt = node_types[j];
  int arity = get_arity(nt, node);
  assert(arity >= 0);
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
      for (int i = 0; i < arity; i++) {
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
      value v = v_eq(
        node_to_value(node->children[0]),
        node_to_value(node->children[1]));
      return nt.negated_function ? v_not(v) : v;
    }
    case NTT::Func: {
      vector<value> args;
      assert(arity <= (int)node->children.size());
      for (int i = 0; i < arity; i++) {
        args.push_back(node_to_value(node->children[i]));
      }
      value res = v_apply(
          v_const(functions[nt.index].name, functions[nt.index].sort),
          args);
      if (nt.negated_function) {
        return v_not(res);
      } else {
        return res;
      }
    }
    case NTT::Var: {
      return v_var(free_vars[nt.index].name, free_vars[nt.index].sort);
    }

    default:
      assert(false);
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

sat_expr exactly_one(vector<sat_expr> const& bools) {
  vector<sat_expr> vec;
  for (int i = 0; i < (int)bools.size(); i++) {
    for (int j = i+1; j < (int)bools.size(); j++) {
      vec.push_back(sat_not(sat_and(bools[i], bools[j])));
    }
  }

  vector<sat_expr> at_least_one_vec;
  for (int i = 0; i < (int)bools.size(); i++) {
    at_least_one_vec.push_back(bools[i]);
  }

  vec.push_back(sat_or(at_least_one_vec));

  return sat_and(vec);
}

struct ValueVector {
  sat_expr is_true;
  sat_expr is_false;
  vector<vector<sat_expr>> is_obj;

  ValueVector(sat_expr is_true, sat_expr is_false) :
    is_true(is_true),
    is_false(is_false)
    { }
};

struct VarEncoding {
  vector<sat_expr> vars;
};

sat_expr SketchFormula::encodings_not_eq(VarEncoding& enc1, VarEncoding& enc2)
{
  assert(enc1.vars.size() == enc2.vars.size());
  if (enc1.vars.size() <= 1) {
    return sat_false();
  } else {
    /*
    // not sound
    vector<sat_expr> vec;
    for (int i = 0; i < enc1.vars.size(); i++) {
      vec.push_back(!sat_and(enc1.vars[i], enc2.vars[i]));
    }
    return sat_and(vec);
    */

    vector<sat_expr> vec;
    for (int i = 0; i < (int)enc1.vars.size(); i++) {
      vector<sat_expr> enc2_not_i;
      for (int j = 0; j < (int)enc2.vars.size(); j++) {
        if (i != j) {
          enc2_not_i.push_back(enc2.vars[j]);
        }
      }
      vec.push_back(sat_and(enc1.vars[i], sat_or(enc2_not_i)));
    }
    return sat_or(vec);
  }
}

sat_expr SketchFormula::interpret(
    std::shared_ptr<Model> model,
    std::vector<object_value> const& vars,
    bool target_value)
{
  assert(vars.size() == free_vars.size());
  vector<VarEncoding> var_exps;
  sat_expr const_true = sat_true();
  sat_expr const_false = sat_false();
  for (int i = 0; i < (int)vars.size(); i++) {
    vector<sat_expr> v;
    int dsize = model->get_domain_size(free_vars[i].sort);
    assert((int)vars[i] < dsize);
    for (int j = 0; j < dsize; j++) {
      v.push_back(j == (int)vars[i] ? const_true : const_false);
    }
    VarEncoding enc;
    enc.vars = v;
    var_exps.push_back(enc);
  }
  ValueVector vv = to_value_vector(root, model, NULL, var_exps);
  return vv.is_true;

  return target_value ? vv.is_true : vv.is_false;
}

VarEncoding SketchFormula::make_existential_var_encoding(
    shared_ptr<Model> model,
    SketchModel* sm,
    lsort so,
    string const& vname)
{
  vector<sat_expr> v;
  int dsize = model ? model->get_domain_size(so) : sm->get_domain_size(so);
  for (int j = 0; j < dsize; j++) {
    sat_expr e = bool_const(name("existential_var_" + vname + "_eq_" + to_string(j)));
    v.push_back(e);
  }

  vector<sat_expr> vec;
  for (int j = 0; j < dsize; j++) {
    vec.push_back(v[j]);
    for (int k = j+1; k < dsize; k++) {
      solver.add(sat_not(sat_and(v[j], v[k])));
    }
  }
  solver.add(sat_or(vec));

  VarEncoding eve;
  eve.vars = move(v);
  return eve;
}

template <typename A>
vector<A> concat_vector(vector<A> const& a, vector<A> const& b) {
  vector<A> res = a;
  for (A const& x : b) {
    res.push_back(x);
  }
  return res;
}

vector<vector<VarEncoding>> SketchFormula::get_all_var_exps_tree(
    shared_ptr<Model> model, SketchModel* sm,
    int idx, vector<QRange> const& qranges, string const& vname)
{
  if (idx == (int)qranges.size()) {
    vector<vector<VarEncoding>> res;
    res.push_back({});
    return res;
  }

  QRange const& qr = qranges[idx];

  if (qr.qtype == QType::Forall) {
    vector<VarEncoding> prefix;
    for (VarDecl decl : qr.decls) {
      prefix.push_back(make_existential_var_encoding(
          model, sm, decl.sort, iden_to_string(decl.name) + "_" + vname));
    }

    vector<vector<VarEncoding>> suffixes = get_all_var_exps_tree(model, sm, idx+1, qranges, vname);

    for (int i = 0; i < (int)suffixes.size(); i++) {
      suffixes[i] = concat_vector(prefix, suffixes[i]);
    }

    return suffixes;
  }
  else if (qr.qtype == QType::NearlyForall) {
    vector<VarEncoding> prefix1;
    vector<VarEncoding> prefix2;
    vector<sat_expr> not_all_eq;
    for (VarDecl decl : qr.decls) {
      VarEncoding enc1 =
          make_existential_var_encoding(model, sm, decl.sort, iden_to_string(decl.name) + "_" + vname + "0");
      VarEncoding enc2 =
          make_existential_var_encoding(model, sm, decl.sort, iden_to_string(decl.name) + "_" + vname + "1");
      prefix1.push_back(enc1);
      prefix2.push_back(enc2);

      not_all_eq.push_back(encodings_not_eq(enc1, enc2));
    }

    solver.add(sat_or(not_all_eq));

    vector<vector<VarEncoding>> suffixes1 =
        get_all_var_exps_tree(model, sm, idx+1, qranges, vname + "0");
    vector<vector<VarEncoding>> suffixes2 =
        get_all_var_exps_tree(model, sm, idx+1, qranges, vname + "1");

    for (int i = 0; i < (int)suffixes1.size(); i++) {
      suffixes1[i] = concat_vector(prefix1, suffixes1[i]);
    }
    for (int i = 0; i < (int)suffixes2.size(); i++) {
      suffixes2[i] = concat_vector(prefix2, suffixes2[i]);
    }

    return concat_vector(suffixes1, suffixes2);
  }
  else {
    assert(false);
  }
}

sat_expr SketchFormula::interpret_not(std::shared_ptr<Model> model) {
  return interpret_not(model, NULL);
}

sat_expr SketchFormula::interpret_not(SketchModel& sm) {
  return interpret_not(NULL, &sm);
}

sat_expr SketchFormula::interpret_not(shared_ptr<Model> model, SketchModel* sm)
{
  assert ((model != nullptr) ^ (sm != NULL));

  vector<QRange> qranges = tqd.with_foralls_grouped();
  vector<vector<VarEncoding>> all_var_exps =
      get_all_var_exps_tree(model, sm, 0, qranges, "path");
  vector<sat_expr> vec;
  for (vector<VarEncoding>& var_exps : all_var_exps) {
    ValueVector vv = to_value_vector(root, model, sm, var_exps);
    sat_expr e = vv.is_false;
    vec.push_back(e);
  }

  return sat_and(vec);
}

sat_expr SketchFormula::ftree_to_expr(
    shared_ptr<FTree> ft,
    vector<ValueVector>& children,
    NodeType& nt)
{
  switch (ft->type) {
    case FTree::Type::True: {
      return sat_true();
    }
    case FTree::Type::False: {
      return sat_false();
    }

    case FTree::Type::And: {
      return sat_and(
          ftree_to_expr(ft->left, children, nt),
          ftree_to_expr(ft->right, children, nt));
    }

    case FTree::Type::Or: {
      return sat_or(
          ftree_to_expr(ft->left, children, nt),
          ftree_to_expr(ft->right, children, nt));
    }

    case FTree::Type::Atom: {
      assert(0 <= ft->arg_idx && ft->arg_idx < (int)nt.domain.size());
      return get_vector_value_entry(
          children[ft->arg_idx],
          nt.domain[ft->arg_idx],
          ft->arg_value);
    }

    default:
      assert(false);
  }

}

ValueVector SketchFormula::to_value_vector(
    SFNode* node,
    std::shared_ptr<Model> model,
    SketchModel* sm,
    std::vector<VarEncoding> const& var_exprs)
{
  vector<ValueVector> children;
  for (SFNode* child : node->children) {
    children.push_back(to_value_vector(child, model, sm, var_exprs));
  }

  sat_expr const_true = sat_true();
  sat_expr const_false = sat_false();

  vector<sat_expr> v_is_true;
  vector<sat_expr> v_is_false;
  vector<vector<vector<sat_expr>>> v_objs;
  vector<int> domain_sizes;
  for (int i = 0; i < (int)sorts.size(); i++) {
    int domain_size = model ? model->get_domain_size(sorts[i]) : sm->get_domain_size(sorts[i]);
    domain_sizes.push_back(domain_size);
    v_objs.push_back({});
    for (int j = 0; j < domain_size; j++) {
      v_objs[v_objs.size() - 1].push_back({});
    }
  }
  
  for (int nt_i = 0; nt_i < (int)node_types.size(); nt_i++) {
    NodeType& nt = node_types[nt_i];
    int arity = get_arity(nt, node);

    if (arity == -1) {
      v_is_true.push_back(const_false);
      v_is_false.push_back(const_false);
      for (int i = 0; i < (int)sorts.size(); i++) {
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
        v_is_false.push_back(nt.ntt == NTT::False ? const_true : const_false);
        for (int i = 0; i < (int)sorts.size(); i++) {
          for (int j = 0; j < domain_sizes[i]; j++) {
            v_objs[i][j].push_back(const_false);
          }
        }
        
        break;
      }

      case NTT::And:
      case NTT::Or: {
        vector<sat_expr> vec;
        vector<sat_expr> vec_not;
        for (int i = 0; i < arity; i++) {
          vec.push_back(children[i].is_true);
          vec_not.push_back(children[i].is_false);
        }

        v_is_true.push_back(nt.ntt == NTT::And ? sat_and(vec) : sat_or(vec));
        v_is_false.push_back(nt.ntt == NTT::And ? sat_or(vec_not) : sat_and(vec_not));
        
        for (int i = 0; i < (int)sorts.size(); i++) {
          for (int j = 0; j < domain_sizes[i]; j++) {
            v_objs[i][j].push_back(const_false);
          }
        }

        break;
      }

      case NTT::Not: {
        v_is_true.push_back(children[0].is_false);
        v_is_false.push_back(children[0].is_true);
        
        for (int i = 0; i < (int)sorts.size(); i++) {
          for (int j = 0; j < domain_sizes[i]; j++) {
            v_objs[i][j].push_back(const_false);
          }
        }

        break;
      }

      case NTT::Eq: {
        int sort_index = get_sort_index(nt.domain[0]);
        vector<sat_expr> possibilities;
        for (int j = 0; j < domain_sizes[sort_index]; j++) {
            possibilities.push_back(sat_and(
                children[0].is_obj[sort_index][j], children[1].is_obj[sort_index][j]));
        }

        vector<sat_expr> possibilities_not;
        for (int i = 0; i < domain_sizes[sort_index]; i++) {
          vector<sat_expr> vec2;
          for (int j = 0; j < domain_sizes[sort_index]; j++) {
            if (i != j) {
              vec2.push_back(children[1].is_obj[sort_index][j]);
            }
          } 
          possibilities_not.push_back(sat_and(
              children[0].is_obj[sort_index][i],
              sat_or(vec2)));
        }

        if (nt.negated_function) {
          v_is_true.push_back(sat_or(possibilities_not));
          v_is_false.push_back(sat_or(possibilities));
        } else {
          v_is_true.push_back(sat_or(possibilities));
          v_is_false.push_back(sat_or(possibilities_not));
        }

        for (int i = 0; i < (int)sorts.size(); i++) {
          for (int j = 0; j < domain_sizes[i]; j++) {
            v_objs[i][j].push_back(const_false);
          }
        }

        break;
      }

      case NTT::Func: {
        if (USE_FTREE) {
          int range_size = model ? model->get_domain_size(nt.range) : sm->get_domain_size(nt.range);
          vector<sat_expr> exprs;
          for (int val = 0; val < range_size; val++) {
            if (model != nullptr) {
              shared_ptr<FTree> ftree = model->getFunctionFTree(
                  this->functions[nt.index].name, val);
              sat_expr e = ftree_to_expr(ftree, children, nt);
              exprs.push_back(e);
            } else {
              assert(sm != NULL);
              vector<sat_expr> possibilities;
              auto iter = sm->functions.find(this->functions[nt.index].name);
              assert (iter != sm->functions.end());
              for (SketchFunctionEntry& sfe : iter->second.table) {
                vector<sat_expr> vec;
                vec.push_back(sfe.res.get(val));
                for (int i = 0; i < (int)sfe.args.size(); i++) {
                  vec.push_back(get_vector_value_entry(children[i], nt.domain[i], sfe.args[i]));
                }
                possibilities.push_back(sat_and(vec));
              }
              exprs.push_back(sat_or(possibilities));
            }
          }

          if (dynamic_cast<BooleanSort*>(nt.range.get())) {
            if (nt.negated_function) {
              v_is_true.push_back(exprs[0]);
              v_is_false.push_back(exprs[1]);
            } else {
              v_is_true.push_back(exprs[1]);
              v_is_false.push_back(exprs[0]);
            }

            for (int i = 0; i < (int)sorts.size(); i++) {
              for (int j = 0; j < domain_sizes[i]; j++) {
                v_objs[i][j].push_back(const_false);
              }
            }
          } else {
            v_is_true.push_back(const_false); 
            v_is_false.push_back(const_false); 

            int sort_index = get_sort_index(nt.range);

            for (int i = 0; i < (int)sorts.size(); i++) {
              for (int j = 0; j < domain_sizes[i]; j++) {
                v_objs[i][j].push_back(i == sort_index ? exprs[j] : const_false);
              }
            }
          }
        } else {
          assert(false && "doesn't support sm");
          if (dynamic_cast<BooleanSort*>(nt.range.get())) {
            vector<sat_expr> arg_possibilities;
            vector<sat_expr> arg_possibilities_not;
            for (FunctionEntry const& e : model->getFunctionEntries(this->functions[nt.index].name)) {
              vector<sat_expr> arg_conjuncts;
              assert(e.args.size() == nt.domain.size());
              for (int i = 0; i < e.args.size(); i++) {
                arg_conjuncts.push_back(get_vector_value_entry(
                  children[i], nt.domain[i], e.args[i]));
              }
              if (e.res == (nt.negated_function ? 0 : 1)) {
                arg_possibilities.push_back(sat_and(arg_conjuncts));
              } else {
                arg_possibilities_not.push_back(sat_and(arg_conjuncts));
              }
            }

            v_is_true.push_back(sat_or(arg_possibilities));
            v_is_false.push_back(sat_or(arg_possibilities_not));

            for (int i = 0; i < sorts.size(); i++) {
              for (int j = 0; j < domain_sizes[i]; j++) {
                v_objs[i][j].push_back(const_false);
              }
            }
          } else {
            int sort_index = get_sort_index(nt.range);
            int dsize = domain_sizes[sort_index];
            vector<vector<sat_expr>> arg_possibilities;
            for (int i = 0; i < dsize; i++) {
              arg_possibilities.push_back(vector<sat_expr>());
            }

            for (FunctionEntry const& e : model->getFunctionEntries(this->functions[nt.index].name)) {
              vector<sat_expr> arg_conjuncts;
              assert(e.args.size() == nt.domain.size());
              for (int i = 0; i < e.args.size(); i++) {
                arg_conjuncts.push_back(get_vector_value_entry(
                  children[i], nt.domain[i], e.args[i]));
              }
              arg_possibilities[e.res].push_back(sat_and(arg_conjuncts));
            }

            v_is_true.push_back(const_false); 
            v_is_false.push_back(const_false); 

            for (int i = 0; i < sorts.size(); i++) {
              for (int j = 0; j < domain_sizes[i]; j++) {
                v_objs[i][j].push_back(i == sort_index ?
                    sat_or(arg_possibilities[j]) : const_false);
              }
            }
          }
        }

        break;
      }

      case NTT::Var: {
        int sort_index = get_sort_index(nt.range);

        assert(nt.index < (int)var_exprs.size());

        v_is_true.push_back(const_false);
        v_is_false.push_back(const_false);
        for (int i = 0; i < (int)sorts.size(); i++) {
          for (int j = 0; j < domain_sizes[i]; j++) {
            if (i == sort_index) {
              v_objs[i][j].push_back(var_exprs[nt.index].vars[j]);
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

  ValueVector vv(
      new_const_impl(case_by_node_type(node, v_is_true), node->name + "_is_true"),
      new_const_impl(case_by_node_type(node, v_is_false), node->name + "_is_false")
      );
  for (int i = 0; i < (int)sorts.size(); i++) {
    vv.is_obj.push_back({});
    for (int j = 0; j < domain_sizes[i]; j++) {
      vv.is_obj[i].push_back(new_const_impl(case_by_node_type(node, v_objs[i][j]),
        node->name + "_is_" + dynamic_cast<UninterpretedSort*>(sorts[i].get())->name + "_" + to_string(j)
      ));
    }
  }

  return vv;
}

sat_expr SketchFormula::case_by_node_type(SFNode* node, std::vector<sat_expr> const& args)
{
  assert(args.size() == node_types.size());
  assert(node->nt_bools.size() == node_types.size() - 1);
  sat_expr res = args[args.size() - 1];
  for (int i = args.size() - 2; i >= 0; i--) {
    res = sat_ite(node->nt_bools[i], args[i], res);
  }
  return res;
}

/*
sat_expr SketchFormula::new_const(sat_expr e, string const& na) {
  sat_expr b = bool_const(name(na));
  solver.add(b == e);
  return b;
}
*/

sat_expr SketchFormula::new_const_impl(sat_expr e, string const& na) {
  sat_expr b = bool_const(name(na));
  solver.add(sat_implies(b, e));
  return b;
}

sat_expr SketchFormula::get_vector_value_entry(ValueVector& vv,
    lsort s, object_value o)
{
  if (dynamic_cast<BooleanSort*>(s.get())) {
    if (o == 1) {
      return vv.is_true;
    } else {
      return sat_not(vv.is_true);
    }
  }
  else if (dynamic_cast<UninterpretedSort*>(s.get())) {
    int sort_index = get_sort_index(s);
    
    assert(sort_index != -1);
    assert(sort_index < (int)vv.is_obj.size());
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
  for (int i = 0; i < (int)sorts.size(); i++) {
    UninterpretedSort* usort2 = dynamic_cast<UninterpretedSort*>(sorts[i].get());
    assert(usort2 != NULL);
    if (usort2->name == usort->name) {
      return i;
    }
  }
  assert(false);
}

void SketchFormula::add_constraint_for_no_outer_negation() {
  if (NEGATE_FUNCS) {
    return;
  }
  for (int idx = 0; idx < nodes.size(); idx++) {
    SFNode* node = &nodes[idx];
    if (!node->is_leaf) {
      solver.add(sat_implies(
        node_is_not(node),
        sat_not(sat_or({
          node_is_true(node->children[0]),
          node_is_false(node->children[0]),
          node_is_and(node->children[0]),
          node_is_or(node->children[0]),
          node_is_not(node->children[0])
        }))));
    }
  }
}

void SketchFormula::add_lex_constraints() {
  for (int idx = 0; idx < (int)nodes.size(); idx++) {
    SFNode* node = &nodes[idx];
    if (!node->is_leaf) {
      solver.add(sat_implies(
        node_is_eq_or_ne(node),
        nodes_le(node->children[0], node->children[1])
      ));
      solver.add(sat_implies(
        node_is_and(node),
        children_ascending(node)
      ));
      solver.add(sat_implies(
        node_is_or(node),
        children_ascending(node)
      ));
    }
  }
}

sat_expr SketchFormula::children_ascending(SFNode* node) {
  assert(!node->is_leaf);
  vector<sat_expr> vec;
  for (int i = 1; i < (int)node->children.size(); i++) {
    vec.push_back(nodes_le(node->children[i-1], node->children[i]));
  }
  return sat_and(vec);
}

sat_expr SketchFormula::nodes_le(SFNode* a, SFNode* b) {
  auto iter = nodes_le_map.find(make_pair(a, b));
  if (iter != nodes_le_map.end()) {
    return iter->second;
  }

  assert(!(a->is_leaf ^ b->is_leaf));

  vector<sat_expr> vec;
  for (int i = 0; i < (int)a->nt_bools.size(); i++) {
    vec.push_back(sat_implies(b->nt_bools[i], a->nt_bools[i]));
  }

  if (!a->is_leaf) {
    vector<sat_expr> children_lex;
    for (int i = 0; i < (int)node_types.size(); i++) {
      int arity = get_arity(node_types[i], a);
      assert (arity == get_arity(node_types[i], b));
      if (i < (int)node_types.size() - 1) {
        children_lex.push_back(sat_implies(b->nt_bools[i], children_lex_le(a, b, arity)));
      } else {
        children_lex.push_back(children_lex_le(a, b, arity));
      }
    }
    vec.push_back(case_by_node_type(a, children_lex));
  }

  sat_expr res = new_const_impl(sat_and(vec), a->name + "_le_" + b->name);

  nodes_le_map.insert(make_pair(make_pair(a,b), res));
  return res;
}

sat_expr SketchFormula::children_lex_le(SFNode* a, SFNode* b, int nchildren) {
  assert(0 <= nchildren && nchildren <= (int)a->children.size() && nchildren <= (int)b->children.size());
  assert(!a->is_leaf);
  assert(!b->is_leaf);

  sat_expr res = sat_true();
  for (int i = nchildren - 1; i >= 0; i--) {
    res = sat_and(
        nodes_le(a->children[i], b->children[i]),
        sat_implies(
          nodes_eq(a->children[i], b->children[i]),
          res));
  }

  return res;
}

sat_expr SketchFormula::nodes_eq(SFNode* a, SFNode* b) {
  auto iter = nodes_eq_map.find(make_pair(a, b));
  if (iter != nodes_eq_map.end()) {
    return iter->second;
  }

  assert(!(a->is_leaf ^ b->is_leaf));

  vector<sat_expr> children_eq_per_node_type;
  for (int i = 0; i < (int)node_types.size(); i++) {
    int arity = get_arity(node_types[i], a);
    assert(arity == get_arity(node_types[i], b));
    if (arity != -1) {
      vector<sat_expr> children_eq;
      for (int j = 0; j < arity; j++) {
        children_eq.push_back(nodes_eq(a->children[j], b->children[j]));
      }
      children_eq_per_node_type.push_back(sat_and(children_eq));
    } else {
      children_eq_per_node_type.push_back(sat_false());
    }
  }

  sat_expr res = new_const_impl(sat_and(
      node_types_eq(a, b),
      case_by_node_type(a, children_eq_per_node_type)),
      a->name + "_eq_" + b->name);

  nodes_eq_map.insert(make_pair(make_pair(a,b), res));
  return res;
}

sat_expr SketchFormula::node_types_eq(SFNode* a, SFNode* b) {
  sat_expr res = sat_true();
  for (int i = node_types.size() - 2; i >= 0; i--) {
    res = sat_ite(a->nt_bools[i],
        b->nt_bools[i],
        sat_and(sat_not(b->nt_bools[i]), res));
  }
  return res;
}

sat_expr SketchFormula::node_is_ntt(SFNode* node, NTT ntt) {
  vector<sat_expr> vec;
  int i = 0;
  for (NodeType& nt : node_types) {
    if (nt.ntt == ntt) {
      if (i < (int)node_types.size() - 1) {
        vec.push_back(node->nt_bools[i]);
      }
      return sat_and(vec);
    } else {
      vec.push_back(sat_not(node->nt_bools[i]));
    }
    i++;
  }
  assert(false);
}

sat_expr SketchFormula::node_is_not(SFNode* a) {
  if (NEGATE_FUNCS) return sat_false();
  return node_is_ntt(a, NTT::Not);
}
sat_expr SketchFormula::node_is_true(SFNode* a) { return node_is_ntt(a, NTT::True); }
sat_expr SketchFormula::node_is_false(SFNode* a) { return node_is_ntt(a, NTT::False); }
sat_expr SketchFormula::node_is_or(SFNode* a) { return node_is_ntt(a, NTT::Or); }
sat_expr SketchFormula::node_is_and(SFNode* a) { return node_is_ntt(a, NTT::And); }

sat_expr SketchFormula::node_is_eq_or_ne(SFNode* node) {
  vector<sat_expr> vec;
  int i = 0;
  for (NodeType& nt : node_types) {
    if (nt.ntt == NTT::Eq) {
      vector<sat_expr> eq_vec;
      for (int j = i; j < (int)node_types.size(); j++) {
        if (node_types[j].ntt == NTT::Eq) {
          assert(j < (int)node->nt_bools.size());
          eq_vec.push_back(node->nt_bools[j]);
        } else {
          break;
        }
      }

      vec.push_back(sat_or(eq_vec));
      return sat_and(vec);
    } else {
      assert(i < (int)node->nt_bools.size());
      vec.push_back(sat_not(node->nt_bools[i]));
    }
    i++;
  }
  assert(false);
}

sat_expr SketchFormula::node_is_var(SFNode* node, int var_index) {
  vector<sat_expr> vec;
  int i = 0;
  for (NodeType& nt : node_types) {
    if (nt.ntt == NTT::Var && nt.index == var_index) {
      if (i < (int)node_types.size() - 1) {
        vec.push_back(node->nt_bools[i]);
      }
      return sat_and(vec);
    } else {
      vec.push_back(sat_not(node->nt_bools[i]));
    }
    i++;
  }
  assert(false);
}

/*
void SketchFormula::add_variable_ordering_constraints() {
  for (lsort so : this->sorts) {
    vector<int> var_indices;
    for (int i = 0; i < free_vars.size(); i++) {
      if (sorts_eq(free_vars[i].sort, so)) {
        var_indices.push_back(i);
      }
    }
    if (var_indices.size() > 1) {
      add_variable_ordering_constraints_for_variables(var_indices);
    }
  }

  //std::cout << solver << "\n";
}
*/

bool contains(vector<int> const& v, int t) {
  for (int i = 0; i < (int)v.size(); i++) {
    if (v[i] == t) return true;
  }
  return false;
}

/*
void SketchFormula::add_variable_ordering_constraints_for_variables(
    vector<int> const& var_indices)
{
  int m = var_indices.size();
  map<SFNode*, vector<sat_expr>> all_seen_vars;

  sat_expr const_false = sat_false();
  sat_expr const_true = sat_true();

  for (SFNode* node : post_order_traversal()) {
    SFNode* prev = get_node_latest_before_subtree_in_post_order(node);

    vector<sat_expr> emp;
    vector<sat_expr>& prev_seen_vars = (prev != NULL ? all_seen_vars.find(prev)->second : emp);

    vector<vector<sat_expr>> seen_var_parts;
    seen_var_parts.resize(m - 1);
    for (NodeType& nt : node_types) {
      int arity = get_arity(nt, node);
      if (nt.ntt == NTT::Var && contains(var_indices, nt.index)) {
        for (int i = 0; i < m-1; i++) {
          seen_var_parts[i].push_back(var_indices[i] == nt.index ? const_true :
              prev == NULL ? const_false : prev_seen_vars[i]);
        }
      } else if (arity > 0) {
        vector<sat_expr>& child_seen_vars =
            all_seen_vars.find(node->children[arity - 1])->second;
        for (int i = 0; i < m-1; i++) {
          seen_var_parts[i].push_back(child_seen_vars[i]);
        }
      } else {
        for (int i = 0; i < m-1; i++) {
          seen_var_parts[i].push_back(prev == NULL ? const_false : prev_seen_vars[i]);
        }
      }
    }

    vector<sat_expr> seen_vars;
    for (int i = 0; i < m-1; i++) {
      seen_vars.push_back(new_const(case_by_node_type(node, seen_var_parts[i]),
          node->name + "_seen_var_" + to_string(i)));
    }
    all_seen_vars.insert(make_pair(node, seen_vars));

    for (int i = 1; i < m; i++) {
      solver.add(sat_implies(
        node_is_var(node, var_indices[i]),
        prev == NULL ? const_false : prev_seen_vars[i-1]));
    }
  }
}
*/

vector<SFNode*> SketchFormula::post_order_traversal() {
  vector<SFNode*> res;
  post_order_traversal_(root, res);
  return res;
}

void SketchFormula::post_order_traversal_(SFNode* node, vector<SFNode*>& res) {
  for (SFNode* child : node->children) {
    post_order_traversal_(child, res);
  }
  res.push_back(node);
}

SFNode* SketchFormula::get_node_latest_before_subtree_in_post_order(SFNode* node) {
  SFNode* pa = node->parent;
  if (pa == NULL) {
    return NULL;
  }

  while (node == pa->children[0]) {
    pa = pa->parent;
    node = node->parent;
    if (pa == NULL) {
      return NULL;
    }
  }

  for (int i = 1; i < (int)pa->children.size(); i++) {
    if (pa->children[i] == node) {
      return pa->children[i-1];
    }
  }
  assert(false);
}

sat_expr SketchFormula::bool_const(std::string const& name) {
  bool_count++;
  return solver.new_sat_var(name.c_str());
}

void SketchFormula::constrain_conj_disj_form() {
  assert(2 <= arity_at_depth.size());

  constrain_node_as_and(root);
  for (SFNode* child : root->children) {
    constrain_node_as_or(child);
  }
}

void SketchFormula::constrain_disj_form() {
  assert(2 <= arity_at_depth.size());

  constrain_node_as_or(root);

  for (SFNode* child : root->children) {
    constrain_node_as_non_and_or(child);
  }
}

void SketchFormula::constrain_node_as_non_and_or(SFNode* node) {
  for (int i = 0; i < (int)node->nt_bools.size(); i++) {
    sat_expr e = node->nt_bools[i];
    if (node_types[i].ntt == NTT::And || node_types[i].ntt == NTT::Or) {
      solver.add(sat_not(node->nt_bools[i]));
    }
  }
}

void SketchFormula::constrain_node_as_and(SFNode* node) {
  for (int i = 0; i < (int)node->nt_bools.size(); i++) {
    sat_expr e = node->nt_bools[i];
    solver.add(node_types[i].ntt == NTT::And ? e : sat_not(e));
  }
}

void SketchFormula::constrain_node_as_or(SFNode* node) {
  for (int i = 0; i < (int)node->nt_bools.size(); i++) {
    sat_expr e = node->nt_bools[i];
    solver.add(node_types[i].ntt == NTT::Or ? e : sat_not(e));
  }
}
