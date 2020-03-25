#ifndef SYNTH_ENUMERATOR_H
#define SYNTH_ENUMERATOR_H

#include "model.h"
#include "sat_solver.h"
#include "top_quantifier_desc.h"
#include "sketch.h"

struct EnumOptions {
  // SAT solving
  int arity;
  int depth;
  bool conj;

  // Naive solving
  int disj_arity;
  int conj_arity;
  bool impl_shape;
  //bool strat2;
  bool strat_alt;
};

struct Options {
  bool enum_sat;

  bool get_space_size;

  // If true, generate X such that X & conj is invariant
  // otherwise, generate X such X is invariant and X ==> conj
  bool with_conjs;

  bool filter_redundant;

  bool whole_space;

  bool pre_bmc;
  bool post_bmc;
  bool minimal_models;
};

struct Counterexample {
  // is_true
  std::shared_ptr<Model> is_true;

  // not (is_false)
  std::shared_ptr<Model> is_false;

  // hypothesis ==> conclusion
  std::shared_ptr<Model> hypothesis;
  std::shared_ptr<Model> conclusion;

  bool none;

  Counterexample() : none(false) { }

  json11::Json to_json() const;
  static Counterexample from_json(json11::Json, std::shared_ptr<Module>);
};

class CandidateSolver {
public:
  virtual ~CandidateSolver() {}

  virtual value getNext() = 0;
  virtual void addCounterexample(Counterexample cex, value candidate) = 0;
  virtual void addExistingInvariant(value inv) = 0;
  virtual long long getProgress() = 0;
  virtual long long getSpaceSize() = 0;
};

std::shared_ptr<CandidateSolver> make_sat_candidate_solver(
    std::shared_ptr<Module> module, EnumOptions const& options,
      bool ensure_nonredundant);

std::shared_ptr<CandidateSolver> make_naive_candidate_solver(
    std::shared_ptr<Module> module, EnumOptions const& options);

inline std::shared_ptr<CandidateSolver> make_candidate_solver(
    std::shared_ptr<Module> module, bool enum_sat, 
    EnumOptions const& options,
    bool ensure_nonredundant)
{
  if (enum_sat) {
    return make_sat_candidate_solver(module, options, ensure_nonredundant);
  } else {
    return make_naive_candidate_solver(module, options);
  }
}

std::shared_ptr<CandidateSolver> compose_candidate_solvers(
  std::vector<std::shared_ptr<CandidateSolver>> solvers);

#endif
