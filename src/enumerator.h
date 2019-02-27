#ifndef ENUMERATOR_H
#define ENUMERATOR_H

#include "smt.h"
#include "logic.h"

void add_constraints(std::shared_ptr<Module> module, SMT& solver);

std::vector<value> enumerate_for_template(
    std::shared_ptr<Module> module,
    value templ, int k = 3);

std::vector<value> enumerate_fills_for_template(
    std::shared_ptr<Module> module,
    value templ);

std::vector<value> remove_equiv2(std::vector<value> const& values);

#endif
