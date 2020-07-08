#include "template_desc.h"

#include <ostream>
#include <istream>
#include <cassert>

using namespace std;

ostream& operator<<(ostream& os, const TemplateSpace& td)
{
  os << "TemplateSpace[ ";
  os << td.k
    << " " << td.depth
    << " " << td.count\
    << " " << td.vars.size();
  assert (td.vars.size() == td.quantifiers.size());
  for (int i = 0; i < (int)td.vars.size(); i++) {
    os << " " << td.vars[i]
       << " " << (td.quantifiers[i] == TemplateSpace::Forall ? "forall" : "exists");
  }
  os << " ]";
  return os;
}

istream& operator>>(istream& is, TemplateSpace& td)
{
  string b;
  is >> b;
  assert(b == "TemplateSpace[");
  is >> td.k;
  is >> td.depth;
  is >> td.count;
  int sz;
  is >> sz;
  td.vars = {};
  for (int i = 0; i < sz; i++) {
    int x;
    is >> x;
    string quant;
    is >> quant;
    assert (quant == "forall" || quant == "exists");
    td.vars.push_back(x);
    td.quantifiers.push_back(quant == "forall" ? TemplateSpace::Forall : TemplateSpace::Exists);
  }
  is >> b;
  assert(b == "]");
  return is;
}

std::string TemplateSpace::to_string(std::shared_ptr<Module> module) const {
  std::string s = "((";
  for (int i = 0; i < (int)module->sorts.size(); i++) {
    if (i > 0) {
      s += ", ";
    }
    s += module->sorts[i];
    s += " ";
    s += ::to_string(vars[i]);
  }
  s += ") depth " + ::to_string(depth) + ", k " + ::to_string(k) + ") count " + ::to_string(count);
  return s;
}

ostream& operator<<(ostream& os, const TemplateSubSpace& tss)
{
  os << "TemplateSubSpace[ ";
  os << tss.ts;
  os << " ";

  if (tss.ts.depth == 2) {
    os << "tree " << tss.tree_idx << " ";
  }

  os << "prefix";
  for (int p : tss.prefix) {
    os << " " << p;
  }
  os << " ]";
  return os;
}

istream& operator>>(istream& is, TemplateSubSpace& tss)
{
  string b;
  is >> b;
  assert (b == "TemplateSubSpace[");

  is >> tss.ts;

  if (tss.ts.depth == 2) {
    is >> b;
    assert (b == "tree");
    is >> tss.tree_idx;
  } else {
    tss.tree_idx = -1;
  }

  is >> b;
  assert (b == "prefix");

  while (true) {
    is >> b;
    assert (b.size() > 0);
    if (b[0] == ']') {
      break;
    }
    tss.prefix.push_back(stoi(b));
    assert ((int)tss.prefix.size() <= tss.ts.k);
  }

  return is;
}
