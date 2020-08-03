#ifndef CONTEXTS_H
#define CONTEXTS_H

#include <unordered_map>
#include <string>

#include "smt.h"

#include "logic.h"

/**
 * Contains a mapping from Sorts to z3 sorts.
 */

class BackgroundContext {
public:
  smt::context ctx;
  smt::solver solver;
  std::unordered_map<std::string, smt::sort> sorts;

  BackgroundContext(smt::context& ctx, std::shared_ptr<Module> module);

  smt::sort getUninterpretedSort(std::string name);
  smt::sort getSort(std::shared_ptr<Sort> sort);
};

/**
 * Contains a mapping from functions to z3 function decls.
 */
class ModelEmbedding {
public:
  std::shared_ptr<BackgroundContext> ctx;
  std::unordered_map<iden, smt::func_decl> mapping;

  ModelEmbedding(
      std::shared_ptr<BackgroundContext> ctx,
      std::unordered_map<iden, smt::func_decl> const& mapping)
      : ctx(ctx), mapping(mapping) { }

  static std::shared_ptr<ModelEmbedding> makeEmbedding(
      std::shared_ptr<BackgroundContext> ctx,
      std::shared_ptr<Module> module);

  smt::func_decl getFunc(iden) const;

  smt::expr value2expr(std::shared_ptr<Value>);

  smt::expr value2expr(std::shared_ptr<Value>,
      std::unordered_map<iden, smt::expr> const& consts);

  smt::expr value2expr_with_vars(std::shared_ptr<Value>,
      std::unordered_map<iden, smt::expr> const& vars);

  smt::expr value2expr(std::shared_ptr<Value>,
      std::unordered_map<iden, smt::expr> const& consts,
      std::unordered_map<iden, smt::expr> const& vars);

  void dump();
};

class InductionContext {
public:
  std::shared_ptr<BackgroundContext> ctx;
  std::shared_ptr<ModelEmbedding> e1;
  std::shared_ptr<ModelEmbedding> e2;
  int action_idx;

  InductionContext(smt::context& ctx, std::shared_ptr<Module> module,
      int action_idx = -1);
  InductionContext(std::shared_ptr<BackgroundContext>, std::shared_ptr<Module> module,
      int action_idx = -1);

private:
  void init(std::shared_ptr<Module> module);
};

class ChainContext {
public:
  std::shared_ptr<BackgroundContext> ctx;
  std::vector<std::shared_ptr<ModelEmbedding>> es;

  ChainContext(smt::context& ctx, std::shared_ptr<Module> module, int numTransitions);
};

class ConjectureContext {
public:
  std::shared_ptr<BackgroundContext> ctx;
  std::shared_ptr<ModelEmbedding> e;

  ConjectureContext(smt::context& ctx, std::shared_ptr<Module> module);
};

class BasicContext {
public:
  std::shared_ptr<BackgroundContext> ctx;
  std::shared_ptr<ModelEmbedding> e;

  BasicContext(smt::context& ctx, std::shared_ptr<Module> module);
  BasicContext(std::shared_ptr<BackgroundContext>, std::shared_ptr<Module> module);
};

class InitContext {
public:
  std::shared_ptr<BackgroundContext> ctx;
  std::shared_ptr<ModelEmbedding> e;

  InitContext(smt::context& ctx, std::shared_ptr<Module> module);
  InitContext(std::shared_ptr<BackgroundContext>, std::shared_ptr<Module> module);

private:
  void init(std::shared_ptr<Module> module);
};

class InvariantsContext {
public:
  std::shared_ptr<BackgroundContext> ctx;
  std::shared_ptr<ModelEmbedding> e;

  InvariantsContext(smt::context& ctx, std::shared_ptr<Module> module);
};


std::string name(std::string const& basename);
std::string name(iden basename);

struct ActionResult {
  std::shared_ptr<ModelEmbedding> e;
  smt::expr constraint;

  ActionResult(
    std::shared_ptr<ModelEmbedding> e,
    smt::expr constraint)
  : e(e), constraint(constraint) { }
};

ActionResult applyAction(
    std::shared_ptr<ModelEmbedding> e,
    std::shared_ptr<Action> action,
    std::unordered_map<iden, smt::expr> const& consts);

bool is_satisfiable(std::shared_ptr<Module>, value);

bool is_complete_invariant(std::shared_ptr<Module>, value);

bool is_itself_invariant(std::shared_ptr<Module>, value);
bool is_itself_invariant(std::shared_ptr<Module>, std::vector<value>);

// Check if wpr^n(value) is inductive (no init check)
// Here, wpr includes the 'empty action'.
bool is_wpr_itself_inductive(std::shared_ptr<Module>, value, int wprIter);

bool is_invariant_with_conjectures(std::shared_ptr<Module>, value);
bool is_invariant_with_conjectures(std::shared_ptr<Module>, std::vector<value>);

bool is_invariant_wrt(std::shared_ptr<Module>, value invariant_so_far, value candidate);
bool is_invariant_wrt(std::shared_ptr<Module>, value invariant_so_far, std::vector<value> const& candidate);

bool is_invariant_wrt_tryhard(
    std::shared_ptr<Module>,
    value invariant_so_far,
    value candidate);

#endif
