#ifndef ALT_SYNTH_ENUMERATOR_H
#define ALT_SYNTH_ENUMERATOR_H

#include "synth_enumerator.h"
#include "bitset_eval_result.h"
#include "var_lex_graph.h"
#include "subsequence_trie.h"
#include "template_desc.h"

class AltDisjunctCandidateSolver : public CandidateSolver {
public:
  AltDisjunctCandidateSolver(
      std::shared_ptr<Module>,
      TemplateSpace const& tspace);

  value getNext();
  void addCounterexample(Counterexample cex, value candidate);
  void addExistingInvariant(value inv);

  long long getProgress() { return progress; }
  long long getPreSymmCount();
  long long progress;

//private:
  std::shared_ptr<Module> module;
  int disj_arity;

  TemplateSpace tspace;
  TopAlternatingQuantifierDesc taqd;

  std::vector<value> pieces;

  TemplateSubSlice tss;
  std::vector<int> slice_index_map;

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

  std::vector<std::vector<int>> existing_invariant_indices;
  SubsequenceTrie existing_invariant_trie;

  std::set<ComparableValue> existing_invariant_set;

  std::map<ComparableValue, int> piece_to_index;

  TransitionSystem ts;

  void increment();
  void skipAhead(int upTo);
  void dump_cur_indices();
  value disjunction_fuse(std::vector<value> values);
  std::vector<int> get_indices_of_value(value inv);
  int get_index_of_piece(value p);
  void init_piece_to_index();
  void existing_invariants_append(std::vector<int> const& indices);

  void setSubSlice(TemplateSubSlice const&);
};

#endif
