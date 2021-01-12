#include "synth_enumerator.h"

#include <cassert>
#include <vector>
#include <map>

#include "enumerator.h"
#include "alt_synth_enumerator.h"
#include "alt_depth2_synth_enumerator.h"
#include "flooding.h"

using namespace std;

class OverlordCandidateSolver : public CandidateSolver {
public:
  vector<TemplateSubSlice> sub_slices;
  vector<TemplateSpace> spaces;
  vector<shared_ptr<CandidateSolver>> solvers;

  vector<int> cex_idx;
  vector<int> rd_idx;
  vector<Counterexample> cexes;
  vector<RedundantDesc> redundantDescs;

  unique_ptr<Flood> flood;

  int idx;
  int solver_idx;
  bool done;

  OverlordCandidateSolver(
      shared_ptr<Module> module,
      vector<TemplateSubSlice> const& sub_slices,
      std::vector<value> const& extra_starting_formulas_flood)
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
      rd_idx.push_back(0);
    }
    idx = 0;
    done = false;
    set_solver_idx();

    init_quant_masks();
    initialize_flood(module, calc_max_e(sub_slices), extra_starting_formulas_flood);
  }

  static int calc_max_e(vector<TemplateSubSlice> const& sub_slices)
  {
    int emax = 0;
    for (TemplateSubSlice const& tss : sub_slices) {
      int e = 0;
      for (int i = 0; i < (int)tss.ts.vars.size(); i++) {
        if (tss.ts.quantifiers[i] == Quantifier::Exists) {
          e += tss.ts.vars[i];
        }
      }
      if (e > emax) emax = e;
    }
    return emax;
  }

  vector<vector<int>> subidx_maps;

  void initialize_flood(shared_ptr<Module> module, int max_e,
      std::vector<value> const& extra_starting_formulas_flood)
  {
    TemplateSpace ts = get_full_tspace_for_flood();
    this->flood = unique_ptr<Flood>(new Flood(module, ts, max_e));
    init_subidx_maps(module, ts);
    append_redundant_descs(
        this->flood->get_initial_redundant_descs(extra_starting_formulas_flood));
    update_cexes_invs();
  }

  void append_redundant_descs(vector<RedundantDesc> const& rds)
  {
    for (RedundantDesc const& rd : rds) {
      redundantDescs.push_back(rd);
    }
  }

  void update_cexes_invs() {
    vector<int> new_v;
    while (rd_idx[solver_idx] < (int)redundantDescs.size()) {
      if (rd_for_solver(redundantDescs[rd_idx[solver_idx]], solver_idx)) {
        vector<int> const& old_v = redundantDescs[rd_idx[solver_idx]].v;
        new_v.resize(old_v.size());
        for (int j = 0; j < (int)old_v.size(); j++) {
          new_v[j] = subidx_maps[solver_idx][old_v[j]];
          if (new_v[j] == -1) {
            goto skip;
          }
        }
        solvers[solver_idx]->addRedundantDesc(new_v);
        skip: {}
      }
      rd_idx[solver_idx]++;
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
    //invs.push_back(inv);
    //solvers[solver_idx]->addExistingInvariant(inv);
    //inv_idx[solver_idx]++;
    append_redundant_descs(flood->add_formula(inv));
    update_cexes_invs();
  }

  vector<uint32_t> quant_masks;
  void init_quant_masks() {
    quant_masks.resize(spaces.size());
    for (int i = 0; i < (int)spaces.size(); i++) {
      uint32_t m = 0;
      for (int j = 0; j < (int)spaces[i].quantifiers.size(); j++) {
        if (spaces[i].quantifiers[j] == Quantifier::Exists) {
          m |= (1 << j);
        }
      }
      quant_masks[i] = m;
    }
  }
  bool rd_for_solver(RedundantDesc const& rd, int solver_idx) {
    return rd.quant_mask == quant_masks[solver_idx];
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

  void addRedundantDesc(std::vector<int> const&) {
    assert(false);
  }

  TemplateSpace get_full_tspace_for_flood() {
    assert (spaces.size() > 0);
    TemplateSpace ts;
    ts.vars.resize(spaces[0].vars.size());
    ts.quantifiers.resize(spaces[0].vars.size());
    for (int i = 0; i < (int)ts.quantifiers.size(); i++) {
      ts.vars[i] = 0;
      ts.quantifiers[i] = Quantifier::Forall;
    }
    ts.depth = 1;
    ts.k = 1;
    for (int i = 0; i < (int)spaces.size(); i++) {
      ts.depth = max(ts.depth, spaces[i].depth);
      ts.k = max(ts.k, spaces[i].k);
      for (int j = 0; j < (int)ts.vars.size(); j++) {
        ts.vars[j] = max(ts.vars[j], spaces[i].vars[j]);
      }
    }

    assert (ts.depth == 1);
    return ts;
  }

  void init_subidx_maps(shared_ptr<Module> module, TemplateSpace& ts) {
    value templ = ts.make_templ(module);
    TopAlternatingQuantifierDesc big_taqd(templ);

    subidx_maps.resize(spaces.size());
    vector<value> clauses = this->flood->get_clauses();
    for (int i = 0; i < (int)spaces.size(); i++) {
      value small_templ = get_sub_templ(module, big_taqd, spaces[i]);
      vector<value> sub_clauses = get_clauses_for_template(module, small_templ);

      subidx_maps[i] = get_subidx_map(clauses, sub_clauses);
    }
  }

  // returns 'forall' stuff regardless of inputs
  value get_sub_templ(
      shared_ptr<Module> module,
      TopAlternatingQuantifierDesc const& big_taqd,
      TemplateSpace const& sub_ts)
  {
    vector<int> var_counts = sub_ts.vars;

    vector<VarDecl> decls;
    for (Alternation const& alt : big_taqd.alternations()) {
      for (VarDecl const& decl : alt.decls) {
        int sort_idx = sort_idx_of_module(module, decl.sort);
        if (var_counts[sort_idx] > 0) {
          var_counts[sort_idx]--;
          decls.push_back(decl);
        }
      }
    }
    return v_forall(decls, v_template_hole());
  }

  // return map from idx in `clauses` to corresponding value in `sub_clauses`
  // or -1 if it doesn't exist in `sub_clauses`
  static vector<int> get_subidx_map(
      vector<value> const& clauses,
      vector<value> const& sub_clauses)
  {
    vector<int> res(clauses.size());
    map<ComparableValue, int> m;
    for (int i = 0; i < (int)clauses.size(); i++) {
      m.insert(make_pair(
        ComparableValue(TopAlternatingQuantifierDesc::get_body(clauses[i])),
        i));
      res[i] = -1;
    }
    for (int i = 0; i < (int)sub_clauses.size(); i++) {
      value v = TopAlternatingQuantifierDesc::get_body(sub_clauses[i]);
      auto it = m.find(ComparableValue(v));
      assert (it != m.end());
      assert (res[it->second] == -1);
      res[it->second] = i;
    }

    // sanity check
    int last = -1;
    for (int i = 0; i < (int)res.size(); i++) {
      if (res[i] != -1) {
        assert (res[i] == last + 1);
        last++;
      }
    }

    return res;
  }
};

std::shared_ptr<CandidateSolver> make_candidate_solver(
    std::shared_ptr<Module> module,
    vector<TemplateSubSlice> const& sub_slices,
    std::vector<value> const& extra_starting_formulas_flood)
{
  return shared_ptr<CandidateSolver>(
      new OverlordCandidateSolver(module, sub_slices, extra_starting_formulas_flood));
}
