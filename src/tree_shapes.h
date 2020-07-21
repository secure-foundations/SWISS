#ifndef TREE_SHAPES_H
#define TREE_SHAPES_H

#include <vector>
#include <string>

struct SymmEdge {
  int idx;
  int inc;

  SymmEdge(int idx, int inc) : idx(idx), inc(inc) { }
};

struct TreeShape {
  bool top_level_is_conj;
  int total;
  std::vector<int> parts;

  // For i < j, there's an edge from j to i
  // if any symmetry-normalized formula should have
  // pieces[i] + inc <= piece[j]
  // This is vector from j to (i, inc) (or to -1 if no such edge)
  // for 0 <= j < total
  // inc will always be 0 or 1.
  std::vector<SymmEdge> symmetry_back_edges;

  std::string to_string() const;
};

std::vector<TreeShape> get_tree_shapes_up_to(int n);

bool is_normalized_for_tree_shape(TreeShape const& ts, std::vector<int> const& pieces);

TreeShape tree_shape_for(bool top_level_is_conj, std::vector<int> const& parts);

#endif
