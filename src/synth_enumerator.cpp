#include "synth_enumerator.h"

#include <cassert>
#include <vector>
#include <map>

#include "enumerator.h"
#include "alt_synth_enumerator.h"
#include "alt_depth2_synth_enumerator.h"

using namespace std;

/*std::shared_ptr<CandidateSolver> make_naive_candidate_solver(
    std::shared_ptr<Module> module, EnumOptions const& options)
{
  assert (0 <= options.template_idx && options.template_idx < (int)module->templates.size());
  value templ = module->templates[options.template_idx];
  cout << "selecting template " << templ->to_string() << endl;

  if (options.depth2_shape) {
    return shared_ptr<CandidateSolver>(new AltDepth2CandidateSolver(module, templ, options.disj_arity));
  } else {
    return shared_ptr<CandidateSolver>(new AltDisjunctCandidateSolver(module, templ, options.disj_arity));
  }
}*/

/*class ComposedCandidateSolver : public CandidateSolver {
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
};*/

class OverlordCandidateSolver : public CandidateSolver {
public:
  vector<TemplateSubSlice> sub_slices;
  vector<TemplateSpace> spaces;
  vector<shared_ptr<CandidateSolver>> solvers;
  int idx;
  int solver_idx;
  bool done;

  OverlordCandidateSolver(
      shared_ptr<Module> module,
      vector<TemplateSubSlice> const& sub_slices)
  {
    this->sub_slices = sub_slices;

    spaces = spaces_containing_sub_slices(sub_slices);
    for (int i = 0; i < (int)spaces.size(); i++) {
      cout << endl;
      cout << "--- Initializing enumerator ---" << endl;

      solvers.push_back(shared_ptr<CandidateSolver>(
          spaces[i].depth == 2
              ? (CandidateSolver*)new AltDepth2CandidateSolver(module, spaces[i])
              : (CandidateSolver*)new AltDisjunctCandidateSolver(module, spaces[i])
          ));
    }
    idx = 0;
    done = false;
    set_solver_idx();
  }

  void set_solver_idx() {
    assert (false && "TODO add logging here");

    if (idx < (int)sub_slices.size()) {
      for (int i = 0; i < (int)spaces.size(); i++) {
        if (is_subspace(sub_slices[idx], spaces[i])) {
          solver_idx = i;
          solvers[i]->setSubSlice(sub_slices[idx]);
          return;
        }
      }
      assert (false);
    } else {
      done = true;
    }
  }

  value getNext() {
    while (!done) {
      value next = solvers[idx]->getNext();
      if (next != nullptr) {
        return next;
      } else {
        idx++;
        set_solver_idx();
      }
    }
    return nullptr;
  }

  void addCounterexample(Counterexample cex, value candidate) {
    for (int i = 0; i < (int)solvers.size(); i++) {
      solvers[i]->addCounterexample(cex, candidate);
    }
  }

  void addExistingInvariant(value inv) {
    for (int i = 0; i < (int)solvers.size(); i++) {
      solvers[i]->addExistingInvariant(inv);
    }
  }

  long long getProgress() {
    return -1;
  }

  long long getPreSymmCount() {
    long long res = 0;
    for (int i = 0; i < (int)solvers.size(); i++) {
      res += solvers[i]->getPreSymmCount();
    }
    return res;
  }

  void setSubSlice(TemplateSubSlice const& tss) {
    assert(false);
  }
};

/*std::shared_ptr<CandidateSolver> compose_candidate_solvers(
  std::vector<std::shared_ptr<CandidateSolver>> const& solvers)
{
  return shared_ptr<CandidateSolver>(new ComposedCandidateSolver(solvers));
}*/

std::shared_ptr<CandidateSolver> make_candidate_solver(
    std::shared_ptr<Module> module,
    vector<TemplateSubSlice> const& sub_slices,
    bool ensure_nonredundant)
{
  return shared_ptr<CandidateSolver>(
      new OverlordCandidateSolver(module, sub_slices));
}
