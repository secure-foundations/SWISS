#ifndef ALT_DEPTH2_SYNTH_ENUMERATOR_H
#define ALT_DEPTH2_SYNTH_ENUMERATOR_H

#include "synth_enumerator.h"
#include "bitset_eval_result.h"
#include "var_lex_graph.h"
#include "subsequence_trie.h"
#include "tree_shapes.h"
#include "template_desc.h"

class AltDepth2CandidateSolver : public CandidateSolver {
public:
  AltDepth2CandidateSolver(
      std::shared_ptr<Module>,
      TemplateSpace const& tspce);

  value getNext();
  void addCounterexample(Counterexample cex, value candidate);
  void addExistingInvariant(value inv);
  long long getProgress() { return progress; }
  long long getPreSymmCount();
  long long getSpaceSize() { assert(false); }

//private:
  std::shared_ptr<Module> module;
  int total_arity;
  long long progress;

  TemplateSpace tspace;
  TopAlternatingQuantifierDesc taqd;

  std::vector<value> pieces;
  std::vector<TreeShape> tree_shapes;

  TemplateSubSlice tss;
  std::vector<int> slice_index_map;
  TransitionSystem sub_ts;
  int target_state;

  int tree_shape_idx;
  std::vector<int> cur_indices_sub;
  std::vector<int> cur_indices;
  std::vector<int> var_index_states;
  int start_from;
  int done_cutoff;
  bool finish_at_cutoff;
  bool done;

  std::vector<Counterexample> cexes;
  std::vector<std::vector<std::pair<BitsetEvalResult, BitsetEvalResult>>> cex_results;
  std::vector<std::pair<AlternationBitsetEvaluator, AlternationBitsetEvaluator>> abes;

  TransitionSystem ts;

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

  void setSubSlice(TemplateSubSlice const&);

  std::vector<uint64_t> evaluator_buf;
};

#endif
