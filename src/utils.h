#ifndef UTILS_H
#define UTILS_H

#include "logic.h"

bool is_redundant_quick(value a, value b);

std::vector<int> sort_and_uniquify(std::vector<int> const& v);

template <typename T>
void vector_append(std::vector<T>& a, std::vector<T> const& b)
{
  for (int i = 0; i < (int)b.size(); i++) {
    a.push_back(b[i]);
  }
}

#endif
