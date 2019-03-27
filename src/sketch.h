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
  NTT btt;
  int index;
  
  std::vector<lsort> domain;
  lsort range;

  NodeType(NTT btt, int index,
      std::vector<lsort> const& domain, lsort range)
      : btt(btt) , index(index) , domain(domain), range(range) { }
};

struct SFNode {
  std::vector<SFNode*> children;
  bool is_leaf;
  std::vector<z3::expr> nt_bools;
  std::vector<std::string> nt_bool_names;

  std::vector<z3::expr> sort_bools;
};

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

  z3::expr interpret(std::vector<z3::expr>);
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
};

#endif
