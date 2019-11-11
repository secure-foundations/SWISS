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
  std::unordered_map<iden, z3::func_decl> mapping;

  ModelEmbedding(
      std::shared_ptr<BackgroundContext> ctx,
      std::unordered_map<iden, z3::func_decl> const& mapping)
      : ctx(ctx), mapping(mapping) { }

  static std::shared_ptr<ModelEmbedding> makeEmbedding(
      std::shared_ptr<BackgroundContext> ctx,
      std::shared_ptr<Module> module);

  z3::func_decl getFunc(iden) const;

  z3::expr value2expr(std::shared_ptr<Value>);

  z3::expr value2expr(std::shared_ptr<Value>,
      std::unordered_map<iden, z3::expr> const& consts);

  z3::expr value2expr_with_vars(std::shared_ptr<Value>,
      std::unordered_map<iden, z3::expr> const& vars);

  z3::expr value2expr(std::shared_ptr<Value>,
      std::unordered_map<iden, z3::expr> const& consts,
      std::unordered_map<iden, z3::expr> const& vars);

  void dump();
};

class InductionContext {
public:
  std::shared_ptr<BackgroundContext> ctx;
  std::shared_ptr<ModelEmbedding> e1;
  std::shared_ptr<ModelEmbedding> e2;

  InductionContext(z3::context& ctx, std::shared_ptr<Module> module);
};

class ChainContext {
public:
  std::shared_ptr<BackgroundContext> ctx;
  std::vector<std::shared_ptr<ModelEmbedding>> es;

  ChainContext(z3::context& ctx, std::shared_ptr<Module> module, int numTransitions);
};

class BasicContext {
public:
  std::shared_ptr<BackgroundContext> ctx;
  std::shared_ptr<ModelEmbedding> e;

  BasicContext(z3::context& ctx, std::shared_ptr<Module> module);
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

class InvariantsContext {
public:
  std::shared_ptr<BackgroundContext> ctx;
  std::shared_ptr<ModelEmbedding> e;

  InvariantsContext(z3::context& ctx, std::shared_ptr<Module> module);
};

std::string name(std::string basename);
std::string name(iden basename);

struct ActionResult {
  std::shared_ptr<ModelEmbedding> e;
  z3::expr constraint;

  ActionResult(
    std::shared_ptr<ModelEmbedding> e,
    z3::expr constraint)
  : e(e), constraint(constraint) { }
};

ActionResult applyAction(
    std::shared_ptr<ModelEmbedding> e,
    std::shared_ptr<Action> action,
    std::unordered_map<iden, z3::expr> const& consts);

bool is_complete_invariant(std::shared_ptr<Module>, value);

bool is_itself_invariant(std::shared_ptr<Module>, value);
bool is_itself_invariant(std::shared_ptr<Module>, std::vector<value>);

// Check if wpr^n(value) is inductive (no init check)
// Here, wpr includes the 'empty action'.
bool is_wpr_itself_inductive(std::shared_ptr<Module>, value, int wprIter);

bool is_invariant_with_conjectures(std::shared_ptr<Module>, value);
bool is_invariant_with_conjectures(std::shared_ptr<Module>, std::vector<value>);

// TODO better place for this?
void z3_set_timeout(z3::context&, int ms);

#endif
