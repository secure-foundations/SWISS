#ifndef SUBSEQUENCE_TRIE_H
#define SUBSEQUENCE_TRIE_H

#include <vector>
#include <memory>

// Stores a set S of sequences
// Supports the query: given a sequence t, does there exist subsequence s in S
// such that s is a subsequence of t.

// Query is exponential in t.size(), but we expect to use this where t.size()
// is around 5.

struct TrieNode {
  std::vector<std::unique_ptr<TrieNode>> children;
  bool isTerm;

  TrieNode(int n) {
    children.resize(n);
    isTerm = false;
  }
};

inline bool subseq_query(std::vector<int> const& t, int idx, TrieNode* node, int& upTo) {
  if (node->isTerm) {
    upTo = idx;
    return true;
  }

  if (idx == t.size()) {
    return false;
  }

  if (subseq_query(t, idx + 1, node, upTo)) {
    return true;
  }

  if (node->children[t[idx]] != nullptr) {
    return subseq_query(t, idx + 1, node->children[t[idx]].get(), upTo);
  }

  return false;
}

struct SubsequenceTrie {
  std::unique_ptr<TrieNode> root;
  int alphabet_size;

  SubsequenceTrie() { }

  SubsequenceTrie(int alpha) {
    alphabet_size = alpha;
    root = std::unique_ptr<TrieNode>(new TrieNode(alpha));
  }

  void insert(std::vector<int> const& s) {
    TrieNode* node = root.get();
    for (int i = 0; i < s.size(); i++) {
      int j = s[i];
      if (node->children[j] == nullptr) {
        node->children[j] = std::unique_ptr<TrieNode>(new TrieNode(alphabet_size));
      }
      node = node->children[j].get();
    }
    node->isTerm = true;
  }

  // Determine if a subsequence of t exists in s
  bool query(std::vector<int> const& t, int& upTo) {
    return subseq_query(t, 0, root.get(), upTo);
  }
};

#endif
