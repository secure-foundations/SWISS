#include "synth_enumerator.h"

#include <cassert>

#include "enumerator.h"

using namespace std;

NaiveCandidateSolver::NaiveCandidateSolver(shared_ptr<Module> module, Options const& options, bool ensure_nonredundant, Shape shape)
  : module(module)
  , shape(shape)
  , options(options)
  , ensure_nonredundant(ensure_nonredundant)
{
  assert (!ensure_nonredundant);
  assert (options.conj_arity >= 1);
  assert (options.disj_arity >= 1);
  assert (module->templates.size() == 1);

  for (int i = 1; i <= options.disj_arity; i++) {
    vector<value> level = enumerate_for_template(module, module->templates[0], i);
    for (value v : level) {
      values.push_back(v);
    }
  }
  for (int i = 0; i < values.size(); i++) {
    values[i] = values[i]->simplify();
  }

  cout << "Using " << values.size() << " values that are a disjunction of at most " << options.disj_arity << " terms." << endl;

  /*for (value v : level1) {
    cout << v->to_string() << endl;
  }*/

  cur_indices = {};
}

bool passes_cex(value v, Counterexample const& cex)
{
  if (cex.is_true) {
    return cex.is_true->eval_predicate(v);
  }
  else if (cex.is_false) {
    return !cex.is_false->eval_predicate(v);
  }
  else {
    return !cex.hypothesis->eval_predicate(v)
         || cex.conclusion->eval_predicate(v);
  }
}

value NaiveCandidateSolver::getNext()
{
  while (true) {
    if (cur_indices.size() > options.conj_arity) {
      return nullptr;
    }

    vector<value> conjuncts;
    for (int i = 0; i < cur_indices.size(); i++) {
      conjuncts.push_back(values[cur_indices[i]]);
    }
    value v = v_and(conjuncts);

    bool failed = false;
    for (int i = 0; i < cexes.size(); i++) {
      if (!passes_cex(v, cexes[i])) {
        failed = true;
        break;
      }
    }

    increment();

    if (!failed) {
      dump_cur_indices();
      return v;
    }
  }
}

void NaiveCandidateSolver::dump_cur_indices()
{
  cout << cur_indices.
}

void NaiveCandidateSolver::increment()
{
  int j;
  for (j = cur_indices.size() - 1; j >= 0; j--) {
    if (cur_indices[j] != values.size() - cur_indices.size() + j) {
      cur_indices[j]++;
      for (int k = j+1; k < cur_indices.size(); k++) {
        cur_indices[k] = cur_indices[k-1] + 1;
      }
      break;
    }
  }
  if (j == -1) {
    cur_indices.push_back(0);
    assert (cur_indices.size() <= values.size());
    for (int i = 0; i < cur_indices.size(); i++) {
      cur_indices[i] = i;
    }
  }
}

void NaiveCandidateSolver::addCounterexample(Counterexample cex, value candidate)
{
  assert (!cex.none);
  cexes.push_back(cex);
}

void NaiveCandidateSolver::addExistingInvariant(value inv)
{
  assert(false);
}
