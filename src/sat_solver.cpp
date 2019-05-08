#include "sat_solver.h"

using namespace std;

enum class se_type {
  Var,
  And,
  Or,
  Not,
};

struct sat_expr_ {
  se_type ty; 

  // For vars:
  string var_name;
  Glucose::Var var;

  // Everything else:
  vector<sat_expr> args;
};

sat_expr SatSolver::new_sat_var(std::string const& name) {
  sat_expr se;
  se.se.reset(new sat_expr_());
  se.se->ty = se_type::Var;
  se.se->var_name = name;
  se.se->var = solver.newVar();

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
  sat_expr se;
  se.se.reset(new sat_expr_());
  se.se->ty = se_type::And;
  for (sat_expr child : args) {
    if (child.se->ty == se_type::And) {
      extend(se.se->args, child.se->args);
    } else {
      se.se->args.push_back(child);
    }
  }
  return se;
}

sat_expr sat_or(sat_expr a, sat_expr b) {
  return sat_or({a, b});
}

sat_expr sat_or(vector<sat_expr> const& args) {
  sat_expr se;
  se.se.reset(new sat_expr_());
  se.se->ty = se_type::Or;
  for (sat_expr child : args) {
    if (child.se->ty == se_type::Or) {
      extend(se.se->args, child.se->args);
    } else {
      se.se->args.push_back(child);
    }
  }
  return se;
}

sat_expr sat_impl(sat_expr a, sat_expr b) {
  return sat_or(sat_not(a), b);
}

sat_expr sat_not(sat_expr a) {
  sat_expr se;
  se.se.reset(new sat_expr_());
  se.se->ty = se_type::Not;
  se.se->args.push_back(a);
  return se;
}

void SatSolver::add(sat_expr se_, bool negate) {
  shared_ptr<sat_expr_> se = se_.se;
  if (se->ty == se_type::Var) {
    solver.addClause(Glucose::mkLit(se->var, negate));
  }
  else if ((se->ty == se_type::And && !negate) || (se->ty == se_type::Or && negate)) {
    for (sat_expr child : se->args) {
      this->add(child, negate);
    }
  }
  else if (se->ty == se_type::Not) {
    this->add(se->args[0], !negate);
  }
  else if ((se->ty == se_type::And && negate) || (se->ty == se_type::Or && !negate)) {
    Glucose::vec<Glucose::Lit> lits(se->args.size());
    for (int i = 0; i < se->args.size(); i++) {
      lits[i] = expr_to_lit(se->args[i], negate);
    }
    solver.addClause(lits);
  }
  else {
    assert(false);
  }
}

Glucose::Lit SatSolver::expr_to_lit(sat_expr se_, bool negate) {
  shared_ptr<sat_expr_> se = se_.se;
  if (se->ty == se_type::Var) {
    return Glucose::mkLit(se->var, negate);
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
  else if (se->ty == se_type::Not) {
    return expr_to_lit(se->args[0], !negate);
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
  assert(se.se->ty == se_type::Var);
  return solver.modelValue(se.se->var) == l_True;
}

bool SatSolver::is_sat() {
  return solver.solve();
}

void test_sat() {
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

  {
  SatSolver solver;
  sat_expr a = solver.new_sat_var("a");
  sat_expr b = solver.new_sat_var("b");
  sat_expr c = solver.new_sat_var("c");
  solver.add(sat_or(
    sat_and({ a, b, sat_not(c) }),
    sat_and({ a, sat_not(b), sat_not(c) }) ));
  printf("sat: %s\n", solver.is_sat() ? "sat" : "unsat");
  printf("%d\n", solver.get(a));
  printf("%d\n", solver.get(b));
  printf("%d\n", solver.get(c));
  }
}
