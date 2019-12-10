#ifndef BIG_DISJUNCT_SYNTH_ENUMERATOR_H
#define BIG_DISJUNCT_SYNTH_ENUMERATOR_H

#include "synth_enumerator.h"
#include "bitset_eval_result.h"

class BigDisjunctCandidateSolver : public CandidateSolver {
public:
  BigDisjunctCandidateSolver(std::shared_ptr<Module>, int disj_arity);

  value getNext();
  void addCounterexample(Counterexample cex, value candidate);
  void addExistingInvariant(value inv);

//private:
  std::shared_ptr<Module> module;
  int disj_arity;

  TopQuantifierDesc tqd;

  std::vector<value> pieces;
  std::vector<int> cur_indices;

  std::vector<Counterexample> cexes;
  std::vector<std::vector<std::pair<BitsetEvalResult, BitsetEvalResult>>> cex_results;

  std::vector<std::vector<int>> existing_invariant_indices;
  std::set<ComparableValue> existing_invariant_set;

  std::map<ComparableValue, int> piece_to_index;

  void increment();
  void dump_cur_indices();
  value disjunction_fuse(std::vector<value> values);
  std::vector<int> get_indices_of_value(value inv);
  int get_index_of_piece(value p);
  void init_piece_to_index();
};

#endif
