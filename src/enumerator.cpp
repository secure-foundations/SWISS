#include "enumerator.h"

#include <set>
#include <algorithm>

#include "logic.h"
#include "grammar.h"
#include "smt.h"

using namespace std;

struct HoleInfo {
  vector<GrammarVar> vars;
};

void getHoleInfo_(value v, vector<GrammarVar> vars, vector<HoleInfo>& res) {
  assert(v.get() != NULL);
  if (Forall* va = dynamic_cast<Forall*>(v.get())) {
    for (VarDecl decl : va->decls) {
      if (UninterpretedSort* usort = dynamic_cast<UninterpretedSort*>(decl.sort.get())) {
        vars.push_back(GrammarVar(iden_to_string(decl.name), usort->name));
      }
    }
    getHoleInfo_(va->body, vars, res);
  }
  else if (Exists* va = dynamic_cast<Exists*>(v.get())) {
    for (VarDecl decl : va->decls) {
      if (UninterpretedSort* usort = dynamic_cast<UninterpretedSort*>(decl.sort.get())) {
        vars.push_back(GrammarVar(iden_to_string(decl.name), usort->name));
      }
    }
    getHoleInfo_(va->body, vars, res);
  }
  else if (Var* va = dynamic_cast<Var*>(v.get())) {
    return;
  }
  else if (Const* va = dynamic_cast<Const*>(v.get())) {
    return;
  }
  else if (Eq* va = dynamic_cast<Eq*>(v.get())) {
    getHoleInfo_(va->left, vars, res);
    getHoleInfo_(va->right, vars, res);
  }
  else if (Not* va = dynamic_cast<Not*>(v.get())) {
    getHoleInfo_(va->val, vars, res);
  }
  else if (Implies* va = dynamic_cast<Implies*>(v.get())) {
    getHoleInfo_(va->left, vars, res);
    getHoleInfo_(va->right, vars, res);
  }
  else if (Apply* va = dynamic_cast<Apply*>(v.get())) {
    getHoleInfo_(va->func, vars, res);
    for (value arg : va->args) {
      getHoleInfo_(arg, vars, res);
    }
  }
  else if (And* va = dynamic_cast<And*>(v.get())) {
    for (value arg : va->args) {
      getHoleInfo_(arg, vars, res);
    }
  }
  else if (Or* va = dynamic_cast<Or*>(v.get())) {
    for (value arg : va->args) {
      getHoleInfo_(arg, vars, res);
    }
  }
  else if (TemplateHole* value = dynamic_cast<TemplateHole*>(v.get())) {
    HoleInfo hi;
    hi.vars = vars;
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

void add_constraints(shared_ptr<Module> module, SMT& solver) {
  for (string so : module->sorts) {
    string eq = "=." + so;
    string neq = "~=." + so;
    solver.createBinaryAssociativeConstraints(eq); // a = b <-> b = a
    solver.createBinaryAssociativeConstraints(neq); // a ~= b <- b ~= a
    solver.createAllDiffConstraints(eq); // does not allow forall. x: x = x
    solver.createAllDiffConstraints(neq); // does not allow forall. x: x ~= x
  }

  bool has_le = false;
  for (VarDecl decl : module->functions) {
    if (iden_to_string(decl.name) == "le") {
      has_le = true;
    }
  }

  if (has_le) {
    solver.createAllDiffGrandChildrenConstraints("le"); // does not allow forall. x: le (pnd x) (pnd x)
    solver.createAllDiffGrandChildrenConstraints("~le"); // does not allow forall. x: ~le (pnd x) (pnd x)
  }

  bool has_btw = false;
  for (VarDecl decl : module->functions) {
    if (iden_to_string(decl.name) == "btw") {
      has_btw = true;
    }
  }

  // ring properties:
  // ABC -> BCA -> CAB -> ABC
  // ACB -> CBA -> BAC -> ACB
  // only allow ABC or ACB, i.e. do not allow the others
  if (has_btw) {
    solver.breakOccurrences("btw","B","C","A");
    solver.breakOccurrences("btw","C","A","B");
    solver.breakOccurrences("btw","C","B","A");
    solver.breakOccurrences("btw","B","A","C");
    solver.breakOccurrences("~btw","B","C","A");
    solver.breakOccurrences("~btw","C","A","B");
    solver.breakOccurrences("~btw","C","B","A");
    solver.breakOccurrences("~btw","B","A","C");

    solver.createAllDiffConstraints("~btw");
    solver.createAllDiffConstraints("btw");
  }
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
  else if (Var* va = dynamic_cast<Var*>(templ.get())) {
    return templ;
  }
  else if (Const* va = dynamic_cast<Const*>(templ.get())) {
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
  else if (TemplateHole* value = dynamic_cast<TemplateHole*>(templ.get())) {
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
  assert(idx == fills.size());
  return res;
}

vector<value> fill_holes(value templ, vector<vector<value>> const& fills) {
  vector<value> result;

  vector<int> indices;
  for (int i = 0; i < fills.size(); i++) {
    indices.push_back(0);
  }
  while (true) {
    vector<value> values_to_fill;
    for (int i = 0; i < fills.size(); i++) {
      assert(0 <= indices[i] && indices[i] < fills[i].size());
      values_to_fill.push_back(fills[i][indices[i]]);
    }

    int idx = 0;
    result.push_back(fill_holes_in_value(templ, values_to_fill, idx));
    assert(idx == fills.size());
    
    int i;
    for (i = 0; i < fills.size(); i++) {
      indices[i]++;
      if (indices[i] == fills[i].size()) {
        indices[i] = 0;
      } else {
        break;
      }
    }
    if (i == fills.size()) {
      break;
    }
  }

  return result;
}

void enum_conjuncts_(
    vector<value> const& pieces, int k,
    int idx, vector<value> acc, vector<value>& result) {
  if (acc.size() == k) {
    result.push_back(v_and(acc));
    return;
  }
  
  for (int i = idx + 1; i < pieces.size(); i++) {
    if (i == idx+1) acc.push_back(pieces[i]);
    else acc[acc.size() - 1] = pieces[i];

    enum_conjuncts_(pieces, k, i, acc, result);
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
    for (int i = 0; i < names.size(); i++) {
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
  else if (Const* va = dynamic_cast<Const*>(v.get())) {
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
    Grammar grammar = createGrammarFromModule(module, hi.vars);
    context z3_ctx;
    solver z3_solver(z3_ctx);
    SMT solver = SMT(grammar, z3_ctx, z3_solver, 1);
    add_constraints(module, solver);
    vector<value> fills;
    while (solver.solve()) {
      fills.push_back(solver.solutionToValue());
    }

    sort_values(fills);

    //for (value v : fills) {
    //  cout << v->to_string() << endl;
    //}

    //cout << "fills len " << fills.size() << endl;

    vector<value> f1 = filter_boring(fills);
    vector<value> f2 = enum_conjuncts(f1, k);
    vector<value> f3 = f2; //remove_equiv(templ, f2);

    //cout << "f1 len " << f1.size() << endl;
    //cout << "f2 len " << f2.size() << endl;
    //cout << "f3 len " << f3.size() << endl;

    for (int i = 0; i < f3.size(); i++) {
      f3[i] = v_not(f3[i]);
    }
    all_hole_fills.push_back(move(f3));
  }

  vector<value> res = fill_holes(templ, all_hole_fills);
  res = remove_equiv2(res);

  //cout << "res len " << res.size() << endl;
  return res;
}

// XXX this doesn't make any sense
vector<value> enumerate_fills_for_template(
    shared_ptr<Module> module,
    value templ)
{
  vector<HoleInfo> all_hole_info = getHoleInfo(templ);
  vector<vector<value>> all_hole_fills;
  for (HoleInfo hi : all_hole_info) {
    Grammar grammar = createGrammarFromModule(module, hi.vars);
    context z3_ctx;
    solver z3_solver(z3_ctx);
    SMT solver = SMT(grammar, z3_ctx, z3_solver, 1);
    add_constraints(module, solver);
    vector<value> fills;
    while (solver.solve()) {
      fills.push_back(solver.solutionToValue());
    }
    fills = filter_boring(fills);
    for (int i = 0; i < fills.size(); i++) {
      fills[i] = fills[i]->negate();
    }
    all_hole_fills.push_back(move(fills));
  }

  vector<value> res = fill_holes(templ, all_hole_fills);
  return res;
}
