#include "quantifier_permutations.h"

#include <set>
#include <algorithm>
#include <cassert>

using namespace std;

struct QRange {
  string sort_name;
  int start, end;
};

vector<QRange> get_qranges(vector<VarDecl> quantifiers) {
  vector<QRange> res;
  int a = 0;
  set<string> seen_names;

  while (a < quantifiers.size()) {
    int b = a + 1;
    while (b < quantifiers.size() && sorts_eq(quantifiers[a].sort, quantifiers[b].sort)) {
      b++;
    }
    QRange qr;

    lsort s =quantifiers[a].sort;
    UninterpretedSort* usort = dynamic_cast<UninterpretedSort*>(s.get());
    assert(usort != NULL);
    qr.sort_name = usort->name;
    qr.start = a;
    qr.end = b;
    res.push_back(qr);

    assert(seen_names.find(qr.sort_name) == seen_names.end());
    seen_names.insert(qr.sort_name);

    a = b;
  }

  return res;
}

void get_quantifier_permutations_(
    vector<VarDecl> const& quantifiers,
    vector<unsigned int> const& ovs,
    vector<QRange> const& qranges,
    int idx,
    vector<unsigned int> const& partial,
    vector<vector<unsigned int>>& res)
{
  if (idx == qranges.size()) {
    res.push_back(partial);
    return;
  }

  vector<unsigned int> my_ovs;
  for (int i = qranges[idx].start; i < qranges[idx].end; i++) {
    my_ovs.push_back(ovs[i]);
  }
  sort(my_ovs.begin(), my_ovs.end());

  do {
    vector<unsigned int> new_partial = partial;
    for (unsigned int j : my_ovs) {
      new_partial.push_back(j);
    }
    get_quantifier_permutations_(quantifiers, ovs, qranges, idx+1, new_partial, res);
  } while (next_permutation(my_ovs.begin(), my_ovs.end()));
}

vector<vector<unsigned int>> get_quantifier_permutations(
    vector<VarDecl> const& quantifiers,
    vector<unsigned int> const& ovs)
{
  vector<QRange> qranges = get_qranges(quantifiers);
  assert(quantifiers.size() == ovs.size());

  vector<vector<unsigned int>> res;
  get_quantifier_permutations_(quantifiers, ovs, qranges, 0, {}, res);
  return res;
}
