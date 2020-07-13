#include "template_desc.h"

#include <ostream>
#include <istream>
#include <cassert>

using namespace std;

ostream& operator<<(ostream& os, const TemplateSlice& td)
{
  os << "TemplateSlice[ ";
  os << td.k
    << " " << td.depth
    << " " << td.count\
    << " " << td.vars.size();
  assert (td.vars.size() == td.quantifiers.size());
  for (int i = 0; i < (int)td.vars.size(); i++) {
    os << " " << td.vars[i]
       << " " << (td.quantifiers[i] == Quantifier::Forall ? "forall" : "exists");
  }
  os << " ]";
  return os;
}

istream& operator>>(istream& is, TemplateSlice& td)
{
  string b;
  is >> b;
  assert(b == "TemplateSlice[");
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
    td.quantifiers.push_back(quant == "forall" ? Quantifier::Forall : Quantifier::Exists);
  }
  is >> b;
  assert(b == "]");
  return is;
}

std::string TemplateSlice::to_string(std::shared_ptr<Module> module) const {
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

ostream& operator<<(ostream& os, const TemplateSubSlice& tss)
{
  os << "TemplateSubSlice[ ";
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

istream& operator>>(istream& is, TemplateSubSlice& tss)
{
  string b;
  is >> b;
  assert (b == "TemplateSubSlice[");

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

value TemplateSpace::make_templ(shared_ptr<Module> module) const
{
  vector<VarDecl> decls;
  int num = 0;
  for (int i = 0; i < (int)vars.size(); i++) {
    lsort so = s_uninterp(module->sorts[i]);
    for (int j = 0; j < vars[i]; j++) {
      string name = ::to_string(num);
      num++;
      int nsize = 3;
      assert((int)name.size() <= nsize);
      while ((int)name.size() < nsize) {
        name = "0" + name;
      }
      name = "A" + name;
      decls.push_back(VarDecl(string_to_iden(name), so));
    }
  }
  return v_forall(decls, v_template_hole());
}

std::vector<TemplateSpace> spaces_containing_sub_slices(
    shared_ptr<Module> module,
    std::vector<TemplateSubSlice> const& slices)
{
  int nsorts = module->sorts.size();

  vector<TemplateSpace> tspaces;
  tspaces.resize(1 << nsorts);
  for (int i = 0; i < (1 << nsorts); i++) {
    tspaces[i].vars.resize(nsorts);
    tspaces[i].quantifiers.resize(nsorts);
    for (int j = 0; j < nsorts; j++) {
      tspaces[i].vars[j] = -1;
      tspaces[i].quantifiers[j] = (
          (i>>j)&1 ? Quantifier::Exists : Quantifier::Forall);
    }
    tspaces[i].depth = -1;
    tspaces[i].k = -1;
  }

  for (TemplateSubSlice const& tss : slices) {
    int bitmask = 0;
    for (int j = 0; j < nsorts; j++) {
      if (tss.ts.quantifiers[j] == Quantifier::Exists) {
        bitmask |= (1 << j);
      }
    }
    for (int j = 0; j < nsorts; j++) {
      tspaces[bitmask].vars[j] = max(
          tspaces[bitmask].vars[j], tss.ts.vars[j]);
    }
    tspaces[bitmask].depth = max(
        tspaces[bitmask].depth, tss.ts.depth);
    tspaces[bitmask].k = max(
        tspaces[bitmask].k, tss.ts.k);
  }

  vector<TemplateSpace> res;
  for (int i = 0; i < (int)tspaces.size(); i++) {
    if (tspaces[i].depth != -1) {
      res.push_back(tspaces[i]);
    }
  }
  return res;
}

std::vector<int> get_subslice_index_map(
    std::vector<value> const& clauses,
    TemplateSlice const& ts)
{
  assert(false);
}
