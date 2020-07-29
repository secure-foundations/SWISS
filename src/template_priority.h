#ifndef TEMPLATE_PRIORITY_H
#define TEMPLATE_PRIORITY_H

#include "template_desc.h"

#include <vector>

std::vector<TemplateSlice> break_into_slices(
  std::shared_ptr<Module> module,
  TemplateSpace const& ts);

std::vector<std::vector<TemplateSubSlice>> prioritize_sub_slices(
    std::shared_ptr<Module> module,
    std::vector<TemplateSlice> const&,
    int nthreads,
    bool is_for_breadth);

std::vector<TemplateSlice> quantifier_combos(
    std::shared_ptr<Module> module,
    std::vector<TemplateSlice> const& forall_slices);

#endif
