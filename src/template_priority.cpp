#include "template_priority.h"

#include <queue>
#include <iostream>
#include <cassert>
#include <algorithm>

#include "template_counter.h"
#include "tree_shapes.h"
#include "utils.h"

using namespace std;

std::vector<TemplateSlice> break_into_slices(
  shared_ptr<Module> module,
  TemplateSpace const& ts)
{
  return count_many_templates(module, ts);
}

vector<TemplateSlice> quantifier_combos(
    shared_ptr<Module> module,
    vector<TemplateSlice> const& forall_slices,
    int maxExists)
{
  vector<TemplateSlice> res;
  int nsorts = module->sorts.size();
  for (TemplateSlice const& ts : forall_slices) {
    for (int i = 0; i < (1 << nsorts); i++) {
      TemplateSlice ts1 = ts;
      bool okay = true;
      for (int j = 0; j < nsorts; j++) {
        if (ts.vars[j] == 0 && ((i>>j)&1)) {
          okay = false;
          break;
        } else {
          ts1.quantifiers[j] = ((i>>j)&1) ? Quantifier::Exists : Quantifier::Forall;
        }
      }
      if (okay) {
        if (maxExists != -1) {
          int sumExists = 0;
          for (int j = 0; j < nsorts; j++) {
            if (ts1.quantifiers[j] == Quantifier::Exists) {
              sumExists += ts1.vars[j];
            }
          }
          if (sumExists > maxExists) {
            okay = false;
          }
        }
        if (okay) {
          res.push_back(ts1);
        }
      }
    }
  }
  return res;
}

struct Node {
  TemplateSlice ts;
  vector<Node*> succs;
  int pred_count;
};

struct NodePtr {
  Node* node;
  NodePtr(Node* n) : node(n) { }
  bool operator<(NodePtr const& other) const {
    return node->ts.count > other.node->ts.count;
  }
};

vector<TemplateSlice> get_preds(TemplateSlice const& ts)
{
  vector<TemplateSlice> res;
  assert (ts.vars.size() == ts.quantifiers.size());
  for (int i = 0; i < (int)ts.vars.size(); i++) {
    assert (!(ts.vars[i] == 0 && ts.quantifiers[i] == Quantifier::Exists));
    if (ts.vars[i] > 0) {
      TemplateSlice ts1 = ts;
      ts1.vars[i]--;
      if (ts1.vars[i] == 0) ts1.quantifiers[i] = Quantifier::Forall;
      ts1.count = -1;
      res.push_back(ts1);
    }
    if (ts.quantifiers[i] == Quantifier::Exists) {
      TemplateSlice ts1 = ts;
      ts1.quantifiers[i] = Quantifier::Forall;
      ts1.count = -1;
      res.push_back(ts1);
    }
  }
  if (ts.k > 1) {
    TemplateSlice ts1 = ts;
    ts1.k--;
    ts1.count = -1;
    res.push_back(ts1);
  }
  return res;
}

int slices_get_idx(vector<TemplateSlice> const& slices, TemplateSlice const& ts)
{
  for (int i = 0; i < (int)slices.size(); i++) {
    if (slices[i].vars == ts.vars
        && slices[i].quantifiers == ts.quantifiers
        && slices[i].k == ts.k
        && slices[i].depth == ts.depth)
    {
      return i;
    }
  }
  return -1;
}

void get_prefixes_rec(
  vector<vector<int>>& res,
  TreeShape const& ts,
  vector<int>& indices, int i, int vis,
  TransitionSystem const& trans_system)
{
  if (i == (int)indices.size()) {
    res.push_back(indices);
    return;
  }
  SymmEdge const& symm_edge = ts.symmetry_back_edges[i];
  int t = symm_edge.idx == -1 ? 0 : indices[symm_edge.idx] + symm_edge.inc;
  for (int j = t; j < trans_system.nTransitions(); j++) {
    if (trans_system.next(vis, j) != -1) {
      int next = trans_system.next(vis, j);
      indices[i] = j;
      get_prefixes_rec(res, ts, indices, i+1, next, trans_system);
    }
  }
}

vector<vector<int>> get_prefixes(
    TemplateSlice const& slice,
    TreeShape const& tree_shape,
    TransitionSystem const& sub_trans_system)
{
  int MAX_SZ = 1;

  vector<vector<int>> res;
  int sz = slice.k - 2;
  if (sz < 0) sz = 0;
  if (sz > MAX_SZ) sz = MAX_SZ;

  vector<int> indices;
  indices.resize(sz);

  get_prefixes_rec(
      res,
      tree_shape,
      indices,
      0,
      0,
      sub_trans_system);

  return res;
}

vector<vector<int>> get_prefixes(
    TemplateSlice const& slice,
    TransitionSystem const& sub_trans_system)
{
  vector<int> parts;
  parts.resize(slice.k);
  for (int i = 0; i < slice.k; i++) {
    parts[i] = 1;
  }
  TreeShape tree_shape = tree_shape_for(true, parts);
  return get_prefixes(slice, tree_shape, sub_trans_system);
}

TransitionSystem transition_system_for_slice_list(
    shared_ptr<Module> module,
    vector<TemplateSlice> const& slices,
    int maxVars)
{
  TemplateSpace tspace = space_containing_slices_ignore_quants(module, slices);
  value templ = tspace.make_templ(module);
  EnumInfo ei(module, templ);
  return build_transition_system(
      get_var_index_init_state(module, templ),
      ei.var_index_transitions, maxVars);
}


vector<TemplateSubSlice> split_slice_into_sub_slices(
    TransitionSystem const& trans_system,
    vector<TreeShape> const& tree_shapes,
    TemplateSlice const& slice,
    map<vector<int>, TransitionSystem>& sub_ts_cache)
{
  auto iter = sub_ts_cache.find(slice.vars);
  TransitionSystem sub_trans_system;
  if (iter == sub_ts_cache.end()) {
    auto p = get_subslice_index_map(trans_system, slice);
    sub_trans_system = p.first.second;
    sub_ts_cache.insert(make_pair(slice.vars, sub_trans_system));
  } else {
    sub_trans_system = iter->second;
  }

  vector<TemplateSubSlice> sub_slices;
  if (slice.count != 0) {
    if (slice.depth == 1) {
      TemplateSubSlice tss;
      tss.ts = slice;
      for (vector<int> pref : get_prefixes(slice, sub_trans_system)) {
        tss.prefix = pref;
        sub_slices.push_back(tss);
      }
    } else {
      for (int j = 0; j < (int)tree_shapes.size(); j++) {
        if (tree_shapes[j].total == slice.k) {
          TemplateSubSlice tss;
          tss.ts = slice;
          tss.tree_idx = j;
          for (vector<int> pref : get_prefixes(slice, tree_shapes[j], sub_trans_system)) {
            tss.prefix = pref;
            sub_slices.push_back(tss);
          }
        }
      }
    }
  }
  return sub_slices;
}

template <typename T>
void random_sort(vector<T>& v, int a, int b)
{
  for (int i = a; i < b-1; i++) {
    int j = i + (rand() % (b - i));
    T tmp = v[i];
    v[i] = v[j];
    v[j] = tmp;
  }
}

vector<TemplateSlice> remove_count0(vector<TemplateSlice> const& v)
{
  vector<TemplateSlice> res;
  for (TemplateSlice const& slice : v) {
    if (slice.count != 0) {
      res.push_back(slice);
    }
  }
  return res;
}

long long total_count(vector<TemplateSlice> const& slices)
{
  long long sum = 0;
  for (TemplateSlice const& ts : slices) {
    sum += ts.count;
  }
  return sum;
}

bool strictly_dominates(vector<int> const& a, vector<int> const& b) {
  for (int i = 0; i < (int)a.size(); i++) {
    if (a[i] < b[i]) {
      return false;
    }
  }
  return a != b;
}


bool unstrictly_dominates(vector<int> const& a, vector<int> const& b) {
  for (int i = 0; i < (int)a.size(); i++) {
    if (a[i] < b[i]) {
      return false;
    }
  }
  return true;
}

vector<vector<int>> get_pareto_vars(vector<TemplateSlice> const& ts)
{
  vector<vector<int>> v;
  for (int i = 0; i < (int)ts.size(); i++) {
    bool is_p = true;
    for (int j = 0; j < (int)ts.size(); j++) {
      if (i != j) {
        if (strictly_dominates(ts[j].vars, ts[i].vars)) {
          is_p = false;
          break;
        }
      }
    }
    if (is_p) {
      bool is_taken = false;
      for (int j = 0; j < (int)v.size(); j++) {
        if (v[j] == ts[i].vars) {
          is_taken = true;
          break;
        }
      }
      if (!is_taken) {
        v.push_back(ts[i].vars);
      }
    }
  }
  return v;
}

int get_vars_that_slice_fits_in(
    vector<vector<int>> const& m,
    TemplateSlice const& ts)
{
  for (int i = 0; i < (int)m.size(); i++) {
    if (unstrictly_dominates(m[i], ts.vars)) {
      return i;
    }
  }
  assert (false);
}

void sort_decreasing_count_order(vector<vector<TemplateSlice>>& s)
{
  vector<pair<long long, int>> cs;
  cs.resize(s.size());
  for (int i = 0; i < (int)s.size(); i++) {
    cs[i].first = -total_count(s[i]);
    cs[i].second = i;
  }

  sort(cs.begin(), cs.end());

  vector<vector<TemplateSlice>> t = move(s);
  s.clear();
  for (int i = 0; i < (int)t.size(); i++) {
    s.push_back(t[cs[i].second]);
  }
}

int get_max_k(vector<TemplateSlice> const& slices)
{
  int max_k = 0;
  for (int i = 0; i < (int)slices.size(); i++) {
    if (slices[i].k > max_k) {
      max_k = slices[i].k;
    }
  }
  return max_k;
}

vector<vector<TemplateSlice>> split_by_size(vector<TemplateSlice> const& slices)
{
  vector<vector<TemplateSlice>> res;
  int max_k = get_max_k(slices);
  res.resize(max_k);
  for (int i = 0; i < (int)slices.size(); i++) {
    int idx = slices[i].k - 1;
    assert (0 <= idx && idx < (int)res.size());
    res[idx].push_back(slices[i]);
  }
  for (int i = 0; i < (int)res.size(); i++) {
    assert (res[i].size() > 0);
  }
  return res;
}

vector<vector<TemplateSubSlice>> split_into(
  vector<TemplateSlice> const& slices,
  int n,
  TransitionSystem const& trans_system,
  map<vector<int>, TransitionSystem>& sub_ts_cache,
  vector<TreeShape> const& tree_shapes)
{
  vector<vector<TemplateSubSlice>> res;
  res.resize(n);
  assert (n > 0);
  for (int i = 0; i < (int)slices.size(); i++) {
    vector<TemplateSubSlice> new_slices =
      split_slice_into_sub_slices(trans_system, tree_shapes, slices[i], sub_ts_cache);
    random_sort(new_slices, 0, new_slices.size());
    int k = rand() % n;
    for (int j = 0; j < (int)new_slices.size(); j++) {
      res[k].push_back(new_slices[j]);
      k++;
      if (k == (int)res.size()) { 
        k = 0;
      }
    }
  }

  vector<vector<TemplateSubSlice>> res2;
  for (int i = 0; i < n; i++) {
    if (res[i].size() > 0) {
      res2.push_back(move(res[i]));
    }
  }
  return res2;
}

std::vector<std::vector<TemplateSubSlice>> pack_tightly_dont_exceed_hull(
    shared_ptr<Module> module,
    vector<TemplateSlice> const& slices,
    int nthreads,
    TransitionSystem const& trans_system,
    vector<TreeShape> const& tree_shapes,
    map<vector<int>, TransitionSystem>& sub_ts_cache)
{
  vector<vector<int>> m = get_pareto_vars(slices);
  vector<vector<TemplateSlice>> s2;
  s2.resize(m.size());
  for (TemplateSlice const& ts : slices) {
    int idx = get_vars_that_slice_fits_in(m, ts);
    s2[idx].push_back(ts);
  }
  sort_decreasing_count_order(s2);
  vector<vector<TemplateSubSlice>> res;

  for (int i = 0; i < (int)s2.size(); i++) {
    long long c = total_count(s2[i]);
    cout << i << " " << c << endl;

    //long long my_nthreads = (c + 1000 - 1) / 1000;
    //if (my_nthreads > nthreads) my_nthreads = nthreads;
    //assert (my_nthreads > 0);
    long long my_nthreads = (i == 0 ? 2 : 1);

    vector_append(res, split_into(s2[i], my_nthreads, trans_system, sub_ts_cache, tree_shapes));
  }
  return res;
}

std::vector<std::vector<TemplateSubSlice>> prioritize_sub_slices_basic(
    shared_ptr<Module> module,
    vector<TemplateSlice> const& slices,
    int nthreads,
    TransitionSystem const& trans_system,
    vector<TreeShape> const& tree_shapes)
{
  map<vector<int>, TransitionSystem> sub_ts_cache;
  return split_into(slices, nthreads, trans_system, sub_ts_cache, tree_shapes);
}

vector<vector<vector<TemplateSubSlice>>> prioritize_sub_slices_basic_by_size(
    shared_ptr<Module> module,
    vector<TemplateSlice> const& slices,
    int nthreads,
    TransitionSystem const& trans_system,
    vector<TreeShape> const& tree_shapes)
{
  map<vector<int>, TransitionSystem> sub_ts_cache;
  vector<vector<TemplateSlice>> splits = split_by_size(slices);
  vector<vector<vector<TemplateSubSlice>>> res;
  for (int i = 0; i < (int)splits.size(); i++) {
    res.push_back(split_into(splits[i], nthreads, trans_system, sub_ts_cache, tree_shapes));
  }
  return res;
}

std::vector<std::vector<TemplateSubSlice>> prioritize_sub_slices_breadth(
    shared_ptr<Module> module,
    vector<TemplateSlice> const& slices,
    int nthreads,
    TransitionSystem const& trans_system,
    vector<TreeShape> const& tree_shapes)
{
  map<vector<int>, TransitionSystem> sub_ts_cache;
  return pack_tightly_dont_exceed_hull(module, slices, nthreads, trans_system, tree_shapes, sub_ts_cache);
}

vector<vector<vector<TemplateSubSlice>>> prioritize_sub_slices_breadth_by_size(
    shared_ptr<Module> module,
    vector<TemplateSlice> const& slices,
    int nthreads,
    TransitionSystem const& trans_system,
    vector<TreeShape> const& tree_shapes)
{
  map<vector<int>, TransitionSystem> sub_ts_cache;
  vector<vector<TemplateSlice>> splits = split_by_size(slices);
  vector<vector<vector<TemplateSubSlice>>> res;
  for (int i = 0; i < (int)splits.size(); i++) {
    res.push_back(pack_tightly_dont_exceed_hull(module, splits[i], nthreads, trans_system, tree_shapes, sub_ts_cache));
  }
  return res;
}

/*std::vector<std::vector<TemplateSubSlice>> prioritize_sub_slices_finisher(
    shared_ptr<Module> module,
    vector<TemplateSlice> const& slices,
    int nthreads,
    TransitionSystem const& trans_system,
    vector<TreeShape> const& tree_shapes)
{
  for (TemplateSlice const& ts : slices) {
    cout << ts << endl;
  }

  const long long THRES = 1000000000;

  map<vector<int>, TransitionSystem> sub_ts_cache;
  vector<vector<TemplateSubSlice>> res;

  int a = 0;
  while (a < (int)slices.size()) {
    int b = a+1;
    long long tc = slices[a].count;
    while (b < (int)slices.size() && tc + slices[b].count <= THRES) {
      tc += slices[b].count;
      b++;
    }
    if (b == a+1) {
      while (b < (int)slices.size() && slices[b].vars == slices[a].vars) {
        b++;
      }
    }

    vector<TemplateSlice> my_slices;
    for (int i = a; i < b; i++) {
      my_slices.push_back(slices[i]);
    }

    vector_append(res, 
      pack_tightly_dont_exceed_hull(
        module, my_slices, nthreads, trans_system, tree_shapes, sub_ts_cache));

    a = b;
  }

  return res;
}*/

std::vector<std::vector<TemplateSubSlice>> prioritize_sub_slices_finisher(
    shared_ptr<Module> module,
    vector<TemplateSlice> const& slices,
    int nthreads,
    TransitionSystem const& trans_system,
    vector<TreeShape> const& tree_shapes)
{
  int nsorts = module->sorts.size();

  int max_mvars = 0;
  for (TemplateSlice const& ts : slices) {
    max_mvars = max(max_mvars, total_vars(ts));
  }

  vector<vector<TemplateSlice>> all_slices_per_thread;

  int idx = 0;
  vector<vector<int>> max_vars_per_thread;
  vector<long long> counts;
  all_slices_per_thread.resize(nthreads);
  max_vars_per_thread.resize(nthreads);
  counts.resize(nthreads);
  for (int i = 0; i < nthreads; i++) {
    max_vars_per_thread[i].resize(nsorts);
  }

  map<vector<int>, TransitionSystem> sub_ts_cache;

  for (int i = 0; i < (int)slices.size(); i++) {
    TemplateSlice const& ts = slices[i];
    //vector<TemplateSubSlice> new_slices =
    //  split_slice_into_sub_slices(trans_system, tree_shapes, slices[i], sub_ts_cache);
    //random_sort(new_slices, 0, new_slices.size());

    bool from_start = (ts.count <= 100000);

    vector<int> possibilities;
    int jstart = from_start ? 0 : (int)all_slices_per_thread.size() - nthreads;
    for (int j = jstart; j < (int)all_slices_per_thread.size(); j++)
    {
      int m = 0;
      for (int k = 0; k < nsorts; k++) {
        m += max(max_vars_per_thread[j][k], ts.vars[k]);
      }
      if (m <= max_mvars) {
        possibilities.push_back(j);
      }
    }

    if (possibilities.size() == 0) {
      vector<int> i0 = ts.vars;
      all_slices_per_thread.push_back(vector<TemplateSlice>{ts});
      max_vars_per_thread.push_back(i0);
      counts.push_back(ts.count);
      idx++;
    } else {
      int best = possibilities[0];
      for (int j = 1; j < (int)possibilities.size(); j++) {
        if (counts[possibilities[j]] < counts[best]) {
          best = possibilities[j];
        }
      }
      all_slices_per_thread[best].push_back(ts);
      counts[best] += ts.count;
      for (int k = 0; k < nsorts; k++) {
        max_vars_per_thread[best][k] = max(max_vars_per_thread[best][k], ts.vars[k]);
      }
    }
  }

  vector<vector<TemplateSubSlice>> res;

  for (int i = 0; i < (int)all_slices_per_thread.size(); i++) {
    bool last = (i == (int)all_slices_per_thread.size() - 1);

    long long my_nthreads = (last ? nthreads : 1);

    vector_append(res, split_into(all_slices_per_thread[i], my_nthreads, trans_system, sub_ts_cache, tree_shapes));
  }

  return res;
}

vector<vector<vector<TemplateSubSlice>>> prioritize_sub_slices(
    std::shared_ptr<Module> module,
    std::vector<TemplateSlice> const& _slices,
    int nthreads,
    bool is_for_breadth,
    bool by_size,
    bool basic_split)
{
  std::vector<TemplateSlice> slices = _slices;

  if (slices.size() == 0) {
    assert (nthreads == 1);
    vector<TemplateSubSlice> emp;
    std::vector<std::vector<TemplateSubSlice>> res;
    res.push_back(emp);
    vector<vector<vector<TemplateSubSlice>>> res2;
    res2.push_back(res);
    return res2;
  }

  int max_k = 1;

  for (int i = 0; i < (int)slices.size(); i++) {
    vector<TemplateSlice> preds = get_preds(slices[i]);
    for (int j = 0; j < (int)preds.size(); j++) {
      int idx = slices_get_idx(slices, preds[j]);
      if (idx == -1) {
        preds[j].count = 0;
        slices.push_back(preds[j]);
      }
    }
  }

  vector<Node> nodes;
  nodes.resize(slices.size());
  for (int i = 0; i < (int)slices.size(); i++) {
    nodes[i].pred_count = 0;
    nodes[i].ts = slices[i];
    if (slices[i].k > max_k) {
      max_k = slices[i].k;
    }
  }
  for (int i = 0; i < (int)slices.size(); i++) {
    vector<TemplateSlice> preds = get_preds(slices[i]);
    for (int j = 0; j < (int)preds.size(); j++) {
      int idx = slices_get_idx(slices, preds[j]);
      assert (idx != -1);
      nodes[idx].succs.push_back(&nodes[i]);
      nodes[i].pred_count++;
    }
  }

  /*priority_queue<NodePtr> q;
  for (int i = 0; i < (int)slices.size(); i++) {
    if (nodes[i].pred_count == 0) {
      q.push(NodePtr(&nodes[i]));
    }
  }

  vector<TemplateSlice> ordered_slices;

  while (!q.empty()) {
    NodePtr nptr = q.top();
    q.pop();

    ordered_slices.push_back(nptr.node->ts);

    for (Node* succ : nptr.node->succs) {
      succ->pred_count--;
      if (succ->pred_count == 0) {
        q.push(NodePtr(succ));
      }
    }
  }*/
  vector<TemplateSlice> ordered_slices = slices;
  sort(ordered_slices.begin(), ordered_slices.end());

  assert (ordered_slices.size() == slices.size());

  //cout << "----" << endl;
  //for (TemplateSlice const& ts : ordered_slices) {
  //  cout << ts << endl;
  //}
  cout << "ordered_slices: " << ordered_slices.size() << endl;

  ordered_slices = remove_count0(ordered_slices);

  vector<TreeShape> tree_shapes = get_tree_shapes_up_to(max_k);

  //int nsorts = module->sorts.size();
  int max_mvars = 0;
  for (TemplateSlice const& ts : ordered_slices) {
    max_mvars = max(max_mvars, total_vars(ts));
  }

  TransitionSystem trans_system = 
      transition_system_for_slice_list(module, ordered_slices, max_mvars);

  assert(nthreads >= 1);

  if (basic_split) {
    if (by_size) {
      return prioritize_sub_slices_basic_by_size(
          module,
          ordered_slices,
          nthreads,
          trans_system,
          tree_shapes);
    } else {
      return {prioritize_sub_slices_basic(
          module,
          ordered_slices,
          nthreads,
          trans_system,
          tree_shapes)};
    }
  } else if (is_for_breadth) {
    if (by_size) {
      return prioritize_sub_slices_breadth_by_size(
          module,
          ordered_slices,
          nthreads,
          trans_system,
          tree_shapes);
    } else {
      return {prioritize_sub_slices_breadth(
          module,
          ordered_slices,
          nthreads,
          trans_system,
          tree_shapes)};
    }
  } else {
    return {prioritize_sub_slices_finisher(
        module,
        ordered_slices,
        nthreads,
        trans_system,
        tree_shapes)};
  }

  /*vector<vector<TemplateSubSlice>> all_sub_slices_per_thread;

  int idx = 0;
  vector<vector<int>> max_vars_per_thread;
  vector<int> counts;
  all_sub_slices_per_thread.resize(nthreads);
  max_vars_per_thread.resize(nthreads);
  counts.resize(nthreads);
  for (int i = 0; i < nthreads; i++) {
    max_vars_per_thread[i].resize(nsorts);
  }

  map<vector<int>, TransitionSystem> sub_ts_cache;

  for (int i = 0; i < (int)ordered_slices.size(); i++) {
    TemplateSlice const& ts = ordered_slices[i];
    vector<TemplateSubSlice> new_slices =
      split_slice_into_sub_slices(trans_system, tree_shapes, ordered_slices[i], sub_ts_cache);
    random_sort(new_slices, 0, new_slices.size());

    bool from_start = (ts.count <= 100000 || is_for_breadth);

    vector<int> possibilities;
    int jstart = from_start ? 0 : (int)all_sub_slices_per_thread.size() - nthreads;
    for (int j = jstart; j < (int)all_sub_slices_per_thread.size(); j++)
    {
      int m = 0;
      for (int k = 0; k < nsorts; k++) {
        m += max(max_vars_per_thread[j][k], ts.vars[k]);
      }
      if (m <= max_mvars) {
        possibilities.push_back(j);
      }
    }

    if (possibilities.size() == 0) {
      vector<int> i0 = ts.vars;
      all_sub_slices_per_thread.push_back(new_slices);
      max_vars_per_thread.push_back(i0);
      counts.push_back(ts.count);
      idx++;
    } else {
      int best = possibilities[0];
      for (int j = 1; j < (int)possibilities.size(); j++) {
        if (counts[possibilities[j]] < counts[best]) {
          best = possibilities[j];
        }
      }
      vector_append(all_sub_slices_per_thread[best], new_slices);
      counts[best] += ts.count;
      for (int k = 0; k < nsorts; k++) {
        max_vars_per_thread[best][k] = max(max_vars_per_thread[best][k], ts.vars[k]);
      }
    }
  }

  vector<vector<TemplateSubSlice>> res;
  for (int i = 0; i < (int)all_sub_slices_per_thread.size(); i++) {
    if (all_sub_slices_per_thread[i].size() != 0) {
      res.push_back(move(all_sub_slices_per_thread[i]));
      cout << "partition count: " << counts[i] << endl;
    }
  }
  return res;*/

  /*int a = 0;
  while (a < (int)ordered_slices.size()) {
    int b = a;

    long long total_count = 0;
    vector<int> vars;
    vars.resize(module->sorts.size());

    while (b < (int)ordered_slices.size()) {
      int var_sum = 0;
      for (int i = 0; i < (int)vars.size(); i++) {
        vars[i] = max(vars[i], ordered_slices[b].vars[i]);
        var_sum += vars[i];
      }
      if (var_sum > max_mvars) {
        break;
      }

      total_count += ordered_slices[b].count;
      b++;
    }

    long long my_nthreads = (total_count + 1000 - 1) / 1000;
    if (my_nthreads > nthreads) my_nthreads = nthreads;
    assert (my_nthreads > 0);

    vector<vector<TemplateSubSlice>> sub_slices_per_thread;
    sub_slices_per_thread.resize(my_nthreads);

    for (int i = a; i < b; i++) {
      vector<TemplateSubSlice> new_slices =
        split_slice_into_sub_slices(trans_system, tree_shapes, ordered_slices[i]);

      random_sort(new_slices, 0, new_slices.size());
      int k = rand() % sub_slices_per_thread.size();

      for (int j = 0; j < (int)new_slices.size(); j++) {
        sub_slices_per_thread[k].push_back(new_slices[j]);

        k++;
        if (k == (int)sub_slices_per_thread.size()) { 
          k = 0;
        }
      }
    }

    for (int i = 0; i < (int)sub_slices_per_thread.size(); i++) {
      if (sub_slices_per_thread[i].size() > 0) {
        all_sub_slices_per_thread.push_back(
            move(sub_slices_per_thread[i]));
      }
    }

    a = b;
  }

  return all_sub_slices_per_thread;*/

  /*
  for (int i = 0; i < (int)ordered_slices.size(); i++) {
    vector<TemplateSubSlice> new_slices =
        split_slice_into_sub_slices(trans_system, tree_shapes, ordered_slices[i]);

    random_sort(new_slices, 0, new_slices.size());
    int k = rand() % sub_slices_per_thread.size();

    for (int j = 0; j < (int)new_slices.size(); j++) {
      sub_slices_per_thread[k].push_back(new_slices[j]);

      k++;
      if (k == nthreads) { 
        k = 0;
      }
    }
  }

  return sub_slices_per_thread;
  */

}
