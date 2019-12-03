#include "synth_enumerator.h"

#include <cassert>
#include <vector>
#include <map>

#include "enumerator.h"
#include "obviously_implies.h"

using namespace std;

class SimpleCandidateSolver : public CandidateSolver {
public:
  SimpleCandidateSolver(shared_ptr<Module>, int k, bool ensure_nonredundant);

  value getNext();
  void addCounterexample(Counterexample cex, value candidate);
  void addExistingInvariant(value inv);
  void dump_cur_indices();

  shared_ptr<Module> module;
  bool ensure_nonredundant;
  vector<bool> values_usable;
  shared_ptr<ValueList> values;

  int cur_idx;
};

std::shared_ptr<CandidateSolver> make_naive_candidate_solver(
    std::shared_ptr<Module> module, Options const& options,
      bool ensure_nonredundant, Shape shape)
{
  if (options.conj_arity == 1 && !options.impl_shape) {
    return shared_ptr<CandidateSolver>(new SimpleCandidateSolver(module, options.disj_arity, ensure_nonredundant));
  }
  assert(false);
}

SimpleCandidateSolver::SimpleCandidateSolver(shared_ptr<Module> module, int k, bool ensure_nonredundant)
  : module(module)
  , ensure_nonredundant(ensure_nonredundant)
{
  values = cached_get_filtered_values(module, k);
  if (ensure_nonredundant) {
    values->init_extra();
  }

  cout << "Using " << values->values.size() << " values that are a disjunction of at most " << k << " terms." << endl;

  values_usable.resize(values->values.size());
  for (int i = 0; i < values->values.size(); i++) {
    values_usable[i] = true;
  }

  cur_idx = 0;
}

void SimpleCandidateSolver::addCounterexample(Counterexample cex, value candidate)
{
  for (int j = 0; j < values->values.size(); j++) {
    if (values_usable[j]) {
      if (cex.is_true) {
        if (!cex.is_true->eval_predicate(values->values[j])) {
          values_usable[j] = false;
        }
      }
      else if (cex.is_false) {
        if (cex.is_false->eval_predicate(values->values[j])) {
          values_usable[j] = false;
        }
      }
      else {
        if (cex.hypothesis->eval_predicate(values->values[j]) &&
            !cex.conclusion->eval_predicate(values->values[j])) {
          values_usable[j] = false;
        }
      }
    }
  }
}

void SimpleCandidateSolver::addExistingInvariant(value inv)
{
  assert(ensure_nonredundant);
  ComparableValue cv(inv->totally_normalize());
  auto iter = values->normalized_to_idx.find(cv);
  assert(iter != values->normalized_to_idx.end());
  int idx = iter->second;
  assert(0 <= idx && idx < values->implications.size());

  for (int idx2 : values->implications[idx]) {
    values_usable[idx2] = false;
  }
}

void SimpleCandidateSolver::dump_cur_indices()
{
  cout << "cur_indices: " << cur_idx << " / " << values->values.size() << endl;
}

value SimpleCandidateSolver::getNext()
{
  while (true) {
    if (cur_idx >= values->values.size()) {
      return nullptr;
    }

    bool failed = false;

    if (!values_usable[cur_idx]) {
      failed = true;
    }

    if (!failed) {
      dump_cur_indices();
      value v = values->values[cur_idx];
      cur_idx++;
      return v;
    } else {
      cur_idx++;
    }
  }
  assert(false);
}
