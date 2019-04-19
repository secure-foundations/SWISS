#include "model.h"

#include <iostream>

using namespace std;

shared_ptr<FTree> Model::getFunctionFTree(iden name, object_value res) {
  auto iter = ftree_cache.find(make_pair(name, res));
  if (iter == ftree_cache.end()) {
    shared_ptr<FTree> ft = constructFunctionFTree(name, res);
    ftree_cache.insert(make_pair(make_pair(name, res), ft));
    return ft;
  } else {
    return iter->second;
  }
}

shared_ptr<FTree> make_ftree_node(
    vector<FunctionEntry> const& entries,
    vector<size_t> const& domain_sizes);

shared_ptr<FTree> ftree_atom(int idx, int val) {
  shared_ptr<FTree> res(new FTree());
  res->type = FTree::Type::Atom;
  res->arg_idx = idx;
  res->arg_value = val;
  return res;
}

shared_ptr<FTree> ftree_and(shared_ptr<FTree> l, shared_ptr<FTree> r) {
  if (l->type == FTree::Type::True) return r;
  if (l->type == FTree::Type::False) return l;

  if (r->type == FTree::Type::True) return l;
  if (r->type == FTree::Type::False) return r;

  shared_ptr<FTree> res(new FTree());
  res->type = FTree::Type::And;
  res->left = l;
  res->right = r;
  return res;
}

shared_ptr<FTree> ftree_or(shared_ptr<FTree> l, shared_ptr<FTree> r) {
  if (l->type == FTree::Type::True) return l;
  if (l->type == FTree::Type::False) return r;

  if (r->type == FTree::Type::True) return r;
  if (r->type == FTree::Type::False) return l;

  shared_ptr<FTree> res(new FTree());
  res->type = FTree::Type::Or;
  res->left = l;
  res->right = r;
  return res;
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

template <typename T>
pair<vector<T>, vector<T>> split(int n, int mask, vector<T> const& v) {
  pair<vector<T>, vector<T>> p;
  for (int i = 0; i < n; i++) {
    if (mask & (1 << i)) {
      p.first.push_back(v[i]);
    } else {
      p.second.push_back(v[i]);
    }
  }
  return p;
}

shared_ptr<FTree> adjust_arg_indices(int n, int mask, shared_ptr<FTree> ft) {
  switch (ft->type) {
    case FTree::Type::True:
    case FTree::Type::False: {
      return ft;
    }

    case FTree::Type::And:
    case FTree::Type::Or: {
      shared_ptr<FTree> res(new FTree());
      res->type = ft->type;
      res->left = adjust_arg_indices(n, mask, ft->left);
      res->right = adjust_arg_indices(n, mask, ft->right);
      return res;
    }

    case FTree::Type::Atom: {
      int ct = 0;
      int new_arg_idx = -1;
      for (int i = 0; i < n; i++) {
        if (mask & (1 << i)) {
          if (ct == ft->arg_idx) {
            new_arg_idx = i;
            break;
          }
          ct++;
        }
      }
      assert(new_arg_idx != -1);
      shared_ptr<FTree> res(new FTree());
      res->type = ft->type;
      res->arg_idx = new_arg_idx;
      res->arg_value = ft->arg_value;
      return res;
    }

    default:
      assert(false);
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
  for (int mask = (1 << (n-1)); mask < (1 << n) - 1; mask++) {
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
      set<vector<object_value>> fes1_set;
      set<vector<object_value>> fes2_set;

      vector<FunctionEntry> fes1;
      vector<FunctionEntry> fes2;

      for (FunctionEntry const& fe : entries) {
        auto p = split(n, mask, fe.args);

        FunctionEntry fe1;
        fe1.args = p.first;
        fe1.res = (vector_set_1.count(p.first) == 0 ? 0 : 1);

        FunctionEntry fe2;
        fe2.args = p.second;
        fe2.res = (vector_set_2.count(p.second) == 0 ? 0 : 1);

        if (fes1_set.count(fe1.args) == 0) {
          fes1_set.insert(fe1.args);
          fes1.push_back(fe1);
        }

        if (fes2_set.count(fe2.args) == 0) {
          fes2_set.insert(fe2.args);
          fes2.push_back(fe2);
        }
      } 

      int mask_comp = (1 << n) - 1 - mask;
      auto domain_sizes_split = split(n, mask, domain_sizes);

      return ftree_and(
        adjust_arg_indices(n, mask,
            make_ftree_node(fes1, domain_sizes_split.first)),
        adjust_arg_indices(n, mask_comp,
            make_ftree_node(fes2, domain_sizes_split.second))
      );
    }
  }
  return nullptr;
}

float entropy(int t_i, int f_i) {
  float t = t_i / (float)(t_i + f_i);
  float f = f_i / (float)(t_i + f_i);
  float t1 = (t_i == 0 ? 0 : t * log(t) / log(2));
  float f1 = (f_i == 0 ? 0 : f * log(f) / log(2));
  return -(t1 + f1);
}

float calc_score(int ltrue, int lfalse, int rtrue, int rfalse) {
  // ¯\_(ツ)_/¯
  return (float)(
       (ltrue + lfalse) * entropy(ltrue, lfalse) +
       (rtrue + rfalse) * entropy(rtrue, lfalse + rfalse)) /
     (float)(ltrue + lfalse + rtrue + rfalse);
}

shared_ptr<FTree> do_branch(
    vector<FunctionEntry> const& entries,
    vector<size_t> const& domain_sizes)
{
  bool found = false;
  float best_score;
  int best_idx;
  object_value best_val;

  for (int idx = 0; idx < domain_sizes.size(); idx++) {
    for (object_value val = 0; val < domain_sizes[idx]; val++) {
      int ltrue = 0, lfalse = 0, rtrue = 0, rfalse = 0;
      for (FunctionEntry const& fe : entries) {
        if (fe.res == 1) {
          if (fe.args[idx] == val) {
            ltrue++;
          } else {
            rtrue++;
          }
        } else {
          if (fe.args[idx] == val) {
            lfalse++;
          } else {
            rfalse++;
          }
        }
      }

      if (ltrue == 0) {
        continue;
      }
      if ((ltrue + lfalse == 0) || (rtrue + rfalse == 0)) {
        continue;
      }

      float score = calc_score(ltrue, lfalse, rtrue, rfalse);
      if (!found || score < best_score) {
        found = true;
        best_score = score;
        best_idx = idx;
        best_val = val;
      }
    }
  }

  assert(found);

  int idx = best_idx;
  object_value val = best_val;

  vector<FunctionEntry> left;
  vector<FunctionEntry> right;
  for (FunctionEntry const& fe : entries) {
    if (fe.args[idx] == val) {
      left.push_back(fe);
    }

    if (fe.args[idx] != val || fe.res == 0) {
      right.push_back(fe);
    }
  }

  auto left_tree = make_ftree_node(left, domain_sizes);
  auto right_tree = make_ftree_node(right , domain_sizes);

  return ftree_or(
    ftree_and(
      ftree_atom(idx, val),
      left_tree
    ),
    right_tree
  );
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

  auto ft = do_branch(entries, domain_sizes);
  return ft;
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
  else {
    assert(false);
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

shared_ptr<FTree> Model::constructFunctionFTree(iden name, object_value res) {
  vector<FunctionEntry> entries_eq_res;

  for (FunctionEntry const& fe : getFunctionEntries(name)) {
    FunctionEntry fe2;
    fe2.args = fe.args;
    fe2.res = (fe.res == res ? 1 : 0);
    entries_eq_res.push_back(fe2);
  }

  vector<size_t> domain_sizes = get_domain_sizes_for_function(name);

  shared_ptr<FTree> ft = make_ftree_node(entries_eq_res, domain_sizes);
  assert(is_correct(ft, entries_eq_res, domain_sizes));
  return ft;
}

string FTree::to_string() const {
  switch (type) {
    case FTree::Type::True: {
      return "true";
    }
    case FTree::Type::False: {
      return "false";
    }

    case FTree::Type::And:
    case FTree::Type::Or: {
      return (type == FTree::Type::And ? "AND" : "OR") + string("(") +
          left->to_string() + ", " + right->to_string() + ")";
    }

    case FTree::Type::Atom: {
      return "args " + ::to_string(arg_idx) + " == " + ::to_string(arg_value);
    }

    default:
      assert(false);
  }
}

void ftree_tests() {
  cout << make_ftree_node({
      FunctionEntry({0,0,0}, 1)
    },
    { 3, 3, 3 }
  )->to_string() << endl;

  cout << make_ftree_node({
      FunctionEntry({0,0,0}, 1),
      FunctionEntry({1,0,0}, 0),
      FunctionEntry({0,1,0}, 0),
      FunctionEntry({0,0,1}, 0),
      FunctionEntry({0,1,1}, 0),
      FunctionEntry({1,0,1}, 0),
      FunctionEntry({1,1,0}, 0),
      FunctionEntry({1,1,1}, 0),
    },
    { 2, 2, 2 }
  )->to_string() << endl;

  cout << make_ftree_node({
      FunctionEntry({0,0,0}, 1),
      FunctionEntry({1,0,0}, 1),
      FunctionEntry({0,1,0}, 1),
      FunctionEntry({0,0,1}, 0),
      FunctionEntry({0,1,1}, 0),
      FunctionEntry({1,0,1}, 0),
      FunctionEntry({1,1,0}, 1),
      FunctionEntry({1,1,1}, 0),
    },
    { 2, 2, 2 }
  )->to_string() << endl;


  cout << make_ftree_node({
      FunctionEntry({0,0,0}, 0),
      FunctionEntry({1,0,0}, 0),
      FunctionEntry({0,1,0}, 0),
      FunctionEntry({0,0,1}, 1),
      FunctionEntry({0,1,1}, 1),
      FunctionEntry({1,0,1}, 1),
      FunctionEntry({1,1,0}, 0),
      FunctionEntry({1,1,1}, 1),
    },
    { 2, 2, 2 }
  )->to_string() << endl;


  cout << make_ftree_node({
      FunctionEntry({0,0,0}, 0),
      FunctionEntry({1,0,0}, 1),
      FunctionEntry({0,1,0}, 1),
      FunctionEntry({0,0,1}, 1),
      FunctionEntry({0,1,1}, 1),
      FunctionEntry({1,0,1}, 1),
      FunctionEntry({1,1,0}, 1),
      FunctionEntry({1,1,1}, 1),
    },
    { 2, 2, 2 }
  )->to_string() << endl;


  cout << make_ftree_node({
      FunctionEntry({0,0,0}, 0),
      FunctionEntry({0,0,1}, 0),
      FunctionEntry({0,0,2}, 0),
      FunctionEntry({0,1,0}, 0),
      FunctionEntry({0,1,1}, 0),
      FunctionEntry({0,1,2}, 1),
      FunctionEntry({0,2,0}, 0),
      FunctionEntry({0,2,1}, 0),
      FunctionEntry({0,2,2}, 0),
      FunctionEntry({1,0,0}, 0),
      FunctionEntry({1,0,1}, 0),
      FunctionEntry({1,0,2}, 0),
      FunctionEntry({1,1,0}, 0),
      FunctionEntry({1,1,1}, 0),
      FunctionEntry({1,1,2}, 0),
      FunctionEntry({1,2,0}, 1),
      FunctionEntry({1,2,1}, 0),
      FunctionEntry({1,2,2}, 0),
      FunctionEntry({2,0,0}, 0),
      FunctionEntry({2,0,1}, 1),
      FunctionEntry({2,0,2}, 0),
      FunctionEntry({2,1,0}, 0),
      FunctionEntry({2,1,1}, 0),
      FunctionEntry({2,1,2}, 0),
      FunctionEntry({2,2,0}, 0),
      FunctionEntry({2,2,1}, 0),
      FunctionEntry({2,2,2}, 0),
    },
    { 3, 3, 3 }
  )->to_string() << endl;


  cout << make_ftree_node({
      FunctionEntry({0,0,0}, 1),
      FunctionEntry({0,0,1}, 1),
      FunctionEntry({0,0,2}, 1),
      FunctionEntry({0,1,0}, 1),
      FunctionEntry({0,1,1}, 1),
      FunctionEntry({0,1,2}, 0),
      FunctionEntry({0,2,0}, 1),
      FunctionEntry({0,2,1}, 1),
      FunctionEntry({0,2,2}, 1),
      FunctionEntry({1,0,0}, 1),
      FunctionEntry({1,0,1}, 1),
      FunctionEntry({1,0,2}, 1),
      FunctionEntry({1,1,0}, 1),
      FunctionEntry({1,1,1}, 1),
      FunctionEntry({1,1,2}, 1),
      FunctionEntry({1,2,0}, 0),
      FunctionEntry({1,2,1}, 1),
      FunctionEntry({1,2,2}, 1),
      FunctionEntry({2,0,0}, 1),
      FunctionEntry({2,0,1}, 0),
      FunctionEntry({2,0,2}, 1),
      FunctionEntry({2,1,0}, 1),
      FunctionEntry({2,1,1}, 1),
      FunctionEntry({2,1,2}, 1),
      FunctionEntry({2,2,0}, 1),
      FunctionEntry({2,2,1}, 1),
      FunctionEntry({2,2,2}, 1),
    },
    { 3, 3, 3 }
  )->to_string() << endl;


  cout << make_ftree_node({
      FunctionEntry({0,0,0}, 1),
      FunctionEntry({0,0,1}, 1),
      FunctionEntry({0,0,2}, 1),
      FunctionEntry({0,1,0}, 1),
      FunctionEntry({0,1,1}, 1),
      FunctionEntry({0,1,2}, 1),
      FunctionEntry({0,2,0}, 1),
      FunctionEntry({0,2,1}, 1),
      FunctionEntry({0,2,2}, 1),
      FunctionEntry({1,0,0}, 1),
      FunctionEntry({1,0,1}, 1),
      FunctionEntry({1,0,2}, 1),
      FunctionEntry({1,1,0}, 1),
      FunctionEntry({1,1,1}, 1),
      FunctionEntry({1,1,2}, 1),
      FunctionEntry({1,2,0}, 1),
      FunctionEntry({1,2,1}, 1),
      FunctionEntry({1,2,2}, 1),
      FunctionEntry({2,0,0}, 1),
      FunctionEntry({2,0,1}, 1),
      FunctionEntry({2,0,2}, 1),
      FunctionEntry({2,1,0}, 1),
      FunctionEntry({2,1,1}, 1),
      FunctionEntry({2,1,2}, 1),
      FunctionEntry({2,2,0}, 1),
      FunctionEntry({2,2,1}, 1),
      FunctionEntry({2,2,2}, 0),
    },
    { 3, 3, 3 }
  )->to_string() << endl;


  cout << make_ftree_node({
      FunctionEntry({0,0,0}, 1),
      FunctionEntry({0,0,1}, 1),
      FunctionEntry({0,0,2}, 0),
      FunctionEntry({0,1,0}, 1),
      FunctionEntry({0,1,1}, 1),
      FunctionEntry({0,1,2}, 0),
      FunctionEntry({0,2,0}, 0),
      FunctionEntry({0,2,1}, 0),
      FunctionEntry({0,2,2}, 0),
      FunctionEntry({1,0,0}, 1),
      FunctionEntry({1,0,1}, 1),
      FunctionEntry({1,0,2}, 0),
      FunctionEntry({1,1,0}, 1),
      FunctionEntry({1,1,1}, 1),
      FunctionEntry({1,1,2}, 0),
      FunctionEntry({1,2,0}, 0),
      FunctionEntry({1,2,1}, 0),
      FunctionEntry({1,2,2}, 0),
      FunctionEntry({2,0,0}, 0),
      FunctionEntry({2,0,1}, 0),
      FunctionEntry({2,0,2}, 0),
      FunctionEntry({2,1,0}, 0),
      FunctionEntry({2,1,1}, 0),
      FunctionEntry({2,1,2}, 0),
      FunctionEntry({2,2,0}, 0),
      FunctionEntry({2,2,1}, 0),
      FunctionEntry({2,2,2}, 0),
    },
    { 3, 3, 3 }
  )->to_string() << endl;

}
