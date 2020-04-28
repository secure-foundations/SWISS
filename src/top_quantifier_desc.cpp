#include "top_quantifier_desc.h"

#include <set>
#include <cassert>
#include <iostream>

using namespace std;

TopQuantifierDesc::TopQuantifierDesc(value v) {
  while (true) {
    if (Forall* f = dynamic_cast<Forall*>(v.get())) {
      for (VarDecl decl : f->decls) {
        d.push_back(make_pair(QType::Forall, vector<VarDecl>{decl}));
      }
      v = f->body;
    } else if (NearlyForall* f = dynamic_cast<NearlyForall*>(v.get())) {
      d.push_back(make_pair(QType::NearlyForall, f->decls));
      v = f->body;
    } else {
      break;
    }
  }
}

pair<TopQuantifierDesc, value> get_tqd_and_body(value v)
{
  TopQuantifierDesc tqd(v);

  while (true) {
    if (Forall* f = dynamic_cast<Forall*>(v.get())) {
      v = f->body;
    } else if (NearlyForall* f = dynamic_cast<NearlyForall*>(v.get())) {
      v = f->body;
    } else {
      break;
    }
  }

  return make_pair(tqd, v);
}

vector<VarDecl> TopQuantifierDesc::decls() const {
  vector<VarDecl> res;
  for (auto p : d) {
    for (auto decl : p.second) {
      res.push_back(decl);
    }
  }
  return res;
}

vector<QRange> TopQuantifierDesc::with_foralls_grouped() const {
  vector<QRange> res;
  QRange cur;
  cur.start = 0;
  cur.end = 0;

  for (auto p : d) {
    QType ty = p.first;
    if (ty == QType::Forall) {
      cur.end++;
      cur.decls.push_back(p.second[0]);
      cur.qtype = ty;
    } else if (ty == QType::NearlyForall) {
      if (cur.end > cur.start) {
        res.push_back(cur);
      }

      cur.start = cur.end;
      cur.decls = p.second;
      cur.end = cur.start + cur.decls.size();
      cur.qtype = ty;
      res.push_back(cur);

      cur.start = cur.end;
      cur.decls.clear();
    } else {
      assert(false);
    }
  }

  if (cur.end > cur.start) {
    res.push_back(cur);
  }

  return res;
}

vector<QSRange> TopQuantifierDesc::grouped_by_sort() const {
  vector<QSRange> res;
  QSRange cur;
  cur.start = 0;
  cur.end = 0;

  for (auto p : d) {
    QType ty = p.first;
    for (auto decl : p.second) {
      if (cur.end > cur.start && !(cur.qtype == ty && sorts_eq(cur.decls[0].sort, decl.sort))) {
        res.push_back(cur);
        cur.start = cur.end;
        cur.decls.clear();
      }

      cur.decls.push_back(decl);
      cur.qtype = ty;
      cur.sort = decl.sort;
      cur.end++;
    }

    if (ty == QType::NearlyForall && cur.end > cur.start) {
      res.push_back(cur);
      cur.start = cur.end;
      cur.decls.clear();
    }
  }

  if (cur.end > cur.start) {
    res.push_back(cur);
  }

  set<string> names;
  for (int i = 0; i < (int)res.size(); i++) {
    if (i > 0 && res[i].qtype != res[i-1].qtype) {
      names.clear();
    }
    UninterpretedSort* usort = dynamic_cast<UninterpretedSort*>(res[i].decls[0].sort.get());
    assert(usort != NULL);
    if (res[i].qtype == QType::Forall) {
      assert (names.count(usort->name) == 0);
    }
    names.insert(usort->name);
  }

  return res;
}

value TopQuantifierDesc::with_body(value body) const {
  vector<QRange> qrs = with_foralls_grouped();
  for (int i = qrs.size() - 1; i >= 0; i--) {
    QRange& qr = qrs[i];
    if (qr.qtype == QType::Forall) {
      body = v_forall(qr.decls, body);
    }
    else if (qr.qtype == QType::NearlyForall) {
      body = v_nearlyforall(qr.decls, body);
    }
    else {
      assert(false);
    }
  }
  return body;
}

int TopQuantifierDesc::weighted_sort_count(std::string sort) const {
  lsort so = s_uninterp(sort);
  int count = 0;
  for (auto p : d) {
    QType ty = p.first;
    for (VarDecl const& decl : p.second) {
      if (sorts_eq(so, decl.sort)) {
        count += (ty == QType::NearlyForall ? 2 : 1);
      }
    }
  }
  return count;
}

TopAlternatingQuantifierDesc::TopAlternatingQuantifierDesc(value v)
{
  Alternation alt;
  while (true) {
    if (Forall* f = dynamic_cast<Forall*>(v.get())) {
      if (alt.decls.size() > 0 && alt.altType != AltType::Forall) {
        alts.push_back(alt);
        alt.decls = {};
      }

      alt.altType = AltType::Forall;

      for (VarDecl decl : f->decls) {
        alt.decls.push_back(decl);
      }

      v = f->body;
    } else if (/* NearlyForall* f = */ dynamic_cast<NearlyForall*>(v.get())) {
      assert(false);
    } else if (Exists* f = dynamic_cast<Exists*>(v.get())) {
      if (alt.decls.size() > 0 && alt.altType != AltType::Exists) {
        alts.push_back(alt);
        alt.decls = {};
      }

      alt.altType = AltType::Exists;

      for (VarDecl decl : f->decls) {
        alt.decls.push_back(decl);
      }

      v = f->body;
    } else {
      break;
    }
  }

  if (alt.decls.size() > 0) {
    alts.push_back(alt);
  }
}

value TopAlternatingQuantifierDesc::get_body(value v)
{
  while (true) {
    if (Forall* f = dynamic_cast<Forall*>(v.get())) {
      v = f->body;
    }
    else if (Exists* f = dynamic_cast<Exists*>(v.get())) {
      v = f->body;
    }
    else {
      break;
    }
  }
  return v;
}

value TopAlternatingQuantifierDesc::with_body(value v)
{
  for (int i = alts.size() - 1; i >= 0; i--) {
    Alternation& alt = alts[i];
    if (alt.altType == AltType::Forall) {
      v = v_forall(alt.decls, v);
    } else {
      v = v_exists(alt.decls, v);
    }
  }
  return v;
}

vector<QSRange> TopAlternatingQuantifierDesc::grouped_by_sort() const {
  vector<QSRange> res;
  QSRange cur;
  cur.start = 0;
  cur.end = 0;

  for (auto alt : alts) {
    assert(alt.altType == AltType::Forall || alt.altType == AltType::Exists);
    QType ty = alt.altType == AltType::Forall ? QType::Forall : QType::Exists;
    for (auto decl : alt.decls) {
      if (cur.end > cur.start && !(cur.qtype == ty && sorts_eq(cur.decls[0].sort, decl.sort))) {
        res.push_back(cur);
        cur.start = cur.end;
        cur.decls.clear();
      }

      cur.decls.push_back(decl);
      cur.qtype = ty;
      cur.sort = decl.sort;
      cur.end++;
    }

    /*if (ty == QType::NearlyForall && cur.end > cur.start) {
      res.push_back(cur);
      cur.start = cur.end;
      cur.decls.clear();
    }*/
  }

  if (cur.end > cur.start) {
    res.push_back(cur);
  }

  set<string> names;
  for (int i = 0; i < (int)res.size(); i++) {
    if (i > 0 && res[i].qtype != res[i-1].qtype) {
      names.clear();
    }
    UninterpretedSort* usort = dynamic_cast<UninterpretedSort*>(res[i].decls[0].sort.get());
    assert(usort != NULL);
    if (res[i].qtype == QType::Forall) {
      assert (names.count(usort->name) == 0);
    }
    names.insert(usort->name);
  }

  return res;
}

TopAlternatingQuantifierDesc TopAlternatingQuantifierDesc::
    replace_exists_with_forall() const {
  Alternation bigalt;
  bigalt.altType = AltType::Forall;
  for (Alternation const& alt : alts) {
    for (VarDecl decl : alt.decls) {
      bigalt.decls.push_back(decl);
    }
  }

  TopAlternatingQuantifierDesc res;
  res.alts.push_back(bigalt); 
  return res;
}

/*value rename_into(vector<Alternation> const& alts, value v)
{
  int alt_idx = 0;
  int inner_idx = 0;

  TopAlternatingQuantifierDesc atqd(v);
  vector<Alternation>& alts1 = atqd.alts;
  int alt_idx1 = 0;
  int inner_idx1 = 0;

  map<iden, iden> var_map;

  while (alt_idx1 < (int)alts1.size()) {
    while (inner_idx1 < (int)alts1[alt_idx1].decls.size()) {
      if (alt_idx == (int)alts.size()) {
        return nullptr;
      }
      if (inner_idx == (int)alts[alt_idx].decls.size()) {
        inner_idx = 0;
        alt_idx++;
      } else {
        if (alts[alt_idx].altType == alts1[alt_idx1].altType
          && sorts_eq(
            alts[alt_idx].decls[inner_idx].sort,
            alts1[alt_idx1].decls[inner_idx1].sort))
        {
          var_map.insert(make_pair(
            alts1[alt_idx1].decls[inner_idx1].name,
            alts[alt_idx].decls[inner_idx].name));
          inner_idx1++;
        }
        inner_idx++;
      }
    }
    inner_idx1 = 0;
    alt_idx1++;
  }

  value substed = v->replace_var_with_var(TopAlternatingQuantifierDesc::get_body(var_map));

  TopAlternatingQuantifierDesc new_taqd;
  new_taqd.alts = alts;
  return new_taqd.with_body(substed);
}

value TopQuantifierDesc::rename_into(value v)
{
  cout << "Top rename_into " << v->to_string() << endl;
  v = remove_unneeded_quants(v.get());
  cout << "removed unneeded " << v->to_string() << endl;

  vector<Alternation> alts;
  alts.resize(d.size());
  for (int i = 0; i < (int)d.size(); i++) {
    assert (d[i].first == QType::Forall);
    alts[i].decls = d[i].second;
    alts[i].altType = AltType::Forall;
  }

  value res = ::rename_into(alts, v);

  if (res) {
    cout << "result " << res->to_string() << endl;
  } else {
    cout << "result null" << endl;
  }

  return res;
}

value TopAlternatingQuantifierDesc::rename_into(value v)
{
  cout << "Alt rename_into " << v->to_string() << endl;
  v = remove_unneeded_quants(v.get());
  cout << "removed unneeded " << v->to_string() << endl;

  value res = ::rename_into(alts, v);

  if (res) {
    cout << "result " << res->to_string() << endl;
  } else {
    cout << "result null" << endl;
  }

  return res;
}*/


/*void TopAlternatingQuantifierDesc::rename_into_all_possibilities_rec(
    TopAlternatingQuantifierDesc const& v_taqd,
    int v_idx,
    int v_inner_idx,
    int this_idx,
    int this_inner_idx,
    value const& body,
    map<iden, iden>& var_map,
    vector<value>& results)
{
  if (v_idx == (int)v_taqd.alts.size()) {
    results.push_back(this->with_body(body->replace_var_with_var(var_map)));
    return;
  }

  if (v_inner_idx == (int)v_taqd.alts[v_idx].decls.size()) {
    rename_into_all_possibilities_rec(
        v_taqd,
        v_idx + 1, 0,
        this_idx, this_inner_idx,
        body, var_map, results);
    return;
  }

  if (this_idx == (int)this->alts.size()) {
    return;
  }

  if (this_inner_idx == (int)this->alts[this_idx].decls.size()) {
    rename_into_all_possibilities_rec(
        v_taqd,
        v_idx, v_inner_idx,
        this_idx + 1, 0,
        body, var_map, results);
    return;
  }

  if (v_taqd.alts[v_idx].altType != this->alts[this_idx].altType) {
    rename_into_all_possibilities_rec(
        v_taqd,
        v_idx, v_inner_idx,
        this_idx + 1, 0,
        body, var_map, results);
    return;
  }

  rename_into_all_possibilities_rec(
      v_taqd,
      v_idx, v_inner_idx,
      this_idx, this_inner_idx + 1,
      body, var_map, results);

  if (sorts_eq(
    v_taqd.alts[v_idx].decls[v_inner_idx].sort,
    this->alts[this_idx].decls[this_inner_idx].sort))
  {
    var_map[v_taqd.alts[v_idx].decls[v_inner_idx].name] =
        this->alts[this_idx].decls[this_inner_idx].name;
    rename_into_all_possibilities_rec(v_taqd, v_idx, v_inner_idx + 1, this_idx, this_inner_idx + 1, body, var_map, results);
  }
}*/

static bool is_valid(vector<vector<pair<int,int>>>& candidate) {
  for (int i = 0; i < (int)candidate.size(); i++) {
    for (int j = 0; j < (int)candidate[i].size(); j++) {
      for (int k = i; k < (int)candidate.size(); k++) {
        for (int l = 0; l < (int)candidate[k].size(); l++) {
          if (!(i == k && j == l) && candidate[i][j] == candidate[k][l]) {
            return false;
          }
        }
      }
    }
  }

  int last_min_x;
  int last_max_x;
  for (int i = 0; i < (int)candidate.size(); i++) {
    int min_x = candidate[i][0].first;
    int max_x = candidate[i][0].first;
    for (int j = 1; j < (int)candidate[i].size(); j++) {
      min_x = min(min_x, candidate[i][j].first);
      max_x = max(max_x, candidate[i][j].first);
    }

    if (i > 0) {
      if (min_x < last_max_x) {
        return false;
      }
    }

    last_min_x = min_x;
    last_max_x = max_x;
  }

  return true;
}
    
std::vector<value> TopAlternatingQuantifierDesc::rename_into_all_possibilities(value v) {
  cout << "rename_into_all_possibilities " << v->to_string() << endl;

  v = remove_unneeded_quants(v.get());

  TopAlternatingQuantifierDesc taqd(v);

  value body = TopAlternatingQuantifierDesc::get_body(v);

  vector<vector<vector<pair<int,int>>>> possibilities;
  vector<vector<int>> indices;
  vector<vector<pair<int,int>>> candidate;

  possibilities.resize(taqd.alts.size());
  indices.resize(taqd.alts.size());
  candidate.resize(taqd.alts.size());
  for (int i = 0; i < (int)taqd.alts.size(); i++) {
    possibilities[i].resize(taqd.alts[i].decls.size());
    indices[i].resize(taqd.alts[i].decls.size());
    candidate[i].resize(taqd.alts[i].decls.size());
    for (int j = 0; j < (int)taqd.alts[i].decls.size(); j++) {
      indices[i][j] = 0;

      VarDecl const& decl = taqd.alts[i].decls[j];
      for (int k = 0; k < (int)this->alts.size(); k++) {
        if (this->alts[k].altType == taqd.alts[i].altType) {
          for (int l = 0; l < (int)this->alts[k].decls.size(); l++) {
            if (sorts_eq(decl.sort, this->alts[k].decls[l].sort)) {
              possibilities[i][j].push_back(make_pair(k, l));
            }
          }
        }
      }

      if (possibilities[i][j].size() == 0) {
        return {};
      }
    }
  }

  vector<value> results;

  while (true) {
    for (int i = 0; i < (int)taqd.alts.size(); i++) {
      for (int j = 0; j < (int)taqd.alts[i].decls.size(); j++) {
        candidate[i][j] = possibilities[i][j][indices[i][j]];
      }
    }

    if (is_valid(candidate)) {
      map<iden, iden> var_map;
      for (int i = 0; i < (int)taqd.alts.size(); i++) {
        for (int j = 0; j < (int)taqd.alts[i].decls.size(); j++) {
          var_map.insert(make_pair(taqd.alts[i].decls[j].name,
            this->alts[candidate[i][j].first].decls[candidate[i][j].second].name));
        }
      }
      results.push_back(this->with_body(body->replace_var_with_var(var_map)));
    }

    for (int i = 0; i < (int)taqd.alts.size(); i++) {
      for (int j = 0; j < (int)taqd.alts[i].decls.size(); j++) {
        indices[i][j]++;
        if (indices[i][j] == (int)possibilities[i][j].size()) {
          indices[i][j] = 0;
        } else {
          goto continue_loop;
        }
      }
    }

    break;

    continue_loop: { }
  }

  for (value res : results) {
    cout << "result: " << res->to_string() << endl;
  }

  return results;
}

std::vector<value> TopQuantifierDesc::rename_into_all_possibilities(value v) {
  vector<Alternation> alts;
  alts.resize(d.size());
  for (int i = 0; i < (int)d.size(); i++) {
    assert (d[i].first == QType::Forall);
    alts[i].decls = d[i].second;
    alts[i].altType = AltType::Forall;
  }
 
  TopAlternatingQuantifierDesc taqd;
  taqd.alts = alts;
  return taqd.rename_into_all_possibilities(v);
}
