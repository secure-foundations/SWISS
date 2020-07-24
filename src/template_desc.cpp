#include "template_desc.h"

#include <ostream>
#include <istream>
#include <iostream>
#include <cassert>

#include "utils.h"

using namespace std;

ostream& operator<<(ostream& os, const TemplateSpace& ts)
{
  os << "TemplateSpace[ ";

  os << "k " << ts.k
    << " d " << ts.depth
    << " vars (";

  assert (ts.vars.size() == ts.quantifiers.size());
  for (int i = 0; i < (int)ts.vars.size(); i++) {
    os << " " << ts.vars[i]
       << " " << (ts.quantifiers[i] == Quantifier::Forall ? "forall" : "exists");
  }

  os << " ) ]";

  return os;
}

ostream& operator<<(ostream& os, const TemplateSlice& td)
{
  os << "TemplateSlice[ ";
  os << "k " << td.k
    << " d " << td.depth
    << " count " << td.count
    << " size " << td.vars.size()
    << " vars";
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

  is >> b;
  assert(b == "k");
  is >> td.k;

  is >> b;
  assert(b == "d");
  is >> td.depth;

  is >> b;
  assert(b == "count");
  is >> td.count;

  is >> b;
  assert(b == "size");
  int sz;
  is >> sz;

  is >> b;
  assert(b == "vars");

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
  vector<pair<Quantifier, VarDecl>> decls;
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
      decls.push_back(make_pair(quantifiers[i], VarDecl(string_to_iden(name), so)));
    }
  }

  value res = v_template_hole();
  int b = decls.size();
  while (b > 0) {
    int a = b-1;
    while (a > 0 && decls[a-1].first == decls[b-1].first) {
      a--;
    }
    Quantifier q = decls[b-1].first;
    vector<VarDecl> d;
    for (int i = a; i < b; i++) {
      d.push_back(decls[i].second);
    }
    res = (q == Quantifier::Forall
        ? v_forall(d, res)
        : v_exists(d, res));
    b = a;
  }

  return res;
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

int vec_sum(vector<int> const& v) {
  int sum = 0;
  for (int i : v) sum += i;
  return sum;
}
int total_vars(TemplateSlice const& ts) {
  return vec_sum(ts.vars);
}
int total_vars(TemplateSpace const& ts) {
  return vec_sum(ts.vars);
}

bool is_subspace_of_any(TemplateSlice const& slice, vector<TemplateSpace> const& spaces)
{
  for (TemplateSpace const& space : spaces) {
    if (is_subspace(slice, space)) {
      return true;
    }
  }
  return false;
}

TemplateSpace slice_to_space(TemplateSlice const& slice)
{
  TemplateSpace ts;
  ts.vars = slice.vars;
  ts.quantifiers = slice.quantifiers;
  ts.depth = slice.depth;
  ts.k = slice.k;
  return ts;
}

TemplateSpace merge_slice_space(TemplateSlice const& slice, TemplateSpace space)
{
  assert (slice.depth == space.depth);
  if (slice.k > space.k) space.k = slice.k;
  for (int i = 0; i < (int)slice.vars.size(); i++) {
    if (slice.vars[i] > space.vars[i]) space.vars[i] = slice.vars[i];
  }
  return space;
}

bool is_strictly_small(TemplateSlice const& ts, int thresh) {
  return total_vars(ts) < thresh;
}
bool is_small(TemplateSpace const& ts, int thresh) {
  return total_vars(ts) <= thresh;
}

long long vars_product(TemplateSpace const& space)
{
  long long prod = 1;
  for (int v : space.vars) {
    prod = prod * (long long)(v + 1);
  }
  return prod;
}

void sort_by_product(vector<TemplateSpace>& spaces)
{
  vector<pair<long long, int>> v;
  for (int i = 0; i < (int)spaces.size(); i++) {
    v.push_back(make_pair(vars_product(spaces[i]), i));
  }
  sort(v.begin(), v.end());
  vector<TemplateSpace> res;
  for (int i = (int)v.size() - 1; i >= 0; i--) {
    res.push_back(spaces[v[i].second]);
  }
  spaces = move(res);
}

int compute_thresh(std::vector<TemplateSlice> const& slices)
{
  int m = 0;
  for (TemplateSlice const& ts : slices) {
    m = max(total_vars(ts), m);
  }
  m = min(m, 6);
  return m;
}

bool is_subspace_ignoring_k(TemplateSlice const& slice, TemplateSpace const& space);

std::vector<TemplateSpace> finer_spaces_containing_slices_per_quant(
    std::vector<TemplateSlice> const& slices)
{
  if (slices.size() == 0) {
    return {};
  }

  int thresh = compute_thresh(slices);

  vector<TemplateSpace> res;
  vector<TemplateSlice> small_slices;

  for (TemplateSlice const& slice : slices) {
    bool did_merge = false;
    for (int i = 0; i < (int)res.size(); i++) {
      if (is_subspace_ignoring_k(slice, res[i])) {
        if (slice.k > res[i].k) {
          res[i].k = slice.k;
        }
        did_merge = true;
        break;
      }
    }

    if (!did_merge) {
      if (is_strictly_small(slice, thresh)) {
        small_slices.push_back(slice);
      } else {
        res.push_back(slice_to_space(slice));
      }
    }
  }

  for (TemplateSlice const& slice : small_slices) {
    if (!is_subspace_of_any(slice, res)) {
      bool did_merge = false;
      for (int i = 0; i < (int)res.size(); i++) {
        if (is_subspace_ignoring_k(slice, res[i])) {
          if (slice.k > res[i].k) {
            res[i].k = slice.k;
          }
          did_merge = true;
          break;
        }
      }
      if (!did_merge) {
        for (int i = 0; i < (int)res.size(); i++) {
          TemplateSpace merged = merge_slice_space(slice, res[i]);
          if (is_small(merged, thresh)) {
            res[i] = merged;
            did_merge = true;
            break;
          }
        }
      }
      if (!did_merge) {
        res.push_back(slice_to_space(slice));
      }
    } 
  }

  sort_by_product(res);

  return res;
}

std::vector<TemplateSpace> finer_spaces_containing_sub_slices(
    shared_ptr<Module> module,
    std::vector<TemplateSubSlice> const& slices)
{
  int nsorts = module->sorts.size();

  vector<vector<TemplateSlice>> tslices;
  tslices.resize(1 << (nsorts+1));

  for (TemplateSubSlice const& tss : slices) {
    int bitmask = 0;
    for (int j = 0; j < nsorts; j++) {
      if (tss.ts.quantifiers[j] == Quantifier::Exists) {
        bitmask |= (1 << j);
      }
    }
    if (tss.ts.depth == 2) {
      bitmask |= (1 << nsorts);
    }
    tslices[bitmask].push_back(tss.ts);
  }

  vector<TemplateSpace> res;

  for (int i = 0; i < (int)tslices.size(); i++) {
    vector_append(res, 
        finer_spaces_containing_slices_per_quant(tslices[i]));
  }

  return res;
}

TemplateSpace space_containing_slices_ignore_quants(
    std::shared_ptr<Module> module,
    std::vector<TemplateSlice> const& slices)
{
  int nsorts = module->sorts.size();

  TemplateSpace tspace;
  tspace.vars.resize(nsorts);
  tspace.quantifiers.resize(nsorts);
  for (int j = 0; j < nsorts; j++) {
    tspace.vars[j] = 0;
    tspace.quantifiers[j] = Quantifier::Forall;
  }
  tspace.depth = -1;
  tspace.k = -1;

  for (TemplateSlice const& ts : slices) {
    for (int j = 0; j < nsorts; j++) {
      tspace.vars[j] = max(
          tspace.vars[j], ts.vars[j]);
    }
    tspace.depth = max(
        tspace.depth, ts.depth);
    tspace.k = max(
        tspace.k, ts.k);
  }

  assert (tspace.depth != -1);
  return tspace;
}

bool is_subspace_ignoring_k(TemplateSlice const& slice, TemplateSpace const& space)
{
  if (slice.quantifiers != space.quantifiers) {
    return false;
  }
  if (slice.depth != space.depth) {
    return false;
  }
  assert (slice.vars.size() == space.vars.size());
  for (int i = 0; i < (int)slice.vars.size(); i++) {
    if (slice.vars[i] > space.vars[i]) {
      return false;
    }
  }
  return true;
}

bool is_subspace(TemplateSlice const& slice, TemplateSpace const& space)
{
  if (slice.k > space.k) {
    return false;
  }
  return is_subspace_ignoring_k(slice, space);
}

bool is_subspace(TemplateSubSlice const& tss, TemplateSpace const& ts)
{
  return is_subspace(tss.ts, ts);
}
