#ifndef SKETCH_MODEL_H
#define SKETCH_MODEL_H

#include "logic.h"
#include "model.h"

struct SketchFunction;
struct ValueVars;

class SketchModel {
public:
  SketchModel(
      z3::context& ctx, z3::solver& solver,
      std::shared_ptr<Model>, int n);
  void assert_formula(value);

  size_t get_domain_size(lsort);
  size_t get_domain_size(std::string);

private:
  z3::context& ctx;
  z3::solver& solver;

  std::shared_ptr<Model> model;
  std::map<std::string, size_t> domain_sizes;
  std::map<iden, SketchFunction> functions;

  z3::expr to_z3(value v, size_t res, std::map<iden, ValueVars> const& vars);
  z3::expr to_z3_forall_exists(size_t res, std::map<iden, ValueVars> const& vars, bool is_forall, std::vector<VarDecl> const& decls, value body);
  ValueVars make_value_vars_const(lsort, size_t);
};

struct ValueVars {
  lsort sort;
  int n;
  std::vector<z3::expr> exprs;

  z3::expr get(size_t i) const;
};

struct SketchFunctionEntry {
  std::vector<size_t> args;
  ValueVars res;
};

struct SketchFunction {
  std::vector<size_t> domain_sizes;
  std::vector<SketchFunctionEntry> table;
};

#endif
