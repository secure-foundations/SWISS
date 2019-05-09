#include "sat_solver.h"

#include <iostream>

using namespace std;

enum class se_type {
  Var,
  And,
  Or,
};

struct sat_expr_ {
  se_type ty; 

  // For vars:
  string var_name;
  Glucose::Var var;
  bool negated;

  // Everything else:
  vector<sat_expr> args;

  sat_expr_(se_type ty) : ty(ty) {}
};

sat_expr const _sat_true(shared_ptr<sat_expr_>(new sat_expr_(se_type::And)));
sat_expr const _sat_false(shared_ptr<sat_expr_>(new sat_expr_(se_type::Or)));

vector<pair<string, sat_expr>> names;

sat_expr SatSolver::new_sat_var(std::string const& name) {
  sat_expr se;
  se.se.reset(new sat_expr_(se_type::Var));
  se.se->var_name = name;
  se.se->var = solver.newVar();
  se.se->negated = false;

  printf("idx for %s is %d\n", name.c_str(), (int)se.se->var);
  names.push_back(make_pair(name, se));

  return se;
}

template <typename T>
void extend(vector<T> & a, vector<T> const& b) {
  for (T const& t : b) {
    a.push_back(t);
  }
}

sat_expr sat_and(sat_expr a, sat_expr b) {
  return sat_and({a, b});
}

sat_expr sat_and(vector<sat_expr> const& args) {
  vector<sat_expr> new_args;
  for (sat_expr child : args) {
    if (child.se->ty == se_type::And) {
      extend(new_args, child.se->args);
    } else if (child.se->ty == se_type::Or && child.se->args.size() == 0) {
      return child;
    } else {
      new_args.push_back(child);
    }
  }

  if (new_args.size() == 1) {
    return new_args[0];
  } else {
    sat_expr se;
    se.se.reset(new sat_expr_(se_type::And));
    se.se->args = move(new_args);
    return se;
  }
}

sat_expr sat_or(sat_expr a, sat_expr b) {
  return sat_or({a, b});
}

sat_expr sat_or(vector<sat_expr> const& args) {
  vector<sat_expr> new_args;
  for (sat_expr child : args) {
    if (child.se->ty == se_type::Or) {
      extend(new_args, child.se->args);
    } else if (child.se->ty == se_type::And && child.se->args.size() == 0) {
      return child;
    } else {
      new_args.push_back(child);
    }
  }

  if (new_args.size() == 1) {
    return new_args[0];
  } else {
    sat_expr se;
    se.se.reset(new sat_expr_(se_type::Or));
    se.se->args = move(new_args);
    return se;
  }
}

sat_expr sat_implies(sat_expr a, sat_expr b) {
  return sat_or(sat_not(a), b);
}

bool is_true(sat_expr se) {
  return se.se->ty == se_type::And && se.se->args.size() == 0;
}

bool is_false(sat_expr se) {
  return se.se->ty == se_type::Or && se.se->args.size() == 0;
}

sat_expr sat_ite(sat_expr a, sat_expr b, sat_expr c) {
  if (is_true(a)) return b;
  if (is_false(a)) return c;
  if (is_true(b)) return sat_or(a, c);
  if (is_false(b)) return sat_and(sat_not(a), c);
  if (is_true(c)) return sat_or(sat_not(a), b);
  if (is_false(c)) return sat_and(a, b);
  return sat_and(sat_or(sat_not(a), b), sat_or(a, c));
}

sat_expr sat_not(sat_expr se_) {
  shared_ptr<sat_expr_> se = se_.se;
  if (se->ty == se_type::Var) {
    sat_expr nse;
    nse.se.reset(new sat_expr_(se_type::Var));
    nse.se->var_name = se->var_name;
    nse.se->var = se->var;
    nse.se->negated = !se->negated;
    return nse;
  }
  else if (se->ty == se_type::And) {
    sat_expr nse;
    nse.se.reset(new sat_expr_(se_type::Or));
    for (sat_expr arg : se->args) {
      nse.se->args.push_back(sat_not(arg));
    }
    return nse;
  }
  else if (se->ty == se_type::Or) {
    sat_expr nse;
    nse.se.reset(new sat_expr_(se_type::And));
    for (sat_expr arg : se->args) {
      nse.se->args.push_back(sat_not(arg));
    }
    return nse;
  }
  /*
  else if (se->ty == se_type::Ite) {
    return sat_ite(se->args[0], sat_not(se->args[1]), sat_not(se->args[2]));
  }
  */
  else {
    assert(false);
  }
}

void SatSolver::add(sat_expr se) {
  cout << se.to_string() << "\n";
  this->add_(se);
}

void SatSolver::add_(sat_expr se_) {
  shared_ptr<sat_expr_> se = se_.se;
  if (se->ty == se_type::Var) {
    solver.addClause(Glucose::mkLit(se->var, se->negated));
  }
  else if (se->ty == se_type::And) {
    for (sat_expr child : se->args) {
      this->add_(child);
    }
  }
  else if (se->ty == se_type::Or) {
    Glucose::vec<Glucose::Lit> lits(se->args.size());
    for (int i = 0; i < se->args.size(); i++) {
      lits[i] = expr_to_lit(se->args[i], false);
    }
    solver.addClause(lits);
  }
  /*
  else if (se->ty == se_type::Ite) {
    Glucose::Lit cond_true = expr_to_lit(se->args[0], false);
    Glucose::Lit cond_false = expr_to_lit(se->args[0], true);
    Glucose::Lit a = expr_to_list(se->args[1]);
    Glucose::Lit b = expr_to_list(se->args[2]);
    solver.addClause(cond_false, a);
    solver.addClause(cond_true, b);
  }
  */
  else {
    assert(false);
  }
}

Glucose::Lit SatSolver::expr_to_lit(sat_expr se_, bool negate) {
  shared_ptr<sat_expr_> se = se_.se;
  if (se->ty == se_type::Var) {
    return Glucose::mkLit(se->var, negate ^ se->negated);
  }
  else if ((se->ty == se_type::And && !negate) || (se->ty == se_type::Or && negate)) {
    Glucose::Var v = solver.newVar();
    Glucose::Lit l = Glucose::mkLit(v);
    Glucose::Lit neg_l = Glucose::mkLit(v, true);
    for (sat_expr child : se->args) {
      solver.addClause(neg_l, expr_to_lit(child, negate));
    }
    return l;
  }
  else if ((se->ty == se_type::And && negate) || (se->ty == se_type::Or && !negate)) {
    Glucose::Var v = solver.newVar();
    Glucose::Lit l = Glucose::mkLit(v);
    Glucose::Lit neg_l = Glucose::mkLit(v, true);
    Glucose::vec<Glucose::Lit> vec(se->args.size() + 1);
    vec[0] = neg_l;
    for (int i = 0; i < se->args.size(); i++) {
      vec[i+1] = expr_to_lit(se->args[i], negate);
    }
    solver.addClause(vec);
    return l;
  }
  else {
    assert(false);
  }
}

bool SatSolver::get(sat_expr se) {
  assert(got_sat);
  assert(se.se->ty == se_type::Var);
  return solver.modelValue(se.se->var) == (se.se->negated ? l_False : l_True);
}

bool SatSolver::is_sat() {
  this->got_sat = solver.solve();
  printf("got_sat = %d\n", (int)got_sat);

  for (auto p : names) {
    cout << p.first << " : " << get(p.second) << endl;
  }
  for (int i = 0; i < solver.nVars(); i++) {
    printf("var %d : %d \n", i, toInt(solver.modelValue(i)));
  }
  static bool b = false;
  if (!b) b = true;
  else assert(false);

  return this->got_sat;
}

string sat_expr::to_string(int indent) const {
  string tabs;
  for (int i = 0; i < indent; i++) {
    tabs = tabs + " ";
  }

  if (se->ty == se_type::Var) {
    return tabs + (se->negated ? "!" : "") + se->var_name;
  }
  else if (se->ty == se_type::And || se->ty == se_type::Or) {
    string s = tabs + (se->ty == se_type::And ? "(and" : "(or") + "\n";
    for (sat_expr child : se->args) {
      s += child.to_string(indent + 2) + "\n";
    }
    s += tabs + ")";
    return s;
  }
  else {
    assert(false);
  }
}

void test_sat() {
/*
  {
  SatSolver solver;
  sat_expr a = solver.new_sat_var("a");
  sat_expr b = solver.new_sat_var("b");
  sat_expr c = solver.new_sat_var("c");
  printf("sat: %s\n", solver.is_sat() ? "sat" : "unsat");
  printf("%d\n", solver.get(a));
  printf("%d\n", solver.get(b));
  printf("%d\n", solver.get(c));
  }
  */

  {
  SatSolver solver;
  sat_expr a = solver.new_sat_var("a");
  sat_expr b = solver.new_sat_var("b");
  sat_expr c = solver.new_sat_var("c");
  solver.add(sat_and(sat_or(a, b), sat_or(sat_not(a), sat_not(c))));
  printf("sat: %s\n", solver.is_sat() ? "sat" : "unsat");
  printf("%d\n", solver.get(a));
  printf("%d\n", solver.get(b));
  printf("%d\n", solver.get(c));
  }
  return;

  {
  SatSolver solver;
  sat_expr a = solver.new_sat_var("a");
  sat_expr b = solver.new_sat_var("b");
  sat_expr c = solver.new_sat_var("c");
  solver.add(sat_or(
    sat_and({ a, b, sat_not(c) }),
    sat_and({ a, sat_not(b), sat_not(c) }) ));
  solver.add(sat_not(a));
  printf("sat: %s\n", solver.is_sat() ? "sat" : "unsat");
  /*
  printf("%d\n", solver.get(a));
  printf("%d\n", solver.get(b));
  printf("%d\n", solver.get(c));
  */
  }

  {
  SatSolver solver;
  sat_expr a = solver.new_sat_var("a");
  sat_expr b = solver.new_sat_var("b");
  sat_expr c = solver.new_sat_var("c");
  sat_expr d = solver.new_sat_var("d");
  solver.add(a);
  solver.add(b);
  solver.add(sat_or(sat_not(a), sat_and(sat_not(b), sat_or(c, d))));
  printf("sat: %s\n", solver.is_sat() ? "sat" : "unsat");
  }
}
