#include "obviously_implies.h"

#include <map>
#include <vector>
#include <cassert>

#include "top_quantifier_desc.h"

using namespace std;

vector<value> get_disjuncts(value v) {
  if (Or* o = dynamic_cast<Or*>(v.get())) {
    return o->args;
  } else {
    return {v};
  }
}

vector<value> all_sub_disjunctions(value v)
{
  auto p = get_tqd_and_body(v);
  TopQuantifierDesc tqd = p.first;
  value body = p.second;

  vector<value> disjuncts = get_disjuncts(body);

  int n = disjuncts.size();
  assert(n < 30);

  vector<value> res;

  for (int i = 1; i < (1 << n); i++) {
    vector<value> v;
    for (int j = 0; j < n; j++) {
      if (i & (1 << j)) {
        v.push_back(disjuncts[j]);
      }
    }
    res.push_back(tqd.with_body(v_or(v)));
  }

  return res;
}

/*bool disj_subset(
    vector<value> const& ad,
    vector<value> const& bd,
    int ad_idx,
    map<string, string> const& emptyVarMapping)
{
  if (ad_idx == ad.size()) {
    return true;
  }
}

bool obviously_implies(shared_ptr<Module> module, value a, value b)
{
  Forall* af = dynamic_cast<Forall*>(a.get())
  Forall* bf = dynamic_cast<Forall*>(b.get())
  if (af != NULL) {
    assert (bf != NULL);
    assert (af.decls == bf.decls);
    return obviously_implies(module, a->body, b->body);
  }

  assert (bf == NULL);

  vector<value> ad = get_disjuncts(a);
  vector<value> bd = get_disjuncts(b);

  if (ad.size() <= bd.size()) {
    map<string, string> emptyVarMapping;
    return disj_subset(ad, bd, 0, emptyVarMapping);
  } else {
    return false;
  }
}*/
