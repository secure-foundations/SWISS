#ifndef BITSET_EVAL_RESULT_H
#define BITSET_EVAL_RESULT_H

#include <iostream>

struct BitsetEvalResult {
  std::vector<uint64_t> v;
  uint64_t last_bits;

  static BitsetEvalResult eval_over_foralls(std::shared_ptr<Model>, value v);

  static bool disj_is_true(std::vector<BitsetEvalResult*> const& results) {
    int n = results[0]->v.size();
    for (int i = 0; i < n - 1; i++) {
      uint64_t x = 0;
      for (int j = 0; j < results.size(); j++) {
        x |= results[j]->v[i];
      }
      if (x != ~(uint64_t)0) {
        return false;
      }
    }

    uint64_t x = 0;
    for (int j = 0; j < results.size(); j++) {
      x |= results[j]->v[n-1];
    }
    if (x != results[0]->last_bits) {
      return false;
    }

    return true;
  }

  void dump() {
    for (int i = 0; i < v.size(); i++) {
      for (int j = 0; j < 64; j++) {
        if (i < v.size() - 1 || (((uint64_t)1 << j) & last_bits)) {
          std::cout << (v[i] & ((uint64_t)1 << j) ? 1 : 0);
        }
      }
    }
    std::cout << std::endl;
  }
};

#endif
