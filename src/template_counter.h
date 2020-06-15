#ifndef TEMPLATE_COUNTER_H
#define TEMPLATE_COUNTER_H

#include "logic.h"
#include "var_lex_graph.h"

//long long count_space(std::shared_ptr<Module> module, int k, int maxVars);
long long count_template(
    std::shared_ptr<Module> module,
    value templ,
    int k,
    bool depth2,
    bool useAllVars);

struct EnumInfo {
  std::vector<value> clauses;
  std::vector<VarIndexTransition> var_index_transitions;

  EnumInfo(std::shared_ptr<Module>, value templ);
};

#endif
