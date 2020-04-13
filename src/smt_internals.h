#ifndef SMT_INTERNALS_H
#define SMT_INTERNALS_H

#include <cvc4/cvc4.h>
#include "z3++.h"
#include "smt.h"

namespace smt {
  bool is_z3_context(context&);
  CVC4::ExprManager& get_expr_manager(context&);
  CVC4::SmtEngine& get_smt_engine(solver&);
}

#endif
