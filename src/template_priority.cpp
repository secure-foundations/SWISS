#include "template_priority.h"

#include <queue>
#include <iostream>

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
    vector<TemplateSlice> const& forall_slices)
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
        res.push_back(ts1);
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
    return node->ts.count < other.node->ts.count;
  }
};

vector<TemplateSlice> get_preds(TemplateSlice const& ts)
{
  //cout << "    " << ts << endl;

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
  //for (TemplateSlice const& ts1 : res) {
    //cout << "        " << ts1 << endl;
  //}
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
  //cout << "couldn't find" << endl;
  //cout << ts << endl;
  //assert (false);
  return -1;
}

/*
static void getSpaceChunk_rec(vector<SpaceChunk>& res,
  int tree_shape_idx, TreeShape const& ts,
  vector<int>& indices, int i, VarIndexState const& vis,
  vector<value> const& pieces,
  vector<VarIndexTransition> const& var_index_transitions, int sz)
{
  if (i == (int)indices.size()) {
    SpaceChunk sc;
    sc.tree_idx = tree_shape_idx;
    sc.size = ts.total;
    sc.nums = indices;
    res.push_back(move(sc));
    return;
  }
  SymmEdge const& symm_edge = ts.symmetry_back_edges[i];
  int t = symm_edge.idx == -1 ? 0 : indices[symm_edge.idx] + symm_edge.inc;
  for (int j = t; j < (int)pieces.size(); j++) {
    if (var_index_is_valid_transition(vis, var_index_transitions[j].pre)) {
      VarIndexState next(vis.indices.size());
      var_index_do_transition(vis, var_index_transitions[j].res, next);
      indices[i] = j;
      getSpaceChunk_rec(res, tree_shape_idx, ts, indices, i+1, next,
          pieces, var_index_transitions, sz);
    }
  }
}

void AltDepth2CandidateSolver::getSpaceChunk(std::vector<SpaceChunk>& res)
{
  for (int i = 0; i < (int)tree_shapes.size(); i++) {
    //cout << tree_shapes[i].to_string() << endl;
    int sz = tree_shapes[i].total;

    int k;
    if (sz > 4) k = sz - 2;
    else k = 2;

    int j = k < sz ? sz - k : 0;
    VarIndexState vis = var_index_states[0];
    vector<int> indices;
    indices.resize(j);
    getSpaceChunk_rec(res, i, tree_shapes[i], indices, 0, vis,
        pieces, var_index_transitions, sz);
  }
  //cout << "done" << endl;
}*/

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
  vector<vector<int>> res;
  int sz = slice.k - 2;
  if (sz < 0) sz = 0;
  if (sz > 2) sz = 2;

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
    vector<TemplateSlice> const& slices)
{
  TemplateSpace tspace = space_containing_slices_ignore_quants(module, slices);
  value templ = tspace.make_templ(module);
  EnumInfo ei(module, templ);
  return build_transition_system(
      get_var_index_init_state(module, templ),
      ei.var_index_transitions);
}

vector<TemplateSubSlice> split_slice_into_sub_slices(
    TransitionSystem const& trans_system,
    vector<TreeShape> const& tree_shapes,
    TemplateSlice const& slice)
{
  auto p = get_subslice_index_map(trans_system, slice);
  auto sub_trans_system = p.first.second;

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

std::vector<std::vector<TemplateSubSlice>> prioritize_sub_slices(
    std::shared_ptr<Module> module,
    std::vector<TemplateSlice> const& _slices,
    int nthreads)
{
  std::vector<TemplateSlice> slices = _slices;

  if (slices.size() == 0) {
    assert (nthreads == 1);
    vector<TemplateSubSlice> emp;
    std::vector<std::vector<TemplateSubSlice>> res;
    res.push_back(emp);
    return res;
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
    //cout << slices[i] << endl;
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

  priority_queue<NodePtr> q;
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
  }

  assert (ordered_slices.size() == slices.size());

  vector<TreeShape> tree_shapes = get_tree_shapes_up_to(max_k);

  TransitionSystem trans_system = 
      transition_system_for_slice_list(module, ordered_slices);

  assert(nthreads >= 1);
  vector<vector<TemplateSubSlice>> sub_slices_per_thread;
  sub_slices_per_thread.resize(nthreads);

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
}
