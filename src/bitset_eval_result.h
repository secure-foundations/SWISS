#ifndef BITSET_EVAL_RESULT_H
#define BITSET_EVAL_RESULT_H

#include <iostream>
#include <cstring>

struct BitsetEvalResult {
  std::vector<uint64_t> v;
  uint64_t last_bits;

  static BitsetEvalResult eval_over_foralls(std::shared_ptr<Model>, value v);

  static BitsetEvalResult eval_over_alternating_quantifiers(std::shared_ptr<Model>, value v);

  static bool disj_is_true(std::vector<BitsetEvalResult*> const& results) {
    int n = results[0]->v.size();
    for (int i = 0; i < n - 1; i++) {
      uint64_t x = 0;
      for (int j = 0; j < (int)results.size(); j++) {
        x |= results[j]->v[i];
      }
      if (x != ~(uint64_t)0) {
        return false;
      }
    }

    uint64_t x = 0;
    for (int j = 0; j < (int)results.size(); j++) {
      x |= results[j]->v[n-1];
    }
    if (x != results[0]->last_bits) {
      return false;
    }

    return true;
  }

  void dump() {
    for (int i = 0; i < (int)v.size(); i++) {
      for (int j = 0; j < 64; j++) {
        if (i < (int)v.size() - 1 || (((uint64_t)1 << j) & last_bits)) {
          std::cout << (v[i] & ((uint64_t)1 << j) ? 1 : 0);
        }
      }
    }
    std::cout << std::endl;
  }
};

struct BitsetLevel {
  int block_size;
  int num_blocks;
  bool conj;
};

struct AlternationBitsetEvaluator {
  std::vector<BitsetLevel> levels;
  std::vector<uint64_t> scratch;

  bool final_conj;
  int final_num_full_words_64;
  uint64_t final_last_bits;

  static AlternationBitsetEvaluator make_evaluator(
      std::shared_ptr<Model> model, value v);

  void reset_for_conj() {
    for (int i = 0; i < (int)scratch.size(); i++) {
      scratch[i] = ~(uint64_t)0;
    }
    scratch[scratch.size() - 1] = final_last_bits;
  }
  void reset_for_disj() {
    for (int i = 0; i < (int)scratch.size(); i++) {
      scratch[i] = 0;
    }
  }

  void add_conj(BitsetEvalResult const& ber) {
    for (int i = 0; i < (int)scratch.size(); i++) {
      scratch[i] &= ber.v[i];
    }
  }
  void add_disj(BitsetEvalResult const& ber) {
    for (int i = 0; i < (int)scratch.size(); i++) {
      scratch[i] |= ber.v[i];
    }
  }

  static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__, "this requires little endian");

  // scratch[0 .. len] := scratch[0 .. len] & scratch[start .. start + len]
  void block_conj(int start, int len) {
    // word of 4 bytes at a time
    int t;
    for (t = 0; 32*t <= len - 32; t++) {
      int bit_idx = 32*t + start;
      int w32_idx = bit_idx / 32;
      uint64_t w64;
      memcpy(&w64, ((uint32_t*)&scratch[0]) + w32_idx, 8);
      uint32_t block = (uint32_t)(w64 >> ((start % 32)));

      uint32_t tmp;
      memcpy(&tmp, ((uint32_t*)&scratch[0]) + t, 4);
      tmp &= block;
      memcpy(((uint32_t*)&scratch[0]) + t, &tmp, 4);
    }

    if (32*t < len) {
      int bit_idx = 32*t + start;
      int w32_idx = bit_idx / 32;
      uint64_t w64;
      memcpy(&w64, ((uint32_t*)&scratch[0]) + w32_idx, 8);
      uint32_t block = (uint32_t)(w64 >> ((start % 32)));

      uint32_t tmp;
      memcpy(&tmp, ((uint32_t*)&scratch[0]) + t, 4);
      tmp &= block | ((uint32_t)(-1) << (len - 32*t));
      memcpy(((uint32_t*)&scratch[0]) + t, &tmp, 4);
    }
  }

  // scratch[0 .. len] := scratch[0 .. len] | scratch[start .. start + len]
  void block_disj(int start, int len) {
    int t;
    for (t = 0; 32*t <= len - 32; t++) {
      int bit_idx = 32*t + start;
      int w32_idx = bit_idx / 32;
      uint64_t w64;
      memcpy(&w64, ((uint32_t*)&scratch[0]) + w32_idx, 8);
      uint32_t block = (uint32_t)(w64 >> ((start % 32)));

      uint32_t tmp;
      memcpy(&tmp, ((uint32_t*)&scratch[0]) + t, 4);
      tmp |= block;
      memcpy(((uint32_t*)&scratch[0]) + t, &tmp, 4);
    }

    if (32*t < len) {
      int bit_idx = 32*t + start;
      int w32_idx = bit_idx / 32;
      uint64_t w64;
      memcpy(&w64, ((uint32_t*)&scratch[0]) + w32_idx, 8);
      uint32_t block = (uint32_t)(w64 >> ((start % 32)));

      uint32_t tmp;
      memcpy(&tmp, ((uint32_t*)&scratch[0]) + t, 4);
      tmp |= block & (((uint32_t)(1) << (len - 32*t)) - 1);
      memcpy(((uint32_t*)&scratch[0]) + t, &tmp, 4);
    }
  }

  bool final_answer() {
    if (final_conj) {
      for (int j = 0; j < final_num_full_words_64; j++) {
        if (scratch[j] != ~(uint64_t)0) {
          return false;
        }
      }
      return (scratch[final_num_full_words_64] & final_last_bits)
          == final_last_bits;
    } else {
      for (int j = 0; j < final_num_full_words_64; j++) {
        if (scratch[j] != 0) {
          return true;
        }
      }
      return (scratch[final_num_full_words_64] & final_last_bits) != 0;
    }
  }

  bool evaluate() {
    for (int i = 0; i < (int)levels.size(); i++) {
      BitsetLevel& level = levels[i];
      if (level.conj) {
        for (int j = 1; j < level.num_blocks; j++) {
          block_conj(j * level.block_size, level.block_size);
        }
      } else {
        for (int j = 1; j < level.num_blocks; j++) {
          block_disj(j * level.block_size, level.block_size);
        }
      }
    }
    return final_answer();
  }
};

#endif
