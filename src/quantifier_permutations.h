#ifndef QUANTIFIER_PERMUTATIONS_H
#define QUANTIFIER_PERMUTATIONS_H

#include "logic.h"
#include "top_quantifier_desc.h"

std::vector<std::vector<unsigned int>> get_quantifier_permutations(
    TopQuantifierDesc const& tqd,
    std::vector<unsigned int> const& ovs);

std::vector<std::vector<std::vector<unsigned int>>> get_multiqi_quantifier_permutations(
    TopQuantifierDesc const& tqd,
    std::vector<std::vector<unsigned int>> const& ovs);

#endif
