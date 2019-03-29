#ifndef MODEL_H
#define MODEL_H

#include <vector>
#include <unordered_map>
#include "logic.h"
#include "contexts.h"
#include "z3++.h"

class SortInfo {
public:
  size_t domain_size;
};

typedef unsigned int object_value;

class FunctionTable {
public:
  std::vector<std::unique_ptr<FunctionTable>> children;
  object_value value;
};

class FunctionInfo {
public:
  object_value else_value;
  std::unique_ptr<FunctionTable> table;
};

class FunctionEntry {
public:
  std::vector<object_value> args;
  object_value res;
};

struct EvalExpr;

class Model {
public:
  bool eval_predicate(value) const;

  static std::shared_ptr<Model> extract_model_from_z3(
      z3::context& ctx,
      z3::solver& solver,
      std::shared_ptr<Module> module,
      ModelEmbedding const& e);

  void dump() const;
  std::string obj_to_string(Sort*, object_value) const;
  size_t get_domain_size(lsort) const;
  size_t get_domain_size(Sort*) const;
  size_t get_domain_size(std::string) const;
  FunctionInfo const& get_function_info(iden) const;

  void assert_model_is(std::shared_ptr<ModelEmbedding> e);
  void assert_model_is_not(std::shared_ptr<ModelEmbedding> e);
  void assert_model_does_not_have_substructure(std::shared_ptr<ModelEmbedding> e);

  void assert_model_is_or_isnt(std::shared_ptr<ModelEmbedding> e,
      bool exact, bool negate);

  std::vector<FunctionEntry> getFunctionEntries(iden name);

private:
  std::shared_ptr<Module> module;

  std::unordered_map<std::string, SortInfo> sort_info;
  std::unordered_map<iden, FunctionInfo> function_info;

public:
  EvalExpr value_to_eval_expr(
    std::shared_ptr<Value> v,
    std::vector<iden> const& names) const;
};

std::shared_ptr<Model> transition_model(
    z3::context& ctx,
    std::shared_ptr<Module> module,
    std::shared_ptr<Model> start_state,
    int which_action = -1);

std::vector<std::shared_ptr<Model>> get_tree_of_models(
    z3::context& ctx,
    std::shared_ptr<Module> module,
    std::shared_ptr<Model> start_state,
    int depth);

std::vector<std::shared_ptr<Model>> get_tree_of_models2(
  z3::context& ctx,
  std::shared_ptr<Module> module,
  int depth,
  int multiplicity,
  bool reversed = false // find bad models starting at NOT(safety condition)
);

struct QuantifierInstantiation {
  bool non_null;
  value formula;
  std::vector<VarDecl> decls;
  std::vector<object_value> variable_values;
  std::shared_ptr<Model> model;
};

struct Z3VarSet {
  std::vector<z3::expr> vars;
};

// Give a value of the form `forall A,B,...,Z . expr`, returns
// instantions of the variables A,B,...,Z such that the expression returns false
// (if one exists, that is, if the entire expression evaluates to false).
QuantifierInstantiation get_counterexample(std::shared_ptr<Model>, value);
bool eval_qi(QuantifierInstantiation const& qi, value);

/*
Z3VarSet add_existential_constraint(std::shared_ptr<ModelEmbedding>, value);
QuantifierInstantiation z3_var_set_2_quantifier_instantiation(
    Z3VarSet const&,
    z3::solver&,
    std::shared_ptr<Model>,
    value v);
*/

#endif
