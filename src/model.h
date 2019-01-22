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

class Model {
public:
  bool eval_predicate(std::shared_ptr<Value> value) const;

  static std::shared_ptr<Model> extract_model_from_z3(
      z3::context& ctx,
      z3::solver& solver,
      std::shared_ptr<Module> module,
      ModelEmbedding const& e);

  void dump() const;
  std::string obj_to_string(Sort*, object_value) const;
  size_t get_domain_size(Sort*) const;
  size_t get_domain_size(std::string) const;
  FunctionInfo const& get_function_info(std::string) const;

private:
  std::shared_ptr<Module> module;

  std::unordered_map<std::string, SortInfo> sort_info;
  std::unordered_map<std::string, FunctionInfo> function_info;

  object_value eval(
    std::shared_ptr<Value> value,
    std::unordered_map<std::string, object_value> const&) const;

  object_value do_forall(
    Forall* value,
    std::unordered_map<std::string, object_value>&,
    int quantifier_index) const;
  object_value do_exists(
    Exists* value,
    std::unordered_map<std::string, object_value>&,
    int quantifier_index) const;
};

#endif
