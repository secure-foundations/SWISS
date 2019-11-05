#ifndef SYNTH_ENUMERATOR_H
#define SYNTH_ENUMERATOR_H

#include "model.h"
#include "sat_solver.h"
#include "top_quantifier_desc.h"
#include "sketch.h"

struct Options {
  int arity;
  int depth;
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
  CandidateSolver(std::shared_ptr<Module>, Options const&,
      bool ensure_nonredundant, Shape shape);

  value getNext();
  void addCounterexample(Counterexample cex, value candidate);
  void addExistingInvariant(value inv);

private:
  std::vector<std::pair<Counterexample, value>> cexes;
  std::vector<value> existingInvariants;

  std::shared_ptr<Module> module;
  TopQuantifierDesc tqd;
  SatSolver ss;
  SketchFormula sf;

  bool ensure_nonredundant;
  void add_model_gen_constraints();
};

#endif
