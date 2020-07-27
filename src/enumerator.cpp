#include "enumerator.h"

#include <set>
#include <algorithm>
#include <cassert>

#include "logic.h"
#include "obviously_implies.h"
#include "clause_gen.h"

using namespace std;

struct HoleInfo {
  vector<VarDecl> decls;
};

void getHoleInfo_(value v, vector<VarDecl> decls, vector<HoleInfo>& res) {
  assert(v.get() != NULL);
  if (Forall* va = dynamic_cast<Forall*>(v.get())) {
    for (VarDecl decl : va->decls) {
      if (dynamic_cast<UninterpretedSort*>(decl.sort.get())) {
        decls.push_back(VarDecl(decl.name, decl.sort));
      }
    }
    getHoleInfo_(va->body, decls, res);
  }
  else if (Exists* va = dynamic_cast<Exists*>(v.get())) {
    for (VarDecl decl : va->decls) {
      if (dynamic_cast<UninterpretedSort*>(decl.sort.get())) {
        decls.push_back(VarDecl(decl.name, decl.sort));
      }
    }
    getHoleInfo_(va->body, decls, res);
  }
  else if (dynamic_cast<Var*>(v.get())) {
    return;
  }
  else if (dynamic_cast<Const*>(v.get())) {
    return;
  }
  else if (Eq* va = dynamic_cast<Eq*>(v.get())) {
    getHoleInfo_(va->left, decls, res);
    getHoleInfo_(va->right, decls, res);
  }
  else if (Not* va = dynamic_cast<Not*>(v.get())) {
    getHoleInfo_(va->val, decls, res);
  }
  else if (Implies* va = dynamic_cast<Implies*>(v.get())) {
    getHoleInfo_(va->left, decls, res);
    getHoleInfo_(va->right, decls, res);
  }
  else if (Apply* va = dynamic_cast<Apply*>(v.get())) {
    getHoleInfo_(va->func, decls, res);
    for (value arg : va->args) {
      getHoleInfo_(arg, decls, res);
    }
  }
  else if (And* va = dynamic_cast<And*>(v.get())) {
    for (value arg : va->args) {
      getHoleInfo_(arg, decls, res);
    }
  }
  else if (Or* va = dynamic_cast<Or*>(v.get())) {
    for (value arg : va->args) {
      getHoleInfo_(arg, decls, res);
    }
  }
  else if (dynamic_cast<TemplateHole*>(v.get())) {
    HoleInfo hi;
    hi.decls = decls;
    res.push_back(hi);
  }
  else {
    //printf("value2expr got: %s\n", v->to_string().c_str());
    assert(false && "value2expr does not support this case");
  }
}

vector<HoleInfo> getHoleInfo(value v) {
  vector<HoleInfo> res;
  getHoleInfo_(v, {}, res);
  return res;
}

value fill_holes_in_value(value templ, vector<value> const& fills, int& idx) {
  assert(templ.get() != NULL);
  if (Forall* va = dynamic_cast<Forall*>(templ.get())) {
    return v_forall(va->decls, fill_holes_in_value(va->body, fills, idx));
  }
  else if (Exists* va = dynamic_cast<Exists*>(templ.get())) {
    return v_exists(va->decls, fill_holes_in_value(va->body, fills, idx));
  }
  else if (NearlyForall* va = dynamic_cast<NearlyForall*>(templ.get())) {
    return v_nearlyforall(va->decls, fill_holes_in_value(va->body, fills, idx));
  }
  else if (dynamic_cast<Var*>(templ.get())) {
    return templ;
  }
  else if (dynamic_cast<Const*>(templ.get())) {
    return templ;
  }
  else if (Eq* va = dynamic_cast<Eq*>(templ.get())) {
    return v_eq(
        fill_holes_in_value(va->left, fills, idx),
        fill_holes_in_value(va->right, fills, idx));
  }
  else if (Not* va = dynamic_cast<Not*>(templ.get())) {
    return v_not(
        fill_holes_in_value(va->val, fills, idx));
  }
  else if (Implies* va = dynamic_cast<Implies*>(templ.get())) {
    return v_implies(
        fill_holes_in_value(va->left, fills, idx),
        fill_holes_in_value(va->right, fills, idx));
  }
  else if (Apply* va = dynamic_cast<Apply*>(templ.get())) {
    value func = fill_holes_in_value(va->func, fills, idx);
    vector<value> args;
    for (value arg : va->args) {
      args.push_back(fill_holes_in_value(arg, fills, idx));
    }
    return v_apply(func, args);
  }
  else if (And* va = dynamic_cast<And*>(templ.get())) {
    vector<value> args;
    for (value arg : va->args) {
      args.push_back(fill_holes_in_value(arg, fills, idx));
    }
    return v_and(args);
  }
  else if (Or* va = dynamic_cast<Or*>(templ.get())) {
    vector<value> args;
    for (value arg : va->args) {
      args.push_back(fill_holes_in_value(arg, fills, idx));
    }
    return v_or(args);
  }
  else if (dynamic_cast<TemplateHole*>(templ.get())) {
    return fills[idx++];
  }
  else {
    //printf("value2expr got: %s\n", templ->to_string().c_str());
    assert(false && "fill_holes_in_value does not support this case");
  }
}

value fill_holes_in_value(value templ, vector<value> const& fills) {
  int idx = 0;
  value res = fill_holes_in_value(templ, fills, idx);
  assert(idx == (int)fills.size());
  return res;
}

vector<value> fill_holes(value templ, vector<vector<value>> const& fills) {
  vector<value> result;

  vector<int> indices;
  for (int i = 0; i < (int)fills.size(); i++) {
    if (fills[i].size() == 0) {
      return {};
    }
    indices.push_back(0);
  }
  while (true) {
    vector<value> values_to_fill;
    for (int i = 0; i < (int)fills.size(); i++) {
      assert(0 <= indices[i] && indices[i] < (int)fills[i].size());
      values_to_fill.push_back(fills[i][indices[i]]);
    }

    int idx = 0;
    result.push_back(fill_holes_in_value(templ, values_to_fill, idx));
    assert(idx == (int)fills.size());
    
    int i;
    for (i = 0; i < (int)fills.size(); i++) {
      indices[i]++;
      if (indices[i] == (int)fills[i].size()) {
        indices[i] = 0;
      } else {
        break;
      }
    }
    if (i == (int)fills.size()) {
      break;
    }
  }

  return result;
}

bool are_negations1(value a, value b) {
  if (Not* n = dynamic_cast<Not*>(a.get())) {
    return values_equal(n->val, b);
  } else {
    return false;
  }
}

bool are_negations(value a, value b) {
  return are_negations1(a,b) || are_negations1(b,a);
}

void enum_conjuncts_(
    vector<value> const& pieces, int k,
    int idx, vector<value> acc, vector<value>& result)
{
  if ((int)acc.size() == k) {
    result.push_back(v_and(acc));
    return;
  }

  for (int i = idx + 1; i < (int)pieces.size(); i++) {
    if (i == idx+1) acc.push_back(pieces[i]);
    else acc[acc.size() - 1] = pieces[i];

    bool skip = false;
    for (int j = 0; j < (int)acc.size() - 1; j++) {
      if (are_negations(acc[j], pieces[i])) {
        skip = true;
        break;
      }
    }

    if (!skip) {
      enum_conjuncts_(pieces, k, i, acc, result);
    }
  }
}

vector<value> enum_conjuncts(vector<value> const& pieces, int k) {
  vector<value> result;
  for (int i = 1; i <= k; i++) {
    enum_conjuncts_(pieces, i, -1, {}, result);
  }
  return result;
}

bool is_boring(value v, bool pos) {
  if (Not* va = dynamic_cast<Not*>(v.get())) {
    return is_boring(va->val, !pos);
  }
  else if (Eq* va = dynamic_cast<Eq*>(v.get())) {
    // FIXME to_string comparison is sketch
    return va->left->to_string() == va->right->to_string() ||
      (pos && (
        dynamic_cast<Var*>(va->left.get()) != NULL ||
        dynamic_cast<Var*>(va->right.get()) != NULL
      ));
    /*
  } else if (And* va = dynamic_cast<And*>(v.get())) {
    for (value arg : va->args) {
      if (is_boring(arg)) return true;
    }
    return false;
  } else if (Or* va = dynamic_cast<Or*>(v.get())) {
    for (value arg : va->args) {
      if (is_boring(arg)) return true;
    }
    return false;
    */
  } else if (Apply* ap = dynamic_cast<Apply*>(v.get())) {
    return (iden_to_string(dynamic_cast<Const*>(ap->func.get())->name) == "le" &&
      ap->args[0]->to_string() == ap->args[1]->to_string());
  } else {
    return false;
  }
}

vector<value> filter_boring(vector<value> const& values) {
  vector<value> results;
  for (value v : values) {
    if (!is_boring(v, true)) {
      results.push_back(v);
    }
  }
  return results;
}

struct NormalizeState {
  vector<iden> names;
  iden get_name(iden name) {
    int idx = -1;
    for (int i = 0; i < (int)names.size(); i++) {
      if (names[i] == name) {
        idx = i;
        break;
      }
    }
    assert (idx != -1);
    return idx;
  }
};

value normalize(value v, NormalizeState& ns) {
  assert(v.get() != NULL);
  if (Forall* va = dynamic_cast<Forall*>(v.get())) {
    return v_forall(va->decls, normalize(va->body, ns));
  }
  else if (Exists* va = dynamic_cast<Exists*>(v.get())) {
    return v_exists(va->decls, normalize(va->body, ns));
  }
  else if (Var* va = dynamic_cast<Var*>(v.get())) {
    return v_var(ns.get_name(va->name), va->sort);
  }
  else if (dynamic_cast<Const*>(v.get())) {
    return v;
  }
  else if (Eq* va = dynamic_cast<Eq*>(v.get())) {
    return v_eq(
        normalize(va->left, ns),
        normalize(va->right, ns));
  }
  else if (Not* va = dynamic_cast<Not*>(v.get())) {
    return v_not(
        normalize(va->val, ns));
  }
  else if (Implies* va = dynamic_cast<Implies*>(v.get())) {
    return v_implies(
        normalize(va->left, ns),
        normalize(va->right, ns));
  }
  else if (Apply* va = dynamic_cast<Apply*>(v.get())) {
    value func = normalize(va->func, ns);
    vector<value> args;
    for (value arg : va->args) {
      args.push_back(normalize(arg, ns));
    }
    return v_apply(func, args);
  }
  else if (And* va = dynamic_cast<And*>(v.get())) {
    vector<value> args;
    for (value arg : va->args) {
      args.push_back(normalize(arg, ns));
    }
    return v_and(args);
  }
  else if (Or* va = dynamic_cast<Or*>(v.get())) {
    vector<value> args;
    for (value arg : va->args) {
      args.push_back(normalize(arg, ns));
    }
    return v_or(args);
  }
  else {
    //printf("value2expr got: %s\n", templ->to_string().c_str());
    assert(false && "value2expr does not support this case");
  }
}

/*vector<value> remove_equiv(value templ, vector<value> const& values) {
  NormalizeState ns_base;

  vector<value> result;
  set<string> seen;
  for (value v : values) {
    NormalizeState ns = ns_base;
    value norm = normalize(v, ns);
    string s = norm->to_string();

    cout << v->to_string() << endl;
    cout << s << endl;
    cout << endl;

    if (seen.find(s) == seen.end()) {
      seen.insert(s);
      result.push_back(v);
    }
  }
  return result;
}*/

// This one is more thorough (cuts by about 3x) but also slower,
// so we still do the first one to save time.
vector<value> remove_equiv2(vector<value> const& values) {
  vector<value> result;
  set<ComparableValue> seen;
  int i = 0;
  for (value v : values) {
    i++;
    //if (i % 1000 == 0) printf("i = %d\n", i);
    value norm = v->totally_normalize();
    //string s = norm->to_string();

    //cout << v->to_string() << endl;
    //cout << s << endl;
    //cout << endl;

    ComparableValue cv(norm);

    if (seen.find(cv) == seen.end()) {
      seen.insert(cv);
      result.push_back(v);
    }
  }
  return result;
}

void sort_values(vector<value>& values) {
  std::sort(values.begin(), values.end(), [](value a, value b) {
    return lt_value(a, b);
  });
}

vector<value> enumerate_for_template(
    shared_ptr<Module> module,
    value templ, int k)
{
  vector<HoleInfo> all_hole_info = getHoleInfo(templ);
  vector<vector<value>> all_hole_fills;
  for (HoleInfo hi : all_hole_info) {
    vector<value> fills = gen_clauses(module, hi.decls);

    /*cout << "fills" << endl;
    cout << "===========================================" << endl;
    for (value v : fills) {
      cout << v->to_string() << endl;
    }

    cout << "===========================================" << endl;
    cout << "new fills" << endl;
    for (value v : fills1) {
      cout << v->to_string() << endl;
    }
    cout << "===========================================" << endl;
    assert(false);*/

    //cout << "fills len " << fills.size() << endl;

    vector<value> f1 = filter_boring(fills);
    vector<value> f2 = enum_conjuncts(f1, k);
    vector<value> f3 = f2; //remove_equiv(templ, f2);

    //cout << "f1 len " << f1.size() << endl;
    //cout << "f2 len " << f2.size() << endl;
    //cout << "f3 len " << f3.size() << endl;

    for (int i = 0; i < (int)f3.size(); i++) {
      f3[i] = v_not(f3[i]);
    }
    all_hole_fills.push_back(move(f3));
  }

  assert (all_hole_fills.size() > 0);
  vector<value> res = fill_holes(templ, all_hole_fills);
  //vector<value> final = remove_equiv2(res);

  //cout << "res len " << res.size() << endl;
  //return make_pair(final, res);
  return res;
}

vector<value> get_clauses_for_template(
    std::shared_ptr<Module> module, 
    value templ)
{
  return enumerate_for_template(module, templ, 1);
}
