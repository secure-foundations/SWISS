#ifndef SYNTH_ENUMERATOR_H
#define SYNTH_ENUMERATOR_H

#include "model.h"
#include "top_quantifier_desc.h"
#include "template_counter.h"

#include <string>

struct Options {
  bool get_space_size;

  // If true, generate X such that X & conj is invariant
  // otherwise, generate X such X is invariant and X ==> conj
  bool with_conjs;
  bool breadth_with_conjs;

  bool filter_redundant;

  bool whole_space;

  bool pre_bmc;
  bool post_bmc;
  bool minimal_models;

  bool non_accumulative;

  //int threads;

  std::string invariant_log_filename;
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

  bool is_valid() const {
    return none || is_true || is_false || (hypothesis && conclusion);
  }
};

class CandidateSolver {
public:
  virtual ~CandidateSolver() {}

  virtual value getNext() = 0;
  virtual void addCounterexample(Counterexample cex) = 0;
  virtual void addExistingInvariant(value inv) = 0;
  virtual void addRedundantDesc(std::vector<int> const&) = 0;
  virtual long long getProgress() = 0;
  virtual long long getPreSymmCount() = 0;
  virtual long long getSpaceSize() = 0;

  virtual void setSubSlice(TemplateSubSlice const&) = 0;
};

//std::shared_ptr<CandidateSolver> make_sat_candidate_solver(
//    std::shared_ptr<Module> module, EnumOptions const& options,
//      bool ensure_nonredundant);

//std::shared_ptr<CandidateSolver> make_naive_candidate_solver(
//    std::shared_ptr<Module> module, EnumOptions const& options);

/*inline std::shared_ptr<CandidateSolver> make_candidate_solver(
    std::shared_ptr<Module> module,
    EnumOptions const& options,
    bool ensure_nonredundant)
{
  return make_naive_candidate_solver(module, options);
}*/

std::shared_ptr<CandidateSolver> make_candidate_solver(
    std::shared_ptr<Module> module,
    std::vector<TemplateSubSlice> const& sub_slices, 
    bool ensure_nonredundant);

//std::shared_ptr<CandidateSolver> compose_candidate_solvers(
  //std::vector<std::shared_ptr<CandidateSolver>> const& solvers);

extern int numEnumeratedFilteredRedundantInvariants;

#endif
