#include "solve.h"

#include <iostream>
#include <cassert>

using namespace std;

extern int numRetries;

smt::context _z3_ctx_normal;
smt::context _z3_ctx_quick;
smt::context _cvc4_ctx_normal;
smt::context _cvc4_ctx_quick;

smt::context z3_ctx_normal() {
  if (!_z3_ctx_normal.p) {
    _z3_ctx_normal = smt::context(smt::Backend::z3);
    _z3_ctx_normal.set_timeout(45000);
  }
  return _z3_ctx_normal;
}

smt::context z3_ctx_quick() {
  if (!_z3_ctx_quick.p) {
    _z3_ctx_quick = smt::context(smt::Backend::z3);
    _z3_ctx_quick.set_timeout(15000);
  }
  return _z3_ctx_quick;
}

smt::context cvc4_ctx_normal() {
  if (!_cvc4_ctx_normal.p) {
    _cvc4_ctx_normal = smt::context(smt::Backend::cvc4);
    _cvc4_ctx_normal.set_timeout(45000);
  }
  return _cvc4_ctx_normal;
}

smt::context cvc4_ctx_quick() {
  if (!_cvc4_ctx_quick.p) {
    _cvc4_ctx_quick = smt::context(smt::Backend::cvc4);
    _cvc4_ctx_quick.set_timeout(15000);
  }
  return _cvc4_ctx_quick;
}


ContextSolverResult context_solve(
    std::string const& log_info,
    std::shared_ptr<Module> module,
    ModelType mt,
    Strictness st,
    value hint,
    std::function<
        std::vector<std::shared_ptr<ModelEmbedding>>(std::shared_ptr<BackgroundContext>)
      > f)
{
  int num_fails = 0; 
  while (true) {
    smt::context ctx = (num_fails % 2 == 0
        ? (st == Strictness::Quick ? z3_ctx_quick() : z3_ctx_normal())
        : (st == Strictness::Quick ? cvc4_ctx_quick() : cvc4_ctx_normal())
    );
    shared_ptr<BackgroundContext> bgc;
    bgc.reset(new BackgroundContext(ctx, module));
    vector<shared_ptr<ModelEmbedding>> es = f(bgc);

    bgc->solver.set_log_info(log_info);
    smt::SolverResult res = bgc->solver.check_result();

    if (st != Strictness::Strict || (res != smt::SolverResult::Unknown)) {
      ContextSolverResult csr;
      csr.res = res;
      if (es.size() > 0) {
        if (res == smt::SolverResult::Sat) {
          if (mt == ModelType::Any) {
            for (int i = 0; i < (int)es.size(); i++) {
              csr.models.push_back(Model::extract_model_from_z3(
                  bgc->ctx, bgc->solver, module, *es[i]));
            }
            csr.models[0]->dump_sizes();
          } else {
            csr.models = Model::extract_minimal_models_from_z3(
                bgc->ctx, bgc->solver, module, es, hint);
          }
        }
      }
      return csr;
    }

    num_fails++;
    numRetries++;
    assert(num_fails < 20);
    cout << "failure encountered, retrying" << endl;
  }
}
