#ifndef ALT_IMPL_SYNTH_ENUMERATOR_H
#define ALT_IMPL_SYNTH_ENUMERATOR_H

#include "synth_enumerator.h"
#include "bitset_eval_result.h"
#include "var_lex_graph.h"
#include "subsequence_trie.h"

class AltImplCandidateSolver : public CandidateSolver {
public:
  AltImplCandidateSolver(std::shared_ptr<Module>, int disj_arity);

  value getNext();
  void addCounterexample(Counterexample cex, value candidate);
  void addExistingInvariant(value inv);
  long long getProgress() { return progress; }
  long long getSpaceSize();

//private:
  std::shared_ptr<Module> module;
  int arity1;
  int arity2;
  int total_arity;
  long long progress;

  TopAlternatingQuantifierDesc taqd;

  std::vector<value> pieces;
  std::vector<int> cur_indices;
  bool done;

  std::vector<Counterexample> cexes;
  std::vector<std::vector<std::pair<BitsetEvalResult, BitsetEvalResult>>> cex_results;
  std::vector<std::pair<AlternationBitsetEvaluator, AlternationBitsetEvaluator>> abes;

  //std::vector<std::vector<int>> existing_invariant_indices;
  std::vector<SubsequenceTrie> existing_invariant_tries;

  std::set<ComparableValue> existing_invariant_set;

  std::map<ComparableValue, int> piece_to_index;

  std::vector<VarIndexState> var_index_states;
  std::vector<VarIndexTransition> var_index_transitions;

  void increment();
  void skipAhead(int upTo);
  void dump_cur_indices();
  value disjunction_fuse(std::vector<value> values);
  int get_index_for_and(std::vector<value> const& inv);
  std::pair<std::vector<int>, int> get_indices_of_value(value inv);
  std::vector<int> get_simple_indices(std::vector<int> const& v);
  int get_summary_index(std::vector<int> const& v);
  int get_index_of_piece(value p);
  void init_piece_to_index();
  void existing_invariants_append(std::pair<std::vector<int>, int> const& indices);

  void setup_abe1(AlternationBitsetEvaluator& abe, 
      std::vector<std::pair<BitsetEvalResult, BitsetEvalResult>> const& cex_result,
      std::vector<int> const& cur_indices);

  void setup_abe2(AlternationBitsetEvaluator& abe, 
      std::vector<std::pair<BitsetEvalResult, BitsetEvalResult>> const& cex_result,
      std::vector<int> const& cur_indices);
};

#endif
