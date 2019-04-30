#ifndef SKETCH_H
#define SKETCH_H

#include "logic.h"
#include "model.h"
#include "top_quantifier_desc.h"

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
  std::vector<z3::expr> nt_bools;
  std::vector<std::string> nt_bool_names;

  std::vector<z3::expr> sort_bools;

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
  z3::context& ctx;
  z3::solver& solver;

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
    z3::context& ctx,
    z3::solver& solver,
    TopQuantifierDesc const& tqd,
    std::shared_ptr<Module> module,
    int arity, int depth
  );

  z3::expr interpret(std::shared_ptr<Model> model, std::vector<object_value> const&,
      bool target_value);
  z3::expr interpret_not(std::shared_ptr<Model> model);
  z3::expr interpret_not(SketchModel& model);

  value to_value(z3::model& m);

  int get_bool_count() { return bool_count; }

private:

  value node_to_value(SFNode*);
  z3::expr expr_is_sort(SFNode* node, lsort s);

  void make_sort_bools(SFNode* node);
  void make_bt_bools(SFNode* node);
  void make_sort_constraints(SFNode* node);
  void add_lex_constraints();
  void add_constraint_for_no_outer_negation();

  std::map<std::string, bool> bool_map;
  void get_bools(z3::model model);
  bool is_true(std::string name);

  ValueVector to_value_vector(
    SFNode* node,
    std::shared_ptr<Model> model,
    SketchModel* sm,
    std::vector<VarEncoding> const& var_exprs);
  z3::expr case_by_node_type(SFNode*, std::vector<z3::expr> const&);
  z3::expr new_const(z3::expr e, std::string const& name);
  z3::expr new_const_impl(z3::expr e, std::string const& name);
  z3::expr get_vector_value_entry(ValueVector& vv,
      lsort s, object_value o);
  int get_sort_index(lsort s);

  z3::expr node_is_not(SFNode* a);
  z3::expr node_is_true(SFNode* a);
  z3::expr node_is_false(SFNode* a);
  z3::expr node_is_or(SFNode* a);
  z3::expr node_is_and(SFNode* a);
  z3::expr node_is_eq_or_ne(SFNode* a);
  z3::expr node_is_ntt(SFNode* a, NTT ntt);
  z3::expr node_is_var(SFNode* a, int var_index);
  z3::expr nodes_eq(SFNode* a, SFNode* b);
  z3::expr children_lex_le(SFNode* a, SFNode* b, int nchildren);
  z3::expr nodes_le(SFNode* a, SFNode* b);
  z3::expr node_types_eq(SFNode* a, SFNode* b);
  z3::expr children_ascending(SFNode* node);
  std::map<std::pair<SFNode*, SFNode*>, z3::expr> nodes_le_map;
  std::map<std::pair<SFNode*, SFNode*>, z3::expr> nodes_eq_map;
  void vv_constraints(ValueVector& vv, SFNode* node);

  void add_variable_ordering_constraints();
  void add_variable_ordering_constraints_for_variables(std::vector<int> const&);
  std::vector<SFNode*> post_order_traversal();
  void post_order_traversal_(SFNode* node, std::vector<SFNode*>& res);
  SFNode* get_node_latest_before_subtree_in_post_order(SFNode* node);
  void constrain_conj_disj_form();
  void constrain_node_as_and(SFNode* node);
  void constrain_node_as_or(SFNode* node);

  z3::expr bool_const(std::string const& name);
  int bool_count;

  z3::expr ftree_to_expr(
      z3::context& ctx,
      std::shared_ptr<FTree> ftree,
      std::vector<ValueVector>& children,
      NodeType& nt);

  z3::expr interpret_not(std::shared_ptr<Model> model, SketchModel* sm);
  std::vector<std::vector<VarEncoding>> get_all_var_exps_tree(std::shared_ptr<Model>, SketchModel*, int idx, std::vector<QRange> const&, std::string const&);
  VarEncoding make_existential_var_encoding(std::shared_ptr<Model>, SketchModel*, lsort, std::string const&);
  z3::expr encodings_not_eq(VarEncoding&, VarEncoding&);
};

#endif
