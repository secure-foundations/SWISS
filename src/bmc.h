#ifndef BMC_H
#define BMC_H

#include "contexts.h"
#include "logic.h"
#include "model.h"

// Bounded model checking for exactly k steps.
class FixedBMCContext {
  std::shared_ptr<Module> module;
  std::shared_ptr<BackgroundContext> ctx;
  std::shared_ptr<ModelEmbedding> e1;
  std::shared_ptr<ModelEmbedding> e2;

public:
  FixedBMCContext(z3::context& ctx, std::shared_ptr<Module> module, int k);
  bool is_exactly_k_invariant(value v);
  std::shared_ptr<Model> get_k_invariance_violation(value v);
  bool is_reachable(std::shared_ptr<Model> model);
};

// Bounded model checking for <= k steps.
class BMCContext {
  std::vector<std::shared_ptr<FixedBMCContext>> bmcs;

public:
  BMCContext(z3::context& ctx, std::shared_ptr<Module> module, int k);
  bool is_k_invariant(value v);
  std::shared_ptr<Model> get_k_invariance_violation(value v);
  bool is_reachable(std::shared_ptr<Model> model);
};


#endif
