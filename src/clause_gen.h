#ifndef CLAUSE_GEN_H
#define CLAUSE_GEN_H

#include "logic.h"

std::vector<value> gen_clauses(
    std::shared_ptr<Module>,
    std::vector<VarDecl> const& decls);

#endif
