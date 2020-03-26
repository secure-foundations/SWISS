#ifndef TOP_QUANTIFIER_DESC_H
#define TOP_QUANTIFIER_DESC_H

#include "logic.h"

enum class QType {
  Forall,
  NearlyForall,
  Exists,
};

struct QRange {
  int start, end;
  std::vector<VarDecl> decls;
  QType qtype;
};

struct QSRange {
  int start, end;
  std::vector<VarDecl> decls;
  QType qtype;
  lsort sort;
};

class TopQuantifierDesc {
public:
  TopQuantifierDesc(value);

  std::vector<VarDecl> decls() const;

  std::vector<QRange> with_foralls_grouped() const;
  std::vector<QRange> with_foralls_ungrouped() const;

  std::vector<QSRange> grouped_by_sort() const;

  value with_body(value body) const;
  int weighted_sort_count(std::string sort) const;

  //value rename_into(value);

  std::vector<value> rename_into_all_possibilities(value);
 
private:
  std::vector<std::pair<QType, std::vector<VarDecl>>> d;
};

std::pair<TopQuantifierDesc, value> get_tqd_and_body(value);

enum class AltType {
  Forall,
  Exists,
};

struct Alternation {
  AltType altType;
  std::vector<VarDecl> decls;

  bool is_forall() { return altType == AltType::Forall; }
  bool is_exists() { return altType == AltType::Exists; }
};

class TopAlternatingQuantifierDesc {
public:
  TopAlternatingQuantifierDesc(value v);
  std::vector<Alternation> alternations() { return alts; }

  static value get_body(value);
  value with_body(value);
  std::vector<QSRange> grouped_by_sort() const;

  TopAlternatingQuantifierDesc replace_exists_with_forall() const;
  
  //value rename_into(value);
  // Given this = forall A: node, B: node, C: node ...
  // and value = forall X: node, Y: node . f(X) & g(Y)
  // returns {
  //    forall A: node, B: node, C: node . f(A) & g(B)
  //    forall A: node, B: node, C: node . f(A) & g(C)
  //    forall A: node, B: node, C: node . f(B) & g(C)
  // }
  std::vector<value> rename_into_all_possibilities(value);

private:
  std::vector<Alternation> alts;

  TopAlternatingQuantifierDesc() { }

  //friend value rename_into(std::vector<Alternation> const& alts, value v);
  friend class TopQuantifierDesc;

  void rename_into_all_possibilities_rec(
    TopAlternatingQuantifierDesc const& v_taqd,
    int v_idx,
    int v_inner_idx,
    int this_idx,
    int this_inner_idx,
    value const& body,
    std::map<iden, iden>& var_map,
    std::vector<value>& results);
};

#endif
