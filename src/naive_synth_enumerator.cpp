#include "synth_enumerator.h"

#include <cassert>
#include <vector>
#include <map>

#include "enumerator.h"
#include "obviously_implies.h"

using namespace std;

struct PreComp {
  vector<value> values;
  vector<value> unfiltered;
  vector<vector<int>> implications;
  map<ComparableValue, int> normalized_to_idx;
};

map<pair<pair<shared_ptr<Module>, bool>, int>, PreComp> precomp_map;
PreComp make_precomp(
  shared_ptr<Module> module,
  bool ensure_nonredundant,
  int disj_arity)
{
  pair<pair<shared_ptr<Module>, bool>, int> tup = make_pair(make_pair(module, ensure_nonredundant), disj_arity);
  if (precomp_map.count(tup)) {
    return precomp_map[tup];
  }

  PreComp pc;

  auto p = enumerate_for_template(module, module->templates[0], disj_arity);
  pc.values = p.first;
  pc.unfiltered = p.second;
  for (int i = 0; i < pc.values.size(); i++) {
    pc.values[i] = pc.values[i]->simplify();
  }
  for (int i = 0; i < pc.unfiltered.size(); i++) {
    pc.unfiltered[i] = pc.unfiltered[i]->simplify();
  }

  //for (value v : pc.values) {
  //  cout << v->to_string() << endl;
  //}
  //assert(false);

  if (ensure_nonredundant) {
    pc.implications.resize(pc.values.size());

    for (int i = 0; i < pc.values.size(); i++) {
      ComparableValue cv(pc.values[i]->totally_normalize());
      pc.normalized_to_idx.insert(make_pair(cv, i));
    }

    for (int i = 0; i < pc.values.size(); i++) {
      //cout << "doing " << i << "   " << pc.values[i]->to_string() << endl;
      vector<value> subs = all_sub_disjunctions(pc.values[i]);
      bool found_self = false;
      for (value v : subs) {
        ComparableValue cv(v->totally_normalize());
        auto iter = pc.normalized_to_idx.find(cv);
        assert(iter != pc.normalized_to_idx.end());
        int idx = iter->second;
        assert(idx <= i);
        pc.implications[idx].push_back(i);

        //cout << "  " << idx << "   " << v->to_string() << "   " << v->totally_normalize()->to_string() << "   " << endl;

        if (idx == i) {
          found_self = true;
        }
      }
      //cout << endl;
      assert(found_self);
    }
  }

  precomp_map.insert(make_pair(tup, pc));
  return pc;
}

class NaiveCandidateSolver : public CandidateSolver {
public:
  NaiveCandidateSolver(shared_ptr<Module>, Options const&,
      bool ensure_nonredundant, Shape shape);

  value getNext();
  void addCounterexample(Counterexample cex, value candidate);
  void addExistingInvariant(value inv);

//private:
  shared_ptr<Module> module;
  Shape shape;
  Options options;
  bool ensure_nonredundant;

  TopQuantifierDesc tqd;

  vector<bool> values_usable;
  vector<value> values;
  vector<value> unfiltered;

  vector<Counterexample> cexes;
  vector<int> cur_indices;

  vector<vector<pair<bool, bool>>> cached_evals;

  vector<vector<int>> implications;
  map<ComparableValue, int> normalized_to_idx;

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
  if (ensure_nonredundant) {
    assert (!options.impl_shape);
    assert (options.conj_arity == 1);
  }
  if (!options.impl_shape) {
    assert (options.conj_arity >= 1);
  }
  assert (options.disj_arity >= 1);
  assert (module->templates.size() == 1);

  PreComp pc = make_precomp(module, ensure_nonredundant, options.disj_arity);
  values = move(pc.values);
  unfiltered = move(pc.unfiltered);
  implications = move(pc.implications);
  normalized_to_idx = move(pc.normalized_to_idx);

  cout << "Using " << values.size() << " values that are a disjunction of at most " << options.disj_arity << " terms." << endl;
  if (options.impl_shape) {
    cout << "Using " << unfiltered.size() << " for the second term." << endl;
  }

  values_usable.resize(values.size());
  for (int i = 0; i < values.size(); i++) {
    values_usable[i] = true;
  }

  if (options.impl_shape) {
    cur_indices = {0, 0};
  } else {
    if (ensure_nonredundant) {
      cur_indices = {0};
    } else {
      cur_indices = {};
    }
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

      if (!failed) {
        dump_cur_indices();
        increment();
        return v;
      } else {
        increment();
      }
    }
  } else {
    while (true) {
      if (cur_indices.size() > options.conj_arity) {
        return nullptr;
      }

      bool failed = false;

      for (int i = 0; i < cur_indices.size(); i++) {
        if (!values_usable[cur_indices[i]]) {
          failed = true;
        }
      }

      if (!failed) {
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
      }

      if (!failed) {
        dump_cur_indices();

        vector<value> conjuncts;
        for (int i = 0; i < cur_indices.size(); i++) {
          conjuncts.push_back(values[cur_indices[i]]);
        }
        value v = v_and(conjuncts);

        increment();

        //cout << v->to_string() << endl;
        return v;
      } else {
        increment();
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

  if (!options.impl_shape) {
    if (options.conj_arity == 1) {
      for (int j = 0; j < values.size(); j++) {
        if (values_usable[j]) {
          if (cex.is_true) {
            if (!cex.is_true->eval_predicate(values[j])) {
              values_usable[j] = false;
            }
          }
          else if (cex.is_false) {
            if (cex.is_false->eval_predicate(values[j])) {
              values_usable[j] = false;
            }
          }
          else {
            if (cex.hypothesis->eval_predicate(values[j]) &&
                !cex.conclusion->eval_predicate(values[j])) {
              values_usable[j] = false;
            }
          }
        }
      }
    } else {
      cexes.push_back(cex);
      int i = cexes.size() - 1;

      cached_evals.push_back({});
      cached_evals[i].resize(values.size());

      for (int j = 0; j < values.size(); j++) {
        if (values_usable[j]) {
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
  }
}

void NaiveCandidateSolver::addExistingInvariant(value inv)
{
  //cout << "inv is " << inv->to_string() << endl;
  //cout << "norm is " << inv->totally_normalize()->to_string() << endl;

  ComparableValue cv(inv->totally_normalize());
  auto iter = normalized_to_idx.find(cv);
  assert(iter != normalized_to_idx.end());
  int idx = iter->second;
  assert(0 <= idx && idx < implications.size());

  for (int idx2 : implications[idx]) {
    //cout << "setting " << idx2 << endl;
    values_usable[idx2] = false;
  }
}
