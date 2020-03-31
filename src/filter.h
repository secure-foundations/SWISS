#ifndef FILTER_H
#define FILTER_H

#include "logic.h"

std::vector<value> filter_redundant_formulas(
  std::shared_ptr<Module>,
  std::vector<value> const&);

#endif
