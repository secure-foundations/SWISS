#ifndef SKETCH_H
#define SKETCH_H

#include "logic.h"
#include "model.h"

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
  
  std::vector<lsort> domain;
  lsort range;

  NodeType(std::string const& name, NTT ntt, int index,
      std::vector<lsort> const& domain, lsort range)
      : name(name) , ntt(ntt) , index(index) , domain(domain), range(range) { }
};

struct SFNode {
  std::vector<SFNode*> children;
  bool is_leaf;
  std::vector<z3::expr> nt_bools;
  std::vector<std::string> nt_bool_names;

  std::vector<z3::expr> sort_bools;

  std::string name;
};

struct ValueVector;

class SketchFormula {
public:
  z3::context& ctx;
  z3::solver& solver;

  std::vector<lsort> sorts;
  std::vector<VarDecl> functions;
  std::vector<VarDecl> free_vars;
  int arity;
  int depth;
  std::vector<NodeType> node_types;
  std::vector<SFNode> nodes;
  SFNode* root;

  SketchFormula(
    z3::context& ctx,
    z3::solver& solver,
    std::vector<VarDecl> free_vars,
    std::shared_ptr<Module> module,
    int arity, int depth
  );

  z3::expr interpret(std::shared_ptr<Model> model, std::vector<object_value> const&);
  value to_value(z3::model& m);

private:
  value node_to_value(SFNode*);
  z3::expr expr_is_sort(SFNode* node, lsort s);

  void make_sort_bools(SFNode* node);
  void make_bt_bools(SFNode* node);
  void make_sort_constraints(SFNode* node);

  std::map<std::string, bool> bool_map;
  void get_bools(z3::model model);
  bool is_true(std::string name);

  ValueVector to_value_vector(
    SFNode* node,
    std::shared_ptr<Model> model,
    std::vector<object_value> const&);
  z3::expr case_by_node_type(SFNode*, std::vector<z3::expr> const&);
  z3::expr new_const(z3::expr e, std::string const& name);
  z3::expr get_vector_value_entry(ValueVector& vv,
      lsort s, object_value o);
  int get_sort_index(lsort s);
};

#endif
