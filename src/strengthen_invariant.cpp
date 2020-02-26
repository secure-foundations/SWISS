#include "strengthen_invariant.h"

#include "top_quantifier_desc.h"
#include "contexts.h"

using namespace std;

vector<value> remove(vector<value> const& ar, int j) {
  vector<value> v;
  for (int i = 0; i < (int)ar.size(); i++) {
    if (i != j) {
      v.push_back(ar[i]);
    }
  }
  return v;
}

value try_replacing_exists_with_forall(
  shared_ptr<Module> module,
  value invariant_so_far,
  value new_invariant)
{
  TopAlternatingQuantifierDesc taqd(new_invariant);
  value body = TopAlternatingQuantifierDesc::get_body(new_invariant);
  TopAlternatingQuantifierDesc taqd_mod = taqd.replace_exists_with_forall();

  value inv0 = taqd_mod.with_body(body);

  if (is_invariant_wrt(module, invariant_so_far, inv0)) {
    return inv0;
  }

  return new_invariant;
}

value strengthen_invariant_once(
  shared_ptr<Module> module,
  value invariant_so_far,
  value new_invariant)
{
  new_invariant = try_replacing_exists_with_forall(module, invariant_so_far, new_invariant);

  TopAlternatingQuantifierDesc taqd(new_invariant);
  value body = TopAlternatingQuantifierDesc::get_body(new_invariant);

  Or* disj = dynamic_cast<Or*>(body.get());
  if (!disj) {
    return new_invariant;
  }
  vector<value> args = disj->args;

  cout << endl;

  for (int i = 0; i < (int)args.size(); i++) {
    vector<value> new_args = remove(args, i);
    value inv = taqd.with_body(v_or(new_args));
    cout << "trying " << inv->to_string();
    if (is_invariant_wrt(module, invariant_so_far, inv)) {
      cout << "is inv" << endl << endl;
      args = new_args;
      i--;
    } else {
      cout << "is not inv" << endl << endl;
    }
  }

  return taqd.with_body(v_or(args));
}

value strengthen_invariant(
  shared_ptr<Module> module,
  value invariant_so_far,
  value new_invariant)
{
  cout << "strengthening " << new_invariant->to_string() << endl;
  value inv = new_invariant;

  int t = 0;
  while (true) {
    assert (t < 20);

    value inv0 = strengthen_invariant_once(module, invariant_so_far, inv);
    if (v_eq(inv, inv0)) {
      cout << "got " << inv0->to_string() << endl;
      assert(false);
      return inv0;
    }
    inv = inv0;
  }
}
