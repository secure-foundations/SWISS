#ifndef SOLVE_H
#define SOLVE_H

#include "model.h"
#include "smt.h"
#include <functional>

struct ContextSolverResult {
  smt::SolverResult res;
  std::vector<std::shared_ptr<Model>> models;
};

enum class ModelType {
  Any,
  Min
};

enum class Strictness {
  Strict,
  Indef,
  Quick
};

ContextSolverResult context_solve(
    std::string const& log_info,
    std::shared_ptr<Module> module,
    ModelType mt,
    Strictness,
    value hint,
    std::function<
        std::vector<std::shared_ptr<ModelEmbedding>>(std::shared_ptr<BackgroundContext>)
      > f);

void context_solve_init();

#endif
