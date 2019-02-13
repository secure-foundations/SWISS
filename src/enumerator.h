#ifndef ENUMERATOR_H
#define ENUMERATOR_H

#include "smt.h"
#include "logic.h"

void add_constraints(std::shared_ptr<Module> module, SMT& solver);

std::vector<value> enumerate_for_template(
    std::shared_ptr<Module> module,
    value templ);

#endif
