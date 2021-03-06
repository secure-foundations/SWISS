#include "synth_enumerator.h"

#include <cassert>
#include <vector>
#include <map>

#include "enumerator.h"
#include "alt_synth_enumerator.h"
#include "alt_depth2_synth_enumerator.h"

using namespace std;

class OverlordCandidateSolver : public CandidateSolver {
public:
  vector<TemplateSubSlice> sub_slices;
  vector<TemplateSpace> spaces;
  vector<shared_ptr<CandidateSolver>> solvers;

  vector<int> cex_idx;
  vector<int> inv_idx;
  vector<Counterexample> cexes;
  vector<value> invs;

  int idx;
  int solver_idx;
  bool done;

  OverlordCandidateSolver(
      shared_ptr<Module> module,
      vector<TemplateSubSlice> const& sub_slices)
  {
    this->sub_slices = sub_slices;

    cout << "OverlordCandidateSolver" << endl;
    //for (TemplateSubSlice const& tss : sub_slices) {
    //  cout << tss << endl;
    //}

    spaces = finer_spaces_containing_sub_slices(module, sub_slices);
    for (int i = 0; i < (int)spaces.size(); i++) {
      cout << endl;
      cout << "--- Initializing enumerator ---" << endl;
      cout << spaces[i] << endl;

      solvers.push_back(shared_ptr<CandidateSolver>(
          spaces[i].depth == 2
              ? (CandidateSolver*)new AltDepth2CandidateSolver(module, spaces[i])
              : (CandidateSolver*)new AltDisjunctCandidateSolver(module, spaces[i])
          ));
      cex_idx.push_back(0);
      inv_idx.push_back(0);
    }
    idx = 0;
    done = false;
    set_solver_idx();
  }

  void update_cexes_invs() {
    while (inv_idx[solver_idx] < (int)invs.size()) {
      solvers[solver_idx]->addExistingInvariant(invs[inv_idx[solver_idx]]);
      inv_idx[solver_idx]++;
    }

    while (cex_idx[solver_idx] < (int)cexes.size()) {
      solvers[solver_idx]->addCounterexample(cexes[cex_idx[solver_idx]]);
      cex_idx[solver_idx]++;
    }
  }

  void set_solver_idx() {
    if (idx < (int)sub_slices.size()) {
      cout << endl << "OverlordCandidateSolver: " << sub_slices[idx] << endl
           << "OverlordCandidateSolver: index " << idx << " / "
           << sub_slices.size() << endl << endl;
      for (int i = 0; i < (int)spaces.size(); i++) {
        if (is_subspace(sub_slices[idx], spaces[i])) {
          //cout << "setting to " << i << endl;
          solver_idx = i;
          solvers[i]->setSubSlice(sub_slices[idx]);
          //cout << "done set " << endl;

          update_cexes_invs();

          return;
        }
      }
      assert (false);
    } else {
      cout << endl << "OverlordCandidateSolver: done" << endl << endl;
      done = true;
    }
  }

  value getNext() {
    //cout << "getNext()" << endl;
    while (!done) {
      //cout << "calling getNext()" << endl;
      value next = solvers[solver_idx]->getNext();
      //cout << "done getNext()" << endl;
      if (next != nullptr) {
        //cout << "returning" << endl;
        return next;
      } else {
        idx++;
        set_solver_idx();
      }
    }
    return nullptr;
  }

  void addCounterexample(Counterexample cex) {
    cexes.push_back(cex);
    solvers[solver_idx]->addCounterexample(cex);
    cex_idx[solver_idx]++;
  }

  void addExistingInvariant(value inv) {
    invs.push_back(inv);
    solvers[solver_idx]->addExistingInvariant(inv);
    inv_idx[solver_idx]++;
  }

  long long getProgress() {
    return -1;
  }

  long long getSpaceSize() {
    assert(false);
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

std::shared_ptr<CandidateSolver> make_candidate_solver(
    std::shared_ptr<Module> module,
    vector<TemplateSubSlice> const& sub_slices,
    bool ensure_nonredundant)
{
  return shared_ptr<CandidateSolver>(
      new OverlordCandidateSolver(module, sub_slices));
}
