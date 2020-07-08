#ifndef TEMPLATE_DESC_H
#define TEMPLATE_DESC_H

#include <vector>

#include "logic.h"

struct TemplateSpace {
  enum Quantifier {
    Forall,
    Exists
  };

  std::vector<int> vars;
  std::vector<Quantifier> quantifiers;
  int k;
  int depth;
  long long count;
  inline bool operator<(TemplateSpace const& other) const {
    return count < other.count;
  }
  std::string to_string(std::shared_ptr<Module> module) const;
};

struct TemplateSubSpace {
  TemplateSpace ts;
  int tree_idx; // depth 2 only
  std::vector<int> prefix;

  TemplateSubSpace() : tree_idx(-1) { }
};

std::ostream& operator<<(std::ostream& os, const TemplateSpace& tss);
std::istream& operator>>(std::istream& is, TemplateSpace& tss);

std::ostream& operator<<(std::ostream& os, const TemplateSubSpace& tss);
std::istream& operator>>(std::istream& is, TemplateSubSpace& tss);

#endif
