#ifndef SKETCH_H
#define SKETCH_H

#include "logic.h"
#include "model.h"
#include "top_quantifier_desc.h"
#include "sat_solver.h"

enum class NTT {
  True,
  False,
  And,
  Or,
  Not,
  Eq,
  Func,
  Var,
};

struct NodeType {
  std::string name;

  NTT ntt;
  int index;
  bool negated_function;
  
  std::vector<lsort> domain;
  lsort range;

  NodeType(std::string const& name, NTT ntt, int index,
      std::vector<lsort> const& domain, lsort range)
      : name(name) , ntt(ntt) , index(index) ,
        negated_function(false) ,
        domain(domain), range(range) { }
};

struct SFNode {
  std::vector<SFNode*> children;
  SFNode* parent;
  bool is_leaf;
  std::vector<sat_expr> nt_bools;

  std::vector<sat_expr> sort_bools;

  std::string name;
};

inline int get_arity(NodeType const& nt, SFNode* node) {
  if (nt.ntt == NTT::And || nt.ntt == NTT::Or) {
    if (node->is_leaf) return -1;
    else return node->children.size();
  } else {
    return node->children.size() >= nt.domain.size() ? nt.domain.size() : -1;
  }
}

struct ValueVector;
struct VarEncoding;
class SketchModel;

class SketchFormula {
public:
  SatSolver& solver;

  // tree shape
  std::vector<int> arity_at_depth;

  std::vector<lsort> sorts;
  std::vector<VarDecl> functions;
  std::vector<VarDecl> free_vars;
  TopQuantifierDesc tqd;
  std::vector<NodeType> node_types;
  std::vector<SFNode> nodes;
  SFNode* root;

  SketchFormula(
    SatSolver& solver,
    TopQuantifierDesc const& tqd,
    std::shared_ptr<Module> module,
    int arity, int depth
  );

  void constrain_conj_disj_form();
  void constrain_disj_form();

  sat_expr interpret(std::shared_ptr<Model> model, std::vector<object_value> const&,
      bool target_value);
  sat_expr interpret_not(std::shared_ptr<Model> model);
  sat_expr interpret_not(SketchModel& model);

  value to_value();

  int get_bool_count() { return bool_count; }

private:

  value node_to_value(SFNode*);
  sat_expr expr_is_sort(SFNode* node, lsort s);

  void make_sort_bools(SFNode* node);
  void make_bt_bools(SFNode* node);
  void make_sort_constraints(SFNode* node);
  void add_lex_constraints();
  void add_constraint_for_no_outer_negation();

  std::map<std::string, bool> bool_map;
  bool is_true(std::string name);

  ValueVector to_value_vector(
    SFNode* node,
    std::shared_ptr<Model> model,
    SketchModel* sm,
    std::vector<VarEncoding> const& var_exprs);
  sat_expr case_by_node_type(SFNode*, std::vector<sat_expr> const&);
  sat_expr new_const(sat_expr e, std::string const& name);
  sat_expr new_const_impl(sat_expr e, std::string const& name);
  sat_expr get_vector_value_entry(ValueVector& vv,
      lsort s, object_value o);
  int get_sort_index(lsort s);

  sat_expr node_is_not(SFNode* a);
  sat_expr node_is_true(SFNode* a);
  sat_expr node_is_false(SFNode* a);
  sat_expr node_is_or(SFNode* a);
  sat_expr node_is_and(SFNode* a);
  sat_expr node_is_eq_or_ne(SFNode* a);
  sat_expr node_is_ntt(SFNode* a, NTT ntt);
  sat_expr node_is_var(SFNode* a, int var_index);
  sat_expr nodes_eq(SFNode* a, SFNode* b);
  sat_expr children_lex_le(SFNode* a, SFNode* b, int nchildren);
  sat_expr nodes_le(SFNode* a, SFNode* b);
  sat_expr node_types_eq(SFNode* a, SFNode* b);
  sat_expr children_ascending(SFNode* node);
  std::map<std::pair<SFNode*, SFNode*>, sat_expr> nodes_le_map;
  std::map<std::pair<SFNode*, SFNode*>, sat_expr> nodes_eq_map;
  void vv_constraints(ValueVector& vv, SFNode* node);

  void add_variable_ordering_constraints();
  void add_variable_ordering_constraints_for_variables(std::vector<int> const&);
  std::vector<SFNode*> post_order_traversal();
  void post_order_traversal_(SFNode* node, std::vector<SFNode*>& res);
  SFNode* get_node_latest_before_subtree_in_post_order(SFNode* node);
  void constrain_node_as_and(SFNode* node);
  void constrain_node_as_or(SFNode* node);
  void constrain_node_as_non_and_or(SFNode* node);

  sat_expr bool_const(std::string const& name);
  int bool_count;

  sat_expr ftree_to_expr(
      std::shared_ptr<FTree> ftree,
      std::vector<ValueVector>& children,
      NodeType& nt);

  sat_expr interpret_not(std::shared_ptr<Model> model, SketchModel* sm);
  std::vector<std::vector<VarEncoding>> get_all_var_exps_tree(std::shared_ptr<Model>, SketchModel*, int idx, std::vector<QRange> const&, std::string const&);
  VarEncoding make_existential_var_encoding(std::shared_ptr<Model>, SketchModel*, lsort, std::string const&);
  sat_expr encodings_not_eq(VarEncoding&, VarEncoding&);
};

#endif
