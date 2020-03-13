#ifndef SYNTH_ENUMERATOR_H
#define SYNTH_ENUMERATOR_H

#include "model.h"
#include "sat_solver.h"
#include "top_quantifier_desc.h"
#include "sketch.h"

struct Options {
  bool enum_sat;
  bool enum_naive;

  // If true, generate X such that X & conj is invariant
  // otherwise, generate X such X is invariant and X ==> conj
  bool with_conjs;

  bool filter_redundant;

  bool whole_space;
  bool start_with_existing_conjectures;

  bool pre_bmc;
  bool post_bmc;

  // SAT solving
  int arity;
  int depth;

  // Naive solving
  int disj_arity;
  int conj_arity;
  bool impl_shape;
  bool strat2;
  bool strat_alt;
};

enum class Shape {
  SHAPE_DISJ,
  SHAPE_CONJ_DISJ
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
};

std::shared_ptr<CandidateSolver> make_sat_candidate_solver(
    std::shared_ptr<Module> module, Options const& options,
      bool ensure_nonredundant, Shape shape);

std::shared_ptr<CandidateSolver> make_naive_candidate_solver(
    std::shared_ptr<Module> module, Options const& options,
      bool ensure_nonredundant, Shape shape);

inline std::shared_ptr<CandidateSolver> make_candidate_solver(
    std::shared_ptr<Module> module, Options const& options,
      bool ensure_nonredundant, Shape shape)
{
  assert (options.enum_sat ^ options.enum_naive);

  if (options.enum_sat) {
    return make_sat_candidate_solver(module, options, ensure_nonredundant, shape);
  } else {
    return make_naive_candidate_solver(module, options, ensure_nonredundant, shape);
  }
}

#endif
