#ifndef TEMPLATE_DESC_H
#define TEMPLATE_DESC_H

#include <vector>

#include "logic.h"
#include "top_quantifier_desc.h"

enum class Quantifier {
  Forall,
  Exists
};

struct TemplateSpace {
  std::vector<int> vars; // up to
  std::vector<Quantifier> quantifiers;
  int depth;
  int k; // up to

  value make_templ(std::shared_ptr<Module> module) const;
};

struct TemplateSlice {
  std::vector<int> vars; // exact
  std::vector<Quantifier> quantifiers;
  int k; // exact
  int depth;
  long long count;
  inline bool operator<(TemplateSlice const& other) const {
    return count < other.count;
  }
  std::string to_string(std::shared_ptr<Module> module) const;
};

struct TemplateSubSlice {
  TemplateSlice ts;
  int tree_idx; // depth 2 only
  std::vector<int> prefix;

  TemplateSubSlice() : tree_idx(-1) { }
};

std::ostream& operator<<(std::ostream& os, const TemplateSpace& ts);
//std::istream& operator>>(std::istream& is, TemplateSlice& tss);

std::ostream& operator<<(std::ostream& os, const TemplateSlice& tss);
std::istream& operator>>(std::istream& is, TemplateSlice& tss);

std::ostream& operator<<(std::ostream& os, const TemplateSubSlice& tss);
std::istream& operator>>(std::istream& is, TemplateSubSlice& tss);

std::vector<int> get_subslice_index_map(
    TopAlternatingQuantifierDesc const& taqd,
    std::vector<value> const& clauses,
    TemplateSlice const& ts);

std::vector<TemplateSpace> spaces_containing_sub_slices(
    std::shared_ptr<Module> module,
    std::vector<TemplateSubSlice> const& slices);

std::vector<TemplateSpace> finer_spaces_containing_sub_slices(
    std::shared_ptr<Module> module,
    std::vector<TemplateSubSlice> const& slices);

TemplateSpace space_containing_slices_ignore_quants(
    std::shared_ptr<Module> module,
    std::vector<TemplateSlice> const& slices);

bool is_subspace(TemplateSlice const& slice, TemplateSpace const& ts);
bool is_subspace(TemplateSubSlice const& tss, TemplateSpace const& ts);

int total_vars(TemplateSlice const& ts);

#endif
