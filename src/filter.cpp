#include "filter.h"
#include "contexts.h"

using namespace std;

bool is_necessary(
    z3::context& ctx,
    shared_ptr<Module> module,
    vector<value> values,
    int i)
{
  BasicContext basic_ctx(ctx, module);
  for (int j = 0; j < values.size(); j++) {
    if (j == i) {
      basic_ctx.e->value2expr(v_not(values[i]));
    } else {
      basic_ctx.e->value2expr(values[i]);
    }
  }

  z3::solver& solver = basic_ctx.ctx->solver;
  z3::check_result res = solver.check();
  assert (res == z3::sat || res == z3::unsat);

  return res == z3::sat;
}

vector<value> filter_redundant_formulas(
  shared_ptr<Module> module,
  vector<value> values)
{
  z3::context ctx;

  for (int i = 0; i < values.size(); i++) {
    if (!is_necessary(ctx, module, values, i)) {
      for (int j = i; j < values.size() - 1; j++) {
        values[j] = values[j+1];
      }
      values.pop_back();
      i--;
    }
  }

  return values;
}
