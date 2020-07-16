#include "template_priority.h"

#include <queue>
#include <iostream>

#include "template_counter.h"
#include "tree_shapes.h"

using namespace std;

std::vector<TemplateSlice> break_into_slices(
  shared_ptr<Module> module,
  TemplateSpace const& ts)
{
  vector<TemplateSlice> forall_slices = 
      count_many_templates(module, ts);
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
  cout << "couldn't find" << endl;
  cout << ts << endl;
  assert (false);
}

std::vector<std::vector<TemplateSubSlice>> 
  prioritize_sub_slices(std::vector<TemplateSlice> const& slices, int nthreads)
{
  int max_k = 1;
  
  vector<Node> nodes;
  nodes.resize(slices.size());
  for (int i = 0; i < (int)slices.size(); i++) {
    cout << slices[i] << endl;
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

  vector<TemplateSubSlice> sub_slices;
  for (int i = 0; i < (int)ordered_slices.size(); i++) {
    TemplateSlice ts = ordered_slices[i];
    if (ts.count != 0) {
      if (ts.depth == 1) {
        TemplateSubSlice tss;
        tss.ts = ts;
        sub_slices.push_back(tss);
      } else {
        for (int j = 0; j < (int)tree_shapes.size(); j++) {
          if (tree_shapes[j].total == ts.k) {
            TemplateSubSlice tss;
            tss.ts = ts;
            tss.tree_idx = j;
            sub_slices.push_back(tss);
          }
        }
      }
    }
  }

  assert (nthreads == 1);
  return { sub_slices };
}
