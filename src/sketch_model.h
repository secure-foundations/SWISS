#ifndef SKETCH_MODEL_H
#define SKETCH_MODEL_H

#include "logic.h"
#include "model.h"
#include "sat_solver.h"

struct SketchFunction;
struct ValueVars;

class SketchModel {
public:
  SketchModel(
      SatSolver& solver,
      std::shared_ptr<Module>, int n);
  void assert_formula(value);

  size_t get_domain_size(lsort);
  size_t get_domain_size(std::string);

  int get_bool_count() { return bool_count; }

  std::shared_ptr<Model> to_model();

private:
  SatSolver& solver;
  std::shared_ptr<Module> module;

  std::map<std::string, size_t> domain_sizes;
  std::map<iden, SketchFunction> functions;

  sat_expr to_sat(value v, size_t res, std::map<iden, ValueVars> const& vars);
  sat_expr to_sat_forall_exists(size_t res, std::map<iden, ValueVars> const& vars, bool is_forall, std::vector<VarDecl> const& decls, value body);
  ValueVars make_value_vars_const(lsort, size_t);
  ValueVars make_value_vars_var(lsort sort, std::string const& name);

  std::map<std::pair<ComparableValue,size_t>, sat_expr> value_to_expr_map;

  object_value get_value_from_model(ValueVars& vv);

  sat_expr bool_const(std::string const& name);
  int bool_count;

  friend class SketchFormula;
};

struct ValueVars {
  lsort sort;
  int n;
  std::vector<sat_expr> exprs;

  int constant_value;

  sat_expr get(size_t i) const;
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
