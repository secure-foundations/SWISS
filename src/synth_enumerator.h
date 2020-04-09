#ifndef SYNTH_ENUMERATOR_H
#define SYNTH_ENUMERATOR_H

#include "model.h"
#include "sat_solver.h"
#include "top_quantifier_desc.h"
#include "sketch.h"

#include <string>

struct EnumOptions {
  int template_idx;

  // SAT solving
  int arity;
  int depth;
  bool conj;

  // Naive solving
  int disj_arity;
  int conj_arity;
  bool impl_shape; // overly-specific option
  bool depth2_shape;
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

  //int threads;
};

struct SpaceChunk {
  int major_idx;
  int tree_idx;
  int size;
  std::vector<int> nums;

  SpaceChunk() : major_idx(-1), tree_idx(-1), size(-1) { }

  std::string to_string() {
    std::string s = std::to_string(major_idx) + " "
        + std::to_string(size) + " "
        + std::to_string(tree_idx) + " "
        + std::to_string(nums.size());
    for (int i = 0; i < (int)nums.size(); i++) {
      s += std::to_string(nums[i]);
    }
    return s;
  }
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

  virtual void setSpaceChunk(SpaceChunk const&) = 0;
  virtual void getSpaceChunk(std::vector<SpaceChunk>&) = 0;
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

std::shared_ptr<CandidateSolver> make_candidate_solver(
    std::shared_ptr<Module> module, bool enum_sat, 
    std::vector<EnumOptions> const& options,
    bool ensure_nonredundant);

std::shared_ptr<CandidateSolver> compose_candidate_solvers(
  std::vector<std::shared_ptr<CandidateSolver>> const& solvers);

#endif
