#ifndef ALT_DEPTH2_SYNTH_ENUMERATOR_H
#define ALT_DEPTH2_SYNTH_ENUMERATOR_H

#include "synth_enumerator.h"
#include "bitset_eval_result.h"
#include "var_lex_graph.h"
#include "subsequence_trie.h"
#include "tree_shapes.h"

class AltDepth2CandidateSolver : public CandidateSolver {
public:
  AltDepth2CandidateSolver(std::shared_ptr<Module>, value templ, int total_arity);

  value getNext();
  void addCounterexample(Counterexample cex, value candidate);
  void addExistingInvariant(value inv);
  long long getProgress() { return progress; }
  long long getSpaceSize();
  long long getPreSymmCount();

//private:
  std::shared_ptr<Module> module;
  int total_arity;
  long long progress;

  TopAlternatingQuantifierDesc taqd;
  std::vector<value> pieces;

  std::vector<TreeShape> tree_shapes;

  int tree_shape_idx;
  std::vector<int> cur_indices;
  std::vector<VarIndexState> var_index_states;
  int start_from;
  int done_cutoff;
  bool finish_at_cutoff;
  bool done;

  std::vector<Counterexample> cexes;
  std::vector<std::vector<std::pair<BitsetEvalResult, BitsetEvalResult>>> cex_results;
  std::vector<std::pair<AlternationBitsetEvaluator, AlternationBitsetEvaluator>> abes;

  std::vector<VarIndexTransition> var_index_transitions;

  void increment();
  //void skipAhead(int upTo);
  void dump_cur_indices();

  value get_current_value();
  value get_clause(int);

  void setup_abe1(AlternationBitsetEvaluator& abe, 
      std::vector<std::pair<BitsetEvalResult, BitsetEvalResult>> const& cex_result,
      std::vector<int> const& cur_indices);

  void setup_abe2(AlternationBitsetEvaluator& abe, 
      std::vector<std::pair<BitsetEvalResult, BitsetEvalResult>> const& cex_result,
      std::vector<int> const& cur_indices);

  void setSpaceChunk(SpaceChunk const&);
  void getSpaceChunk(std::vector<SpaceChunk>&);

  std::vector<uint64_t> evaluator_buf;
};

#endif
