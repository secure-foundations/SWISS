#include "model.h"

using namespace std;

shared_ptr<FTree> Model::getFunctionFTree(iden name, object_value res) {
  auto iter = ftree_cache.find(make_pair(name, res));
  if (iter == ftree_cache.end()) {
    shared_ptr<FTree> ft = constructFunctionFTree(name);
    ftree_cache.insert(make_pair(make_pair(name, res), ft));
    return ft;
  } else {
    return iter->second;
  }
}

shared_ptr<FTree> try_constant(vector<FunctionEntry> const& entries) {
  assert(entries.size() > 0);

  bool is_const = true;
  for (auto& fe : entries) {
    if (fe.res != entries[0].res) {
      is_const = false;
      break;
    }
  }

  if (is_const) {
    shared_ptr<FTree> ftree(new FTree());
    ftree->type = entries[0].res == 1 ? FTree::Type::True : FTree::Type::False;
    return ftree;
  } else {
    return nullptr;
  }
}

shared_ptr<FTree> try_cross_product(
    vector<FunctionEntry> const& entries,
    vector<size_t> const& domain_sizes)
{
  int n = domain_sizes.size();
  if (n <= 1) {
    return nullptr;
  }
  for (int mask = (1 << (n-1)); mask < (1 << n); mask++) {
    set<vector<object_value>> vector_set_1;
    set<vector<object_value>> vector_set_2;
    int count_trues = 0;
    for (FunctionEntry const& fe : entries) {
      if (fe.res == 1) {
        auto p = split(n, mask, fe.args);
        vector_set_1.insert(p.first);
        vector_set_2.insert(p.second);
        count_trues++;
      }
    }
    if ((long long)count_trues ==
        (long long)vector_set_1.size() * (long long)vector_set_2.size())
    {
      set<FunctionEntry> fes1;
      set<FunctionEntry> fes2;
      for (FunctionEntry const& fe : entries) {
        auto p = split(n, mask, fe.args);

        FunctionEntry fe1;
        fe1.args = p.first;
        fe1.res = fe.res;

        FunctionEntry fe2;
        fe2.args = p.second;
        fe2.res = fe.res;

        fes1.insert(fe1);
        fes2.insert(fe2);
      } 

      int mask_comp = (1 << n) - 1 - mask;
      auto domain_sizes_split = split(n, mask, domain_sizes);
      return ftree_and(
        adjust_arg_indices(n, mask,
            make_ftree_node(set_to_vec(fes1), domain_sizes_split.first)),
        adjust_arg_indices(n, mask_comp,
            make_ftree_node(set_to_vec(fes2), domain_sizes_split.second))
      );
    }
  }
  return nullptr;
}

void do_branch(
    vector<FunctionEntry> const& entries,
    vector<size_t> const& domain_sizes)
{
  
}

shared_ptr<FTree> make_ftree_node(
    vector<FunctionEntry> const& entries,
    vector<size_t> const& domain_sizes)
{
  if (auto ft = try_constant(entries)) {
    return ft;
  }

  if (auto ft = try_cross_product(entries, domain_sizes)) {
    return ft; 
  }

  return do_branch(entries, domain_sizes);
}

bool ftree_eval(
    shared_ptr<FTree> ft,
    vector<object_value> const& args,
    vector<size_t> domain_sizes)
{
  assert(ft != nullptr);
  if (ft->type == FTree::Type::True) return true;
  else if (ft->type == FTree::Type::False) return false;
  else if (ft->type == FTree::Type::And) {
    return ftree_eval(ft->left, args, domain_sizes) &&
           ftree_eval(ft->right, args, domain_sizes);
  }
  else if (ft->type == FTree::Type::Or) {
    return ftree_eval(ft->left, args, domain_sizes) ||
           ftree_eval(ft->right, args, domain_sizes);
  }
  else if (ft->type == FTree::Type::Atom) {
    int arg_idx = ft->arg_idx;
    object_value arg_value = ft->arg_value;

    assert(0 <= arg_idx && arg_idx < domain_sizes.size());
    assert(arg_value < domain_sizes[arg_idx]);
    return args[arg_idx] == arg_value;
  }
}

bool is_correct(
    shared_ptr<FTree> ft,
    vector<FunctionEntry> const& entries,
    vector<size_t> const& domain_sizes)
{
  for (FunctionEntry const& fe : entries) {
    if (!(
      ((int)ftree_eval(ft, fe.args, domain_sizes)) == fe.res
    )) {
      return false;
    }
  }
  return true;
}

shared_ptr<FTree> constructFunctionFTree(iden name, object_value res) {
  vector<FunctionEntry> entries_eq_res;

  for (FunctionEntry const& fe : getFunctionEntries(name)) {
    FunctionEntry fe2;
    fe2.args = fe.args;
    fe2.res = (fe.res == res ? 1 : 0);
    entries_eq_res.push_back(fe2);
  }

  vector<size_t> domain_sizes = get_domain_sizes();

  shared_ptr<FTree> ft = make_ftree_node(entries_eq_res, domain_sizes);
  assert(is_correct(ft, entries_eq_res, domain_sizes));
  return ft;
}
