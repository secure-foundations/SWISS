#ifndef CONTEXTS_H
#define CONTEXTS_H

#include <unordered_map>
#include <string>

#include "z3++.h"

#include "logic.h"

/**
 * Contains a mapping from Sorts to z3 sorts.
 */

class BackgroundContext {
public:
  z3::context& ctx;
  z3::solver solver;
  std::unordered_map<std::string, z3::sort> sorts;

  BackgroundContext(z3::context& ctx, std::shared_ptr<Module> module);

  z3::sort getUninterpretedSort(std::string name);
  z3::sort getSort(std::shared_ptr<Sort> sort);
};

/**
 * Contains a mapping from functions to z3 function decls.
 */
class ModelEmbedding {
public:
  std::shared_ptr<BackgroundContext> ctx;
  std::unordered_map<std::string, z3::func_decl> mapping;

  ModelEmbedding(
      std::shared_ptr<BackgroundContext> ctx,
      std::unordered_map<std::string, z3::func_decl> const& mapping)
      : ctx(ctx), mapping(mapping) { }

  static std::shared_ptr<ModelEmbedding> makeEmbedding(
      std::shared_ptr<BackgroundContext> ctx,
      std::shared_ptr<Module> module);

  z3::func_decl getFunc(std::string) const;

  z3::expr value2expr(std::shared_ptr<Value>);
  z3::expr value2expr(std::shared_ptr<Value>,
      std::unordered_map<std::string, z3::expr> const& consts);
  z3::expr value2expr(std::shared_ptr<Value>,
      std::unordered_map<std::string, z3::expr> const& consts,
      std::unordered_map<std::string, z3::expr> const& vars);

  void dump();
};

class InductionContext {
public:
  std::shared_ptr<BackgroundContext> ctx;
  std::shared_ptr<ModelEmbedding> e1;
  std::shared_ptr<ModelEmbedding> e2;

  InductionContext(z3::context& ctx, std::shared_ptr<Module> module);
};

class InitContext {
public:
  std::shared_ptr<BackgroundContext> ctx;
  std::shared_ptr<ModelEmbedding> e;

  InitContext(z3::context& ctx, std::shared_ptr<Module> module);
};

class ConjectureContext {
public:
  std::shared_ptr<BackgroundContext> ctx;
  std::shared_ptr<ModelEmbedding> e;

  ConjectureContext(z3::context& ctx, std::shared_ptr<Module> module);
};

#endif
