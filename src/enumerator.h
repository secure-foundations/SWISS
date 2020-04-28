#ifndef ENUMERATOR_H
#define ENUMERATOR_H

//#include "expr_gen_smt.h"
#include "logic.h"

//void add_constraints(std::shared_ptr<Module> module, SMT& solver);

std::pair<std::vector<value>, std::vector<value>> enumerate_for_template(
    std::shared_ptr<Module> module,
    value templ, int k = 3);

//std::vector<value> enumerate_fills_for_template(
//    std::shared_ptr<Module> module,
//    value templ);

std::vector<value> remove_equiv2(std::vector<value> const& values);

value fill_holes_in_value(value templ, std::vector<value> const& fills);

struct ValueList {
  std::vector<value> values_unsimplified;
  std::vector<value> values;
  std::vector<std::vector<int>> implications;
  std::map<ComparableValue, int> normalized_to_idx;

  void init_simp();
  void init_extra();
};

std::shared_ptr<ValueList> cached_get_filtered_values(
    std::shared_ptr<Module> module, value templ, int k = 3);

std::shared_ptr<ValueList> cached_get_unfiltered_values(
    std::shared_ptr<Module> module, value templ, int k = 3);

#endif
