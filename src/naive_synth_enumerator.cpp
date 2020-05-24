#include "synth_enumerator.h"

#include <cassert>
#include <vector>
#include <map>

#include "enumerator.h"
#include "obviously_implies.h"
#include "big_disjunct_synth_enumerator.h"
#include "alt_synth_enumerator.h"
#include "alt_impl_synth_enumerator.h"
#include "alt_depth2_synth_enumerator.h"

using namespace std;

class SimpleCandidateSolver : public CandidateSolver {
public:
  SimpleCandidateSolver(shared_ptr<Module>, value templ, int k, bool ensure_nonredundant);

  value getNext();
  void addCounterexample(Counterexample cex, value candidate);
  void addExistingInvariant(value inv);
  void dump_cur_indices();

  long long getProgress() { return progress; }
  long long getSpaceSize() { assert(false); }
  long long progress;

  shared_ptr<Module> module;
  bool ensure_nonredundant;
  vector<bool> values_usable;
  shared_ptr<ValueList> values;

  int cur_idx;

  void setSpaceChunk(SpaceChunk const&) { assert(false); }
  void getSpaceChunk(std::vector<SpaceChunk>&) { assert(false); }
};

SimpleCandidateSolver::SimpleCandidateSolver(shared_ptr<Module> module, value templ, int k, bool ensure_nonredundant)
  : progress(0)
  , module(module)
  , ensure_nonredundant(ensure_nonredundant)
{
  cout << "Using SimpleCandidateSolver" << endl;

  values = cached_get_filtered_values(module, templ, k);

  values->init_simp();
  if (ensure_nonredundant) {
    values->init_extra();
  }

  cout << "Using " << values->values.size() << " values that are a disjunction of at most " << k << " terms." << endl;

  values_usable.resize(values->values.size());
  for (int i = 0; i < (int)values->values.size(); i++) {
    values_usable[i] = true;
  }

  cur_idx = 0;
}

void SimpleCandidateSolver::addCounterexample(Counterexample cex, value candidate)
{
  for (int j = 0; j < (int)values->values.size(); j++) {
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
  assert(0 <= idx && idx < (int)values->implications.size());

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
    progress++;

    if (cur_idx >= (int)values->values.size()) {
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

class ConjunctCandidateSolver : public CandidateSolver {
public:
  ConjunctCandidateSolver(shared_ptr<Module>, value templ,
      int conj_arity, int disj_arity);

  value getNext();
  void addCounterexample(Counterexample cex, value candidate);
  void addExistingInvariant(value inv);

  long long getProgress() { return progress; }
  long long getSpaceSize() { assert(false); }
  long long getPreSymmCount() { assert(false); }
  long long progress;

//private:
  shared_ptr<Module> module;
  int conj_arity;

  shared_ptr<ValueList> values;

  vector<Counterexample> cexes;
  vector<int> cur_indices;

  vector<vector<pair<bool, bool>>> cached_evals;

  void increment();
  void dump_cur_indices();

  void setSpaceChunk(SpaceChunk const&) { assert(false); }
  void getSpaceChunk(std::vector<SpaceChunk>&) { assert(false); }
};

ConjunctCandidateSolver::ConjunctCandidateSolver(shared_ptr<Module> module, value templ, int conj_arity, int disj_arity)
  : progress(0)
  , module(module)
  , conj_arity(conj_arity)
{
  cout << "Using ConjunctCandidateSolver" << endl;

  values = cached_get_filtered_values(module, templ, disj_arity);
  values->init_simp();

  cout << "Using " << values->values.size() << " values that are a disjunction of at most " << disj_arity << " terms." << endl;

  cur_indices = {};
}

void ConjunctCandidateSolver::addCounterexample(Counterexample cex, value candidate)
{
  assert (!cex.none);
  assert (cex.is_true || cex.is_false || (cex.hypothesis && cex.conclusion));

  cexes.push_back(cex);
  int i = cexes.size() - 1;

  cached_evals.push_back({});
  cached_evals[i].resize(values->values.size());

  for (int j = 0; j < (int)values->values.size(); j++) {
    if (cex.is_true) {
      cached_evals[i][j].first = true;
      // TODO if it's false, we never have to look at values->values[j] again.
      cached_evals[i][j].second = cex.is_true->eval_predicate(values->values[j]);
    }
    else if (cex.is_false) {
      cached_evals[i][j].first = cex.is_false->eval_predicate(values->values[j]);
      cached_evals[i][j].second = false;
    }
    else {
      cached_evals[i][j].first = cex.hypothesis->eval_predicate(values->values[j]);
      cached_evals[i][j].second = cex.conclusion->eval_predicate(values->values[j]);
    }
  }
}

void ConjunctCandidateSolver::addExistingInvariant(value inv) {
  assert(false);
}

void ConjunctCandidateSolver::dump_cur_indices()
{
  cout << "cur_indices:";
  for (int i : cur_indices) {
    cout << " " << i;
  }
  cout << " / " << values->values.size() << endl;
}

void ConjunctCandidateSolver::increment()
{
  int j;
  for (j = cur_indices.size() - 1; j >= 0; j--) {
    if (cur_indices[j] != (int)values->values.size() - (int)cur_indices.size() + j) {
      cur_indices[j]++;
      for (int k = j+1; k < (int)cur_indices.size(); k++) {
        cur_indices[k] = cur_indices[k-1] + 1;
      }
      break;
    }
  }
  if (j == -1) {
    cur_indices.push_back(0);
    assert (cur_indices.size() <= values->values.size());
    for (int i = 0; i < (int)cur_indices.size(); i++) {
      cur_indices[i] = i;
    }
  }
}

value ConjunctCandidateSolver::getNext()
{
  while (true) {
    if ((int)cur_indices.size() > this->conj_arity) {
      return nullptr;
    }

    progress++;

    bool failed = false;

    for (int i = 0; i < (int)cexes.size(); i++) {
      bool first_bool = true;
      bool second_bool = true;
      for (int k = 0; k < (int)cur_indices.size(); k++) {
        int j = cur_indices[k];
        first_bool = first_bool && cached_evals[i][j].first;
        second_bool = second_bool && cached_evals[i][j].second;
      }

      if (first_bool && !second_bool) {
        failed = true;
        break;
      }
    }

    if (!failed) {
      dump_cur_indices();

      vector<value> conjuncts;
      for (int i = 0; i < (int)cur_indices.size(); i++) {
        conjuncts.push_back(values->values[cur_indices[i]]);
      }
      value v = v_and(conjuncts);

      increment();
      progress++;

      return v;
    } else {
      increment();
      progress++;
    }
  }
  assert(false);
}

std::shared_ptr<CandidateSolver> make_naive_candidate_solver(
    std::shared_ptr<Module> module, EnumOptions const& options)
{
  assert (0 <= options.template_idx && options.template_idx < (int)module->templates.size());
  value templ = module->templates[options.template_idx];
  cout << "selecting template " << templ->to_string() << endl;

  //if (options.conj_arity == 1 && !options.impl_shape && !options.strat2 && !options.strat_alt) {
  //  return shared_ptr<CandidateSolver>(new SimpleCandidateSolver(module, options.disj_arity, ensure_nonredundant));
  //}
  if (options.conj_arity == 1 && !options.impl_shape && !options.strat_alt && !options.depth2_shape) {
    return shared_ptr<CandidateSolver>(new BigDisjunctCandidateSolver(module, templ, options.disj_arity));
  }
  else if (options.conj_arity == 1 && !options.impl_shape && options.strat_alt && !options.depth2_shape) {
    return shared_ptr<CandidateSolver>(new AltDisjunctCandidateSolver(module, templ, options.disj_arity));
  }
  else if (options.conj_arity == 1 && options.impl_shape && options.strat_alt && !options.depth2_shape) {
    return shared_ptr<CandidateSolver>(new AltImplCandidateSolver(module, templ, options.disj_arity));
  }
  else if (!options.impl_shape && !options.strat_alt && !options.depth2_shape) {
    assert (options.conj_arity > 1);
    return shared_ptr<CandidateSolver>(new ConjunctCandidateSolver(module, templ, options.conj_arity, options.disj_arity));
  }
  else if (!options.impl_shape && options.depth2_shape) {
    return shared_ptr<CandidateSolver>(new AltDepth2CandidateSolver(module, templ, options.disj_arity));
  }
  assert(false);
}

class ComposedCandidateSolver : public CandidateSolver {
public:
  vector<shared_ptr<CandidateSolver>> solvers;
  int idx;
  bool doing_chunks;

  ComposedCandidateSolver(vector<shared_ptr<CandidateSolver>> const& solvers)
    : solvers(solvers), idx(0), doing_chunks(false) { }

  value getNext() {
    while (idx < (int)solvers.size()) {
      value next = solvers[idx]->getNext();
      if (next != nullptr) {
        return next;
      } else {
        if (doing_chunks) {
          return nullptr;
        }
        idx++;
      }
    }
    return nullptr;
  }

  void addCounterexample(Counterexample cex, value candidate) {
    if (doing_chunks) {
      for (int i = 0; i < (int)solvers.size(); i++) {
        solvers[i]->addCounterexample(cex, candidate);
      }
    } else {
      for (int i = idx; i < (int)solvers.size(); i++) {
        solvers[i]->addCounterexample(cex, candidate);
      }
    }
  }

  void addExistingInvariant(value inv) {
    if (doing_chunks) {
      for (int i = 0; i < (int)solvers.size(); i++) {
        solvers[i]->addExistingInvariant(inv);
      }
    } else {
      for (int i = idx; i < (int)solvers.size(); i++) {
        solvers[i]->addExistingInvariant(inv);
      }
    }
    //assert (idx < (int)solvers.size());
    //solvers[idx]->addExistingInvariant(inv);
  }

  long long getProgress() {
    if (doing_chunks) {
      return -1;
    }
    long long prog = 0;
    for (int i = 0; i <= idx && i < (int)solvers.size(); i++) {
      prog += solvers[i]->getProgress();
    }
    return prog;
  }

  long long getSpaceSize() {
    long long res = 0;
    for (int i = 0; i < (int)solvers.size(); i++) {
      res += solvers[i]->getSpaceSize();
    }
    return res;
  }

  long long getPreSymmCount() {
    long long res = 0;
    for (int i = 0; i < (int)solvers.size(); i++) {
      res += solvers[i]->getPreSymmCount();
    }
    return res;
  }


  void setSpaceChunk(SpaceChunk const& sc) {
    doing_chunks = true;
    assert(0 <= sc.major_idx && sc.major_idx < (int)solvers.size());
    solvers[sc.major_idx]->setSpaceChunk(sc);
    idx = sc.major_idx;
  }

  void getSpaceChunk(std::vector<SpaceChunk>& res) {
    for (int i = 0; i < (int)solvers.size(); i++) {
      int cur = res.size();
      solvers[i]->getSpaceChunk(res);
      for (int j = cur; j < (int)res.size(); j++) {
        res[j].major_idx = i;
      }
    }
  }
};

std::shared_ptr<CandidateSolver> compose_candidate_solvers(
  std::vector<std::shared_ptr<CandidateSolver>> const& solvers)
{
  return shared_ptr<CandidateSolver>(new ComposedCandidateSolver(solvers));
}

std::shared_ptr<CandidateSolver> make_candidate_solver(
    std::shared_ptr<Module> module, bool enum_sat, 
    vector<EnumOptions> const& options,
    bool ensure_nonredundant)
{
  vector<shared_ptr<CandidateSolver>> solvers;
  for (EnumOptions const& enum_options : options) {
    cout << endl;
    cout << "--- Initializing enumerator ---" << endl;
    solvers.push_back(make_candidate_solver(
        module, enum_sat, enum_options, ensure_nonredundant));
  }
  cout << endl;
  return compose_candidate_solvers(solvers);
}
