#ifndef SYNTH_ENUMERATOR_H
#define SYNTH_ENUMERATOR_H

#include "model.h"
#include "sat_solver.h"
#include "top_quantifier_desc.h"
#include "sketch.h"

struct Options {
  // SAT solving
  int arity;
  int depth;

  // Naive solving
  int disj_arity;
  int conj_arity;
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
  virtual value getNext() = 0;
  virtual void addCounterexample(Counterexample cex, value candidate) = 0;
  virtual void addExistingInvariant(value inv) = 0;
};

class SatCandidateSolver : public CandidateSolver {
public:
  SatCandidateSolver(std::shared_ptr<Module>, Options const&,
      bool ensure_nonredundant, Shape shape);

  value getNext();
  void addCounterexample(Counterexample cex, value candidate);
  void addExistingInvariant(value inv);

private:
  std::vector<std::pair<Counterexample, value>> cexes;
  std::vector<value> existingInvariants;

  std::shared_ptr<Module> module;
  Shape shape;
  Options options;
  bool ensure_nonredundant;

  TopQuantifierDesc tqd;
  SatSolver ss;
  SketchFormula sf;

  void init_constraints();
};

class NaiveCandidateSolver : public CandidateSolver {
public:
  NaiveCandidateSolver(std::shared_ptr<Module>, Options const&,
      bool ensure_nonredundant, Shape shape);

  value getNext();
  void addCounterexample(Counterexample cex, value candidate);
  void addExistingInvariant(value inv);

//private:
  std::shared_ptr<Module> module;
  Shape shape;
  Options options;
  bool ensure_nonredundant;

  std::vector<value> values;
  std::vector<Counterexample> cexes;
  std::vector<int> cur_indices;

  std::vector<std::vector<std::pair<bool, bool>>> cached_evals;

  void increment();
  void dump_cur_indices();
};

#endif
