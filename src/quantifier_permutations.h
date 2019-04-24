#ifndef QUANTIFIER_PERMUTATIONS_H
#define QUANTIFIER_PERMUTATIONS_H

#include "logic.h"

std::vector<std::vector<unsigned int>> get_quantifier_permutations(
    std::vector<VarDecl> const& quantifiers,
    std::vector<unsigned int> const& ovs);

std::vector<std::vector<std::vector<unsigned int>>> get_multiqi_quantifier_permutations(
    std::vector<VarDecl> const& quantifiers,
    std::vector<std::vector<unsigned int>> const& ovs);

#endif
