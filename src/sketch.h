#ifndef SKETCH_H
#define SKETCH_H

#include "logic.h"
#include "model.h"

class enum BTT {
  True,
  False,
  And,
  Or,
  Not,
  Eq,
  Func,
};

struct BranchType {
  BTT btt;
  std::string func_name;
  
  std::vector<lsort> range;
  lsort domain;

  BranchType(BTT btt, std::string const& func_name,
      std::vector<lsort> const& range, domain)
      : btt(btt) , func_name(func_name) , range(range) , domain(domain) { }
};

struct Node {
  std::vector<Node*> children;
  bool is_leaf;
  std::vector<z3::expr> bt_bools;
  std::vector<z3::expr> lt_bools;
  std::vector<z3::expr> sort_bools;
};

class SketchFormula {
public:
  z3::context& ctx;
  z3::solver& solver;

  std::vector<lsort> sorts;
  std::vector<VarDecl> free_vars;
  int arity;
  int depth;
  vector<BranchType> branch_types;
  vector<Node> nodes;
  Node* root;

  SketchFormula(
    z3::context& ctx,
    z3::solver& solver,
    std::vector<VarDecl> free_vars,
    std::shared_ptr<Module> module,
    int arity, int depth
  );

  z3::expr interpret(std::vector<z3::expr>);
  value to_value();

private:
  value node_to_value(Node*);
};

#endif
