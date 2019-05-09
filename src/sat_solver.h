#ifndef SAT_SOLVER_H
#define SAT_SOLVER_H

#include <string>
#include <vector>
#include <memory>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wlogical-op-parentheses"
#include "lib/glucose-syrup/simp/SimpSolver.h"
#pragma GCC diagnostic pop

struct sat_expr_;
struct sat_expr {
  std::shared_ptr<sat_expr_> se;

  sat_expr() {}
  sat_expr(std::shared_ptr<sat_expr_> se) : se(se) { }

  std::string to_string(int indent = 0) const;
};

class SatSolver {
public:
  SatSolver() { solver.verbosity = -1; this->got_sat = false; }

  void add(sat_expr);
  bool is_sat();
  bool get(sat_expr);

  sat_expr new_sat_var(std::string const& name);

private:
  Glucose::SimpSolver solver;

  Glucose::Lit expr_to_lit(sat_expr, bool negate);
  void add_(sat_expr);

  bool got_sat;
};

sat_expr sat_and(sat_expr, sat_expr);
sat_expr sat_and(std::vector<sat_expr> const&);
sat_expr sat_or(sat_expr, sat_expr);
sat_expr sat_or(std::vector<sat_expr> const&);
sat_expr sat_implies(sat_expr, sat_expr);
sat_expr sat_not(sat_expr);
sat_expr sat_ite(sat_expr, sat_expr, sat_expr);

extern sat_expr const _sat_true;
extern sat_expr const _sat_false;

inline sat_expr sat_true() { return _sat_true; }
inline sat_expr sat_false() { return _sat_false; }

void test_sat();

#endif
