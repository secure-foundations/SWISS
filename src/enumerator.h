#ifndef ENUMERATOR_H
#define ENUMERATOR_H

//#include "expr_gen_smt.h"
#include "logic.h"

std::vector<value> get_clauses_for_template(
    std::shared_ptr<Module> module, 
    value templ);

#endif
