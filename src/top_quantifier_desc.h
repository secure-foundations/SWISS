#ifndef TOP_QUANTIFIER_DESC_H
#define TOP_QUANTIFIER_DESC_H

#include "logic.h"

enum class QType {
  Forall,
  NearlyForall,
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
 
private:
  std::vector<std::pair<QType, std::vector<VarDecl>>> d;
};

std::pair<TopQuantifierDesc, value> get_tqd_and_body(value);

#endif
