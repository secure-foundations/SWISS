#include "strengthen_invariant.h"

#include "top_quantifier_desc.h"
#include "contexts.h"

using namespace std;

vector<value> remove(vector<value> const& ar, int j) {
  vector<value> v;
  for (int i = 0; i < ar.size(); i++) {
    if (i != j) {
      v.push_back(ar[i]);
    }
  }
  return v;
}

value strengthen_invariant(
  shared_ptr<Module> module,
  value invariant_so_far,
  value new_invariant)
{
  TopQuantifierDesc tqd(new_invariant);

  value body = new_invariant;
  while (true) {
    if (Forall* f = dynamic_cast<Forall*>(body.get())) {
      body = f->body;
    }
    else if (NearlyForall* f = dynamic_cast<NearlyForall*>(body.get())) {
      body = f->body;
    }
    else {
      break;
    }
  }

  Or* disj = dynamic_cast<Or*>(body.get());
  if (!disj) {
    return new_invariant;
  }
  vector<value> args = disj->args;

  for (int i = 0; i < args.size(); i++) {
    vector<value> new_args = remove(args, i);
    value inv = tqd.with_body(v_or(new_args));
    if (is_invariant_wrt(module, invariant_so_far, inv)) {
      args = new_args;
      i--;
    }
  }

  return tqd.with_body(v_or(args));
}
