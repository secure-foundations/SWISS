#ifndef TEMPLATE_PRIORITY_H
#define TEMPLATE_PRIORITY_H

#include "template_desc.h"

#include <vector>

std::vector<TemplateSlice> break_into_slices(
  TemplateSpace const& ts);

std::vector<std::vector<TemplateSubSlice>> 
  prioritize_sub_slices(std::vector<TemplateSlice> const&, int nthreads);

#endif
