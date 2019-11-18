#include "synth_enumerator.h"

#include <cassert>

#include "enumerator.h"

using namespace std;

class NaiveCandidateSolver : public CandidateSolver {
public:
  NaiveCandidateSolver(std::shared_ptr<Module>, Options const&,
      bool ensure_nonredundant, Shape shape);

  value getNext();
  void addCounterexample(Counterexample cex, value candidate);
  void addExistingInvariant(value inv);

//private:
  std::shared_ptr<Module> module;
  Shape shape;
  Options options;
  bool ensure_nonredundant;

  TopQuantifierDesc tqd;

  std::vector<value> values;
  std::vector<value> unfiltered;

  std::vector<Counterexample> cexes;
  std::vector<int> cur_indices;

  std::vector<std::vector<std::pair<bool, bool>>> cached_evals;

  void increment();
  void dump_cur_indices();
  value fuse_as_impl(value a, value b);
};

std::shared_ptr<CandidateSolver> make_naive_candidate_solver(
    std::shared_ptr<Module> module, Options const& options,
      bool ensure_nonredundant, Shape shape)
{
  return shared_ptr<CandidateSolver>(new NaiveCandidateSolver(module, options, ensure_nonredundant, shape));
}

NaiveCandidateSolver::NaiveCandidateSolver(shared_ptr<Module> module, Options const& options, bool ensure_nonredundant, Shape shape)
  : module(module)
  , shape(shape)
  , options(options)
  , ensure_nonredundant(ensure_nonredundant)
  , tqd(module->templates[0])
{
  if (!options.impl_shape) {
    assert (options.conj_arity >= 1);
  }
  assert (options.disj_arity >= 1);
  assert (module->templates.size() == 1);

  auto p = enumerate_for_template(module, module->templates[0], options.disj_arity);
  values = p.first;
  unfiltered = p.second;
  for (int i = 0; i < values.size(); i++) {
    values[i] = values[i]->simplify();
  }
  for (int i = 0; i < unfiltered.size(); i++) {
    unfiltered[i] = unfiltered[i]->simplify();
  }

  cout << "Using " << values.size() << " values that are a disjunction of at most " << options.disj_arity << " terms." << endl;
  if (options.impl_shape) {
    cout << "Using " << unfiltered.size() << " for the second term." << endl;
  }

  //for (value v : values) {
  //  cout << v->to_string() << endl;
  //}
  //assert(false);

  if (options.impl_shape) {
    cur_indices = {0, 0};
  } else {
    cur_indices = {};
  }
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

value NaiveCandidateSolver::fuse_as_impl(value a, value b) {
  while (true) {
    if (Forall* f = dynamic_cast<Forall*>(a.get())) {
      a = f->body;
    }
    else if (Exists* f = dynamic_cast<Exists*>(a.get())) {
      a = f->body;
    }
    else {
      break;
    }
  }
  while (true) {
    if (Forall* f = dynamic_cast<Forall*>(b.get())) {
      b = f->body;
    }
    else if (Exists* f = dynamic_cast<Exists*>(b.get())) {
      b = f->body;
    }
    else {
      break;
    }
  }

  // XXX TODO FIXME hack
  vector<VarDecl> decls;
  decls.push_back(VarDecl(string_to_iden("A"), s_uninterp("node")));
  return tqd.with_body(v_exists(decls, v_implies(a, b)));
}

value NaiveCandidateSolver::getNext()
{
  if (options.impl_shape) {
    while (true) {
      if (cur_indices.size() < 2) {
        increment();
        continue;
      } else if (cur_indices.size() > 2) {
        return nullptr;
      }

      value v = fuse_as_impl(values[cur_indices[0]], unfiltered[cur_indices[1]]);
      //cout << "trying " << v->to_string() << endl;

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
  } else {
    while (true) {
      if (cur_indices.size() > options.conj_arity) {
        return nullptr;
      }

      bool failed = false;
      for (int i = 0; i < cexes.size(); i++) {
        bool first_bool = true;
        bool second_bool = true;
        for (int k = 0; k < cur_indices.size(); k++) {
          int j = cur_indices[k];
          first_bool = first_bool && cached_evals[i][j].first;
          second_bool = second_bool && cached_evals[i][j].second;
        }

        if (first_bool && !second_bool) {
          failed = true;
          break;
        }
      }

      increment();

      if (!failed) {
        dump_cur_indices();

        vector<value> conjuncts;
        for (int i = 0; i < cur_indices.size(); i++) {
          conjuncts.push_back(values[cur_indices[i]]);
        }
        value v = v_and(conjuncts);

        //cout << v->to_string() << endl;
        return v;
      }
    }
  }
  assert(false);
}

void NaiveCandidateSolver::dump_cur_indices()
{
  cout << "cur_indices:";
  for (int i : cur_indices) {
    cout << " " << i;
  }
  cout << " / " << values.size() << endl;
}

int t = 0;

void NaiveCandidateSolver::increment()
{
  t++;

  if (options.impl_shape) {
    assert(cur_indices.size() == 2);
    cur_indices[1]++;
    if (cur_indices[1] == unfiltered.size()) {
      cur_indices[0]++;    
      cur_indices[1] = 0;
      if (cur_indices[0] == values.size()) {
        cur_indices = {0,0,0};
      }
    }
  } else {
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
  if (t % 5000000 == 0) {
    dump_cur_indices();
  }
}

void NaiveCandidateSolver::addCounterexample(Counterexample cex, value candidate)
{
  assert (!cex.none);
  assert (cex.is_true || cex.is_false || (cex.hypothesis && cex.conclusion));
  cexes.push_back(cex);

  if (!options.impl_shape) {
    int i = cexes.size() - 1;

    cached_evals.push_back({});
    cached_evals[i].resize(values.size());

    for (int j = 0; j < values.size(); j++) {
      if (cex.is_true) {
        cached_evals[i][j].first = true;
        // TODO if it's false, we never have to look at values[j] again.
        cached_evals[i][j].second = cex.is_true->eval_predicate(values[j]);
      }
      else if (cex.is_false) {
        cached_evals[i][j].first = cex.is_false->eval_predicate(values[j]);
        cached_evals[i][j].second = false;
      }
      else {
        cached_evals[i][j].first = cex.hypothesis->eval_predicate(values[j]);
        cached_evals[i][j].second = cex.conclusion->eval_predicate(values[j]);
      }
    }
  }
}

void NaiveCandidateSolver::addExistingInvariant(value inv)
{
}
