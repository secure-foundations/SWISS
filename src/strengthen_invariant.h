#ifndef STRENGTHEN_INVARIANT_H
#define STRENGTHEN_INVARIANT_H

#include "logic.h"

value strengthen_invariant(
  std::shared_ptr<Module> module,
  value invariant_so_far,
  value new_invariant);

#endif
