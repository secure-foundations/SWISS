#include "tree_shapes.h"

#include <iostream>

using namespace std;

void get_tree_shapes_of_size_rec(
  vector<TreeShape>& res,
  int n,
  vector<int> const& parts,
  int bound,
  int totalSoFar)
{
  if (totalSoFar > n) {
    return;
  }
  if (totalSoFar == n) {
    TreeShape ts; 
    ts.top_level_is_conj = true;
    ts.total = n;
    ts.parts = parts;
    res.push_back(ts);

    ts.top_level_is_conj = false;
    res.push_back(ts);

    return;
  }

  for (int i = 1; i <= bound; i++) {
    vector<int> parts1 = parts;
    parts1.push_back(i);
    get_tree_shapes_of_size_rec(res, n, parts1, i, totalSoFar + i);
  }
}

void get_tree_shapes_of_size(
  vector<TreeShape>& res,
  int n)
{
  if (n == 1) {
    TreeShape ts;
    ts.top_level_is_conj = true;
    ts.total = 1;
    ts.parts.push_back(1);
    res.push_back(ts);
  } else {
    for (int i = 1; i < n; i++) {
      vector<int> parts;
      parts.push_back(i);
      get_tree_shapes_of_size_rec(res, n, parts, i, i);
    }
  }
}

void make_back_edges(TreeShape& ts)
{
  vector<int> starts;
  for (int i = 0; i < (int)ts.parts.size(); i++) {
    starts.push_back(ts.symmetry_back_edges.size());
    for (int j = 0; j < (int)ts.parts[i]; j++) {
      if (j > 0) {
        ts.symmetry_back_edges.push_back(SymmEdge(
            ts.symmetry_back_edges.size() - 1,
            1));
      } else {
        if (i > 0 && ts.parts[i] == ts.parts[i-1]) {
          ts.symmetry_back_edges.push_back(SymmEdge(
            starts[i-1],
            ts.parts[i] == 1 ? 1 : 0));
        } else {
          ts.symmetry_back_edges.push_back(SymmEdge(-1, -1));
        }
      }
    }
  }
}

vector<TreeShape> get_tree_shapes_up_to(int n)
{
  vector<TreeShape> res;
  for (int i = 1; i <= n; i++) {
    get_tree_shapes_of_size(res, i);
  }
  for (int i = 0; i < (int)res.size(); i++) {
    make_back_edges(res[i]);
  }
  return res;
}

bool is_lexicographically_lt(vector<int> const& pieces, int a, int b, int len)
{
  for (int i = 0; i < len; i++) {
    if (pieces[a+i] < pieces[b+i]) return true;
    if (pieces[a+i] > pieces[b+i]) return false;
  }
  return false;
}

bool is_normalized_for_tree_shape(TreeShape const& ts, vector<int> const& pieces)
{
  int pos = ts.parts[0];
  for (int i = 1; i < (int)ts.parts.size(); i++) {
    if (ts.parts[i] == ts.parts[i-1]) {
      if (!is_lexicographically_lt(pieces, pos - ts.parts[i], pos, ts.parts[i])) {
        return false;
      }
    }
    pos += ts.parts[i];
  }
  return true;
}

std::string TreeShape::to_string() const {
  string s = top_level_is_conj ? "AND" : "OR";
  s += " [";
  for (int i = 0; i < (int)parts.size(); i++) {
    if (i > 0) s += ", ";
    s += ::to_string(parts[i]);
  }
  return s + "]";
}
