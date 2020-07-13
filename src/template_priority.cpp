#include "template_priority.h"

#include "template_counter.h"

using namespace std;

std::vector<TemplateSlice> break_into_slices(
  shared_ptr<Module> module,
  TemplateSpace const& ts)
{
  return count_many_templates(module, ts);
}

std::vector<std::vector<TemplateSubSlice>> 
  prioritize_sub_slices(std::vector<TemplateSlice> const&, int nthreads)
{
  assert(false);
}
