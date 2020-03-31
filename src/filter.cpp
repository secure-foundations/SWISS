#include "filter.h"
#include "contexts.h"

using namespace std;

bool is_necessary(
    smt::context& ctx,
    shared_ptr<Module> module,
    vector<value> const& values,
    int i)
{
  BasicContext basic_ctx(ctx, module);
  smt::solver& solver = basic_ctx.ctx->solver;

  for (int j = 0; j < (int)values.size(); j++) {
    if (j == i) {
      solver.add(basic_ctx.e->value2expr(v_not(values[j])));
    } else {
      solver.add(basic_ctx.e->value2expr(values[j]));
    }
  }

  return solver.check_sat();
}

vector<value> filter_redundant_formulas(
  shared_ptr<Module> module,
  vector<value> const& values0)
{
  vector<value> values;

  set<ComparableValue> cvs;
  for (int i = 0; i < (int)values0.size(); i++) {
    ComparableValue cv(values0[i]);
    if (cvs.find(cv) == cvs.end()) {
      values.push_back(values0[i]);
      cvs.insert(values0[i]);
    }
  }

  smt::context ctx;

  for (int i = 0; i < (int)values.size(); i++) {
    if (!is_necessary(ctx, module, values, i)) {
      for (int j = i; j < (int)values.size() - 1; j++) {
        values[j] = values[j+1];
      }
      values.pop_back();
      i--;
    }
  }

  return values;
}
