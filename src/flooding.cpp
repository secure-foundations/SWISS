#include "flooding.h"

#include <algorithm>
#include <iostream>

#include "clause_gen.h"

using namespace std;

// done start with x | not x
// done start with axioms
// done substitute for x in forall x . ...
// TODO start with (exists x . f(x) | not f(y)) and so on
// done basic implication: a & (a -> b)
// done replace forall with existentials
// TODO generalize stuff to existentials
// done permute variables
// done substitutions in (A=B -> stuff)
// done if forall(stuff -> A=B) then do substitutions
// done for axioms, instantiate universals in order to get stuff in the TemplateSpace

Flood::Flood(
    std::shared_ptr<Module> module,
    TemplateSpace const& forall_tspace,
    int max_e)
  : module(module)
  , forall_taqd(v_template_hole())
{
  this->nsorts = module->sorts.size();
  this->max_k = forall_tspace.k;
  this->max_e = max_e;
  this->forall_tspace = forall_tspace;

  value templ = forall_tspace.make_templ(module);
  this->forall_taqd = TopAlternatingQuantifierDesc(templ);
  this->clauses = EnumInfo(module, templ).clauses;

  assert (forall_tspace.depth == 1);

  this->init_piece_to_index();
  this->init_negation_map();
  this->init_masks();
  this->init_eq_substs();
  this->init_universal_instantation_substitutions();
}

std::vector<RedundantDesc> Flood::get_initial_redundant_descs(std::vector<value> const& extras)
{
  assert (this->entries.size() == 0);

  add_negations();
  add_axioms_and_starting_formulas(extras);

  return make_results(0);
}

std::vector<RedundantDesc> Flood::add_formula(value v)
{
  int initial_entry_len = (int)entries.size();
  do_add(v);
  return make_results(initial_entry_len);
}

void Flood::do_add(value v)
{
  int cur = (int)entries.size();

  vector<Entry> new_es = value_to_entries(v);

  for (Entry const& e : new_es) {
    add_checking_subsumes(e);
  }

  while (cur < (int)entries.size()) {
    if (!entries[cur].subsumed) {
      Entry e1 = entries[cur]; // copy out of `entries` vector so we can pass by ref
      process(e1);
      for (int j = 0; j < cur; j++) {
        if (!entries[j].subsumed) {
          Entry e2 = entries[j]; // copy out of `entries` vector so we can pass by ref
          process2(e1, e2);
        }
      }
    }
    cur++;
  }
}

void Flood::init_piece_to_index() {
  for (int i = 0; i < (int)clauses.size(); i++) {
    value v = clauses[i];
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
    piece_to_index.insert(make_pair(ComparableValue(v), i));
  }
}

int Flood::get_index_of_piece(value p) {
  auto it = piece_to_index.find(ComparableValue(p));
  if (it == piece_to_index.end()) {
    return -1;
  }
  return it->second;
}

void sort_and_remove_dupes(vector<int>& t)
{
  if (t.size() == 0) return;
  sort(t.begin(), t.end());
  int cur = 1;
  for (int i = 1; i < (int)t.size(); i++) {
    if (t[i] == t[i-1]) {
    } else {
      t[cur] = t[i];
      cur++;
    }
  }
  t.resize(cur);
}

bool Flood::get_single_entry_of_value(value inv, vector<int>& t) {
  while (true) {
    if (Forall* f = dynamic_cast<Forall*>(inv.get())) {
      inv = f->body;
    }
    else if (Exists* f = dynamic_cast<Exists*>(inv.get())) {
      inv = f->body;
    }
    else {
      break;
    }
  }
  Or* o = dynamic_cast<Or*>(inv.get());
  if (o != NULL) {
    t.resize(o->args.size());
    for (int i = 0; i < (int)t.size(); i++) {
      int idx = get_index_of_piece(o->args[i]);
      if (idx == -1) { return false; }
      t[i] = idx;
    }
    sort_and_remove_dupes(t);
  } else {
    t.resize(1);
    int idx = get_index_of_piece(inv);
    if (idx == -1) { return false; }
    t[0] = idx;
  }
  return true;
}

std::vector<Entry> Flood::value_to_entries(value v)
{
  Entry e;
  e.subsumed = false;
  e.forall_mask = 0;
  e.exists_mask = 0;

  value inv = v;
  while (true) {
    if (Forall* f = dynamic_cast<Forall*>(inv.get())) {
      for (VarDecl const& decl : f->decls) {
        e.forall_mask |= (1 << sort_idx_of_module(module, decl.sort));
      }
      inv = f->body;
    }
    else if (Exists* f = dynamic_cast<Exists*>(inv.get())) {
      for (VarDecl const& decl : f->decls) {
        e.exists_mask |= (1 << sort_idx_of_module(module, decl.sort));
      }
      inv = f->body;
    }
    else {
      break;
    }
  }

  vector<Entry> res;

  std::vector<value> permuted_values =
      forall_taqd.rename_into_all_possibilities_ignore_quantifier_types(v);
  for (value pv : permuted_values) {
    bool success = get_single_entry_of_value(pv, e.v /* output */);
    if (success) {
      res.push_back(e);
    }
  }

  return res;
}

inline bool matches_forall_mask(uint32_t m, uint32_t f)
{
  // need to check that f is disjoint from m
  return (f & m) == 0;
}

inline bool matches_exists_mask(uint32_t m, uint32_t e)
{
  // need to check that e is a subset of m
  return (e | m) == m;
}

void Flood::dump_entry(Entry const& e)
{
  for (int i = 0; i < nsorts; i++) {
    if (e.forall_mask & (1 << i)) {
      cout << "forall " << module->sorts[i] << " . ";
    } else if (e.exists_mask & (1 << i)) {
      cout << "exists " << module->sorts[i] << " . ";
    }
  }
  for (int j = 0; j < (int)e.v.size(); j++) {
    value v = clauses[e.v[j]];
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
    if (j > 0) {
      cout << " | ";
    }
    cout << v->to_string();
  }
  cout << " ( ";
  for (int j = 0; j < (int)e.v.size(); j++) {
    cout << e.v[j] << " ";
  }
  cout << ") " << e.forall_mask << " " << e.exists_mask;
  cout << endl;
}

std::vector<RedundantDesc> Flood::make_results(int start)
{
  std::vector<RedundantDesc> res;
  for (int i = start; i < (int)entries.size(); i++) {
    //cout << "ENTRY: (" << i << ") ";
    //dump_entry(entries[i]);
    
    for (uint32_t m = 0; m < (1 << nsorts); m++) {
      if (matches_forall_mask(m, entries[i].forall_mask)
        && matches_exists_mask(m, entries[i].exists_mask))
      {
        RedundantDesc rd;
        rd.v = entries[i].v;
        rd.quant_mask = m;
        res.push_back(rd);
      }
    }
  }
  return res;
}

inline bool is_indices_subset(vector<int> const& a, vector<int> const& b)
{
  if (a.size() == 0) { return true; }
  if (b.size() == 0) return false;
  int i = 0;
  int j = 0;
  while (true) {
    if (a[i] < b[j]) {
      return false;
    }
    else if (a[i] == b[j]) {
      i++;
      j++;
      if (i >= (int)a.size()) { return true; }
      if (j >= (int)b.size()) return false;
    }
    else {
      j++;
      if (j >= (int)b.size()) return false;
    }
  }
}

// returns true if e1 <= e2
bool does_subsume(Entry const& e1, Entry const& e2)
{
  return
      !(e1.forall_mask & e2.exists_mask)
   && !(e2.forall_mask & e1.exists_mask)
   && is_indices_subset(e1.v, e2.v);
}

void Flood::add_checking_subsumes(Entry const& e) {
  if (e.v.size() == 0) {
    // TODO application ought to handle this gracefully
    assert(false && "we were able to prove 'false' as an invariant, this is probably unexpected");
  }

  assert ((e.forall_mask & e.exists_mask) == 0);

  if (in_bounds(e)) {
    for (int i = 0; i < (int)this->entries.size(); i++) {
      if (!this->entries[i].subsumed) {
        if (does_subsume(this->entries[i], e)) {
          return;
        } else if (does_subsume(e, this->entries[i])) {
          this->entries[i].subsumed = true;
        }
      }
    }
    //cout << "entry: " << this->entries.size() << "  "; dump_entry(e);
    this->entries.push_back(e);
  }
}

void Flood::process2(Entry const& a, Entry const& b)
{
  process_impl(a, b); // handles a and b symmetrically

  process_subst_via_implication(a, b);
  process_subst_via_implication(b, a);
}

Entry Flood::make_entry(vector<int> const& t, uint32_t forall, uint32_t exists)
{
  Entry e;
  e.v = t;
  sort_and_remove_dupes(e.v);
  e.forall_mask = forall;
  e.exists_mask = exists;
  e.subsumed = false; 
  return e;
}

void Flood::process_impl(Entry const& a, Entry const& b)
{
  if ((a.forall_mask & b.exists_mask)
   || (a.exists_mask & b.forall_mask)) {
    return;
  }

  if (a.v.size() + b.v.size() <= 2 || (int)a.v.size() + (int)b.v.size() - 2 > max_k) {
    return;
  }
  if (a.v.size() == 2 && are_negations(a.v[0], a.v[1])) {
    return;
  }
  if (b.v.size() == 2 && are_negations(b.v[0], b.v[1])) {
    return;
  }

  for (int i = 0; i < (int)a.v.size(); i++) {
    for (int j = 0; j < (int)b.v.size(); j++) {
      if (are_negations(a.v[i], b.v[j])) {
        vector<int> t;
        for (int k = 0; k < (int)a.v.size(); k++) {
          if (k != i) {
            t.push_back(a.v[k]);
          }
        }
        for (int k = 0; k < (int)b.v.size(); k++) {
          if (k != j) {
            t.push_back(b.v[k]);
          }
        }
        uint32_t mask = get_sort_uses_mask(t);
        add_checking_subsumes(make_entry(t,
            mask & (a.forall_mask | b.forall_mask),
            mask & (a.exists_mask | b.exists_mask)
           ));
      }
    }
  }
}

// can use process_subst_via_implication instead
// which will cover these cases automatically when it operates on `a=b | not(a=b)`
/*void Flood::process_subst_direct(Entry const& b) {
  for (int i = 0; i < (int)b.v.size(); i++) {
    int neg = negation_map[b.v[i]];
    if (neg == -1) continue;
    if (!is_equality[neg]) continue;
    for (int j = 0; j < (int)b.v.size(); j++) {
      if (j != i) {
        for (int new_thing : subst_map[neg][b.v[j]]) {
          if (new_thing == SUBST_VALUE_TRUE) continue;
          vector<int> t = b.v;
          if (new_thing == SUBST_VALUE_FALSE) {
            t.erase(t.begin() + j);
          } else {
            t[j] = new_thing;
          }
          uint32_t mask = get_sort_uses_mask(t);
          add_checking_subsumes(make_entry(t,
              mask & b.forall_mask,
              mask & b.exists_mask
             ));
        }
      }
    }
  }
}*/

int find_in_vec(vector<int> const& v, int w) {
  for (int j = 0; j < (int)v.size(); j++) {
    if (v[j] == w) return j;
  }
  return -1;
}

void Flood::process_subst_via_implication(Entry const& a, Entry const& b) {
  if (a.v.size() != 2) {
    return;
  }
  if (a.exists_mask != 0) {
    return;
  }

  for (int i = 0; i < 2; i++) {
    if (is_equality[a.v[i]]) {
      //int idx = find_in_vec(b.v, a.v[1-i]);
      //if (idx != -1) {
        for (int j = 0; j < (int)b.v.size(); j++) {
          //if (j != idx) {
            for (int new_thing : subst_map[a.v[i]][b.v[j]]) {
              if (new_thing == SUBST_VALUE_TRUE) continue;
              vector<int> t = b.v;

              /*if (new_thing == SUBST_VALUE_FALSE) {
                t.erase(t.begin() + j);
              } else {
                t[j] = new_thing;
              }
              t.push_back(a.v[1-i]);*/
              // slightly faster equivalent version (we sort later):
              t[j] = a.v[1-i];
              if (new_thing != SUBST_VALUE_FALSE) {
                t.push_back(new_thing);
              }

              uint32_t mask = get_sort_uses_mask(t);
              uint32_t emask = mask & (b.exists_mask);
              uint32_t fmask = mask & (a.forall_mask | b.forall_mask) & ~emask;
              add_checking_subsumes(make_entry(t, fmask, emask));
            }
          //}
        }
      //}
    }
  }
}

bool Flood::are_negations(int a, int b) {
  return negation_map[a] == b;
}

uint32_t Flood::get_sort_uses_mask(std::vector<int> const& t)
{
  uint32_t uses = 0;
  for (int x : t) {
    uses |= sort_uses_masks[x]; 
  }
  return uses;
}

bool Flood::in_bounds(Entry const& e)
{
  return (int)e.v.size() <= max_k
      && exists_count(e) <= max_e;
}

void Flood::process(Entry const& e)
{
  process_replace_forall_with_exists(e);
  //process_subst_direct(e);
  process_instantiate_universal(e);
}

void Flood::process_replace_forall_with_exists(Entry const& e)
{
  for (int i = 0; i < nsorts; i++) {
    if ((e.forall_mask >> i) & 1) {
      Entry f;
      f.v = e.v;
      f.forall_mask = e.forall_mask & ~(1 << i);
      f.exists_mask = e.exists_mask | (1 << i);
      f.subsumed = false;
      add_checking_subsumes(f);
    }
  }
}

inline bool bitmask_subset(uint64_t a, uint64_t b) {
  return (a | b) == b;
}

void Flood::process_instantiate_universal(Entry const& e)
{
  uint64_t uses_mask = 0;
  for (int x : e.v) {
    uses_mask |= var_uses_masks[x];
  }

  int var_idx = 0;
  for (int sort_idx = 0; sort_idx < nsorts; sort_idx++) {
    uint64_t allowed_to_this_point = 0;
    for (int j = 0; j <= sort_idx; j++) {
      allowed_to_this_point |= var_of_sort_masks[j];
    }
    for (int j = sort_idx + 1; j < nsorts && !((e.exists_mask) & (1 << j)); j++) {
      allowed_to_this_point |= var_of_sort_masks[j];
    }

    for (int i = 0; i < forall_tspace.vars[sort_idx]; i++, var_idx++) {
      if ((e.forall_mask & (1 << sort_idx)) && (uses_mask & (1 << var_idx))) {
        for (UniversalInstantiationSubstitution const& uis :
            this->universal_instantiation_substitutions[var_idx])
        {
          if (bitmask_subset(uis.new_uses, allowed_to_this_point)) {
            vector<int> t;
            bool stop = false;
            for (int x : e.v) {
              int new_x = uis.mapping[x];
              if (new_x == SUBST_VALUE_TRUE) {
                stop = true;
                break;
              } else if (new_x == SUBST_VALUE_FALSE) {
                // do nothing
              } else {
                t.push_back(new_x);
              }
            }
            if (!stop) {
              bool is_only_usage_of_this_var = (__builtin_popcount(
                  uses_mask & var_of_sort_masks[sort_idx]) == 1);
              uint64_t fmask = (is_only_usage_of_this_var
                    ? (e.forall_mask & ~(1 << sort_idx))
                    : e.forall_mask);

              uint64_t smask = 0;
              for (int x : t) {
                smask |= this->sort_uses_masks[x];
              }
              fmask |= (smask & ~e.exists_mask);

              add_checking_subsumes(make_entry(t, fmask, e.exists_mask));
            }
          }
        }
      }
    }
  }
}

int Flood::exists_count(Entry const& e)
{
  uint64_t uses_mask = 0;
  for (int x : e.v) {
    uses_mask |= var_uses_masks[x];
  }
  uint64_t exists_mask = 0;
  for (int i = 0; i < nsorts; i++) {
    if ((e.exists_mask >> i) & 1) {
      exists_mask |= var_of_sort_masks[i];
    }
  }
  return __builtin_popcount(uses_mask & exists_mask);
}

void Flood::init_negation_map()
{
  int l = (int)clauses.size();
  negation_map.resize(l);
  for (int i = 0; i < l; i++) {
    negation_map[i] = -1;
  }
  for (int i = 0; i < l; i++) {
    value v = clauses[i];
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
    Not* no = dynamic_cast<Not*>(v.get());
    if (no != NULL) {
      int j = get_index_of_piece(no->val);
      if (j != -1) {
        negation_map[j] = i;
        negation_map[i] = j;
      }
    }
  }
}

struct IdenMask {
  uint32_t sort_use_mask;
  uint64_t var_use_mask;
};

void Flood::init_masks()
{
  this->var_of_sort_masks.resize(nsorts);
  for (int i = 0; i < nsorts; i++) {
    this->var_of_sort_masks[i] = 0;
  }

  map<iden, IdenMask> m;

  vector<Alternation> alts = forall_taqd.alternations();
  assert (alts.size() == 1);
  Alternation alt = alts[0];
  int v_idx = 0;
  for (int i = 0; i < (int)module->sorts.size(); i++) {
    lsort so = s_uninterp(module->sorts[i]);
    for (VarDecl const& decl : alt.decls) {
      if (sorts_eq(decl.sort, so)) {
        IdenMask im;
        assert(i < 8 * (int)sizeof(im.sort_use_mask));
        im.sort_use_mask = (1 << i);

        assert(v_idx < 8 * (int)sizeof(im.var_use_mask));
        im.var_use_mask = (1 << v_idx);

        m.insert(make_pair(decl.name, im));

        this->var_of_sort_masks[i] |= (1 << v_idx);

        v_idx++;
      }
    }
  }

  int l = (int)clauses.size();

  sort_uses_masks.resize(l);
  var_uses_masks.resize(l);

  for (int i = 0; i < l; i++) {
    set<iden> used_vars;
    clauses[i]->get_used_vars(used_vars /* output */);

    uint32_t sort_use_mask = 0;
    uint64_t var_use_mask = 0;
    for (iden id : used_vars) {
      auto it = m.find(id);
      assert(it != m.end());
      sort_use_mask |= it->second.sort_use_mask;
      var_use_mask |= it->second.var_use_mask;
    }
    this->sort_uses_masks[i] = sort_use_mask;
    this->var_uses_masks[i] = var_use_mask;
  }
}

void find_all_single_substs(value v, value l, value r, vector<value>& res)
{
  assert(v.get() != NULL);

  if (values_equal(v, l)) {
    res.push_back(r);
    return;
  }

  if (values_equal(v, r)) {
    res.push_back(l);
    return;
  }

  /*if (Forall* f = dynamic_cast<Forall*>(v.get())) {
    vector<value> new_res;
    find_all_single_substs(f->body, l, r, new_res);
    for (value w : new_res) {
      res.push_back(v_forall(f->decls, w));
    }
  }
  else if (Exists* f = dynamic_cast<Exists*>(v.get())) {
    vector<value> new_res;
    find_all_single_substs(f->body, l, r, new_res);
    for (value w : new_res) {
      res.push_back(v_exists(f->decls, w));
    }
  }
  else if (And* f = dynamic_cast<And*>(v.get())) {
    for (int i = 0; i < (int)f->args.size(); i++) {
      vector<value> new_res;
      find_all_single_substs(f->args[i], l, r, new_res);
      for (value w : new_res) {
        value<value> new_args = f->args;
        new_args[i] = w;
        res.push_back(v_and(new_args));
      }
    }
  }
  else if (Or* f = dynamic_cast<And*>(v.get())) {
    for (int i = 0; i < (int)f->args.size(); i++) {
      vector<value> new_res;
      find_all_single_substs(f->args[i], l, r, new_res);
      for (value w : new_res) {
        value<value> new_args = f->args;
        new_args[i] = w;
        res.push_back(v_or(new_args));
      }
    }
  }
  else*/ if (Apply* f = dynamic_cast<Apply*>(v.get())) {
    for (int i = 0; i < (int)f->args.size(); i++) {
      vector<value> new_res;
      find_all_single_substs(f->args[i], l, r, new_res);
      for (value w : new_res) {
        vector<value> new_args = f->args;
        new_args[i] = w;
        res.push_back(v_apply(f->func, new_args));
      }
    }
  }
  else if (dynamic_cast<Var*>(v.get())) {
    // return nothing
  }
  else if (dynamic_cast<Const*>(v.get())) {
    // return nothing
  }
  else if (Eq* f = dynamic_cast<Eq*>(v.get())) {
    for (int i = 0; i < 2; i++) {
      vector<value> new_res;
      find_all_single_substs(i == 0 ? f->left : f->right, l, r, new_res);
      for (value w : new_res) {
        if (i == 0) {
          res.push_back(v_eq(w, f->right));
        } else {
          res.push_back(v_eq(f->left, w));
        }
      }
    }
  }
  else if (Not* f = dynamic_cast<Not*>(v.get())) {
    find_all_single_substs(f->val, l, r, res);
    for (int i = 0; i < (int)res.size(); i++) {
      res[i] = v_not(res[i]);
    }
  }
  else {
    cout << v->to_string() << endl;
    assert(false);
  }
}

bool is_var_ne_nonvar(value v) {
  if (Not* n = dynamic_cast<Not*>(v.get())) {
    if (Eq* eq = dynamic_cast<Eq*>(n->val.get())) {
      bool v1 = (dynamic_cast<Var*>(eq->left.get()) != NULL);
      bool v2 = (dynamic_cast<Var*>(eq->right.get()) != NULL);
      return v1 ^ v2;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

bool is_taut_false(value v);

bool is_taut_true(value v) {
  if (Eq* eq = dynamic_cast<Eq*>(v.get())) {
    return values_equal(eq->left, eq->right);
  } else if (Not* n = dynamic_cast<Not*>(v.get())) {
    return is_taut_false(n->val);
  } else if (Apply* ap = dynamic_cast<Apply*>(v.get())) {
    /*if (Const* c = dynamic_cast<Const*>(ap->func.get())) {
      if (iden_to_string(c->name) == "le"
          && ap->args.size() == 2
          && values_equal(ap->args[0], ap->args[1])) {
        return true;
      }
    }*/
    return false;
  } else {
    return false;
  }
}

bool is_taut_false(value v) {
  if (Not* n = dynamic_cast<Not*>(v.get())) {
    return is_taut_true(n->val);
  } else if (Apply* ap = dynamic_cast<Apply*>(v.get())) {
    /*if (Const* c = dynamic_cast<Const*>(ap->func.get())) {
      if (iden_to_string(c->name) == "btw"
          && ap->args.size() == 3
          && (
               values_equal(ap->args[0], ap->args[1])
            || values_equal(ap->args[1], ap->args[2])
            || values_equal(ap->args[2], ap->args[0])
          )) {
        return true;
      }
    }*/
    return false;
  } else {
    return false;
  }
}

void Flood::init_eq_substs()
{
  is_equality.resize(clauses.size());
  subst_map.resize(clauses.size());
  for (int i = 0; i < (int)clauses.size(); i++) {
    subst_map[i].resize(clauses.size());
    value v = TopAlternatingQuantifierDesc::get_body(clauses[i]);

    Eq* eq = dynamic_cast<Eq*>(v.get());
    if (eq == NULL) {
      is_equality[i] = false;
    } else {
      is_equality[i] = true;
      for (int j = 0; j < (int)clauses.size(); j++) {
        vector<value> results;
        find_all_single_substs(
            TopAlternatingQuantifierDesc::get_body(clauses[j]),
            eq->left,
            eq->right,
            results);
        for (value r1 : results) {
          value r = order_and_or_eq(r1);
          int ri = get_index_of_piece(r);
          if (ri == j) {
            // do nothing
          } else if (ri != -1) {
            subst_map[i][j].push_back(ri);

            //cout << "subst::: "
            //<< TopAlternatingQuantifierDesc::get_body(clauses[i])->to_string()
            //<< "   ---->   "
            //<< TopAlternatingQuantifierDesc::get_body(clauses[j])->to_string()
            //<< "   ---->   "
            //<< TopAlternatingQuantifierDesc::get_body(clauses[ri])->to_string()
            //<< endl;
          } else if (is_taut_true(r)) {
            subst_map[i][j].push_back(SUBST_VALUE_TRUE);
          } else if (is_taut_false(r)) {
            subst_map[i][j].push_back(SUBST_VALUE_FALSE);
          } else if (is_var_ne_nonvar(r)) {
            // do nothing?
          } else {
            cout << r->to_string() << endl;
            assert(false);
          }
        }
        sort_and_remove_dupes(subst_map[i][j]);
      }
    }
  }

  /*cout << "all substs:" << endl;
  for (int i = 0; i < (int)subst_map.size(); i++) {
    for (int j = 0; j < (int)subst_map[i].size(); j++) {
      for (int k = 0; k < (int)subst_map[i][j].size(); k++) {
        cout << "subst::: "
          << TopAlternatingQuantifierDesc::get_body(clauses[i])->to_string()
          << "   ---->   "
          << TopAlternatingQuantifierDesc::get_body(clauses[j])->to_string()
          << "   ---->   ";
        if (subst_map[i][j][k] == SUBST_VALUE_FALSE) cout << "false";
        else if (subst_map[i][j][k] == SUBST_VALUE_TRUE) cout << "true";
        else cout << TopAlternatingQuantifierDesc::get_body(clauses[subst_map[i][j][k]])->to_string();
        cout << endl;
      }
    }
  }
  assert(false);
  */
}

void Flood::add_negations()
{
  assert (negation_map.size() > 0);
  for (int i = 0; i < (int)negation_map.size(); i++) {
    int j = negation_map[i];
    if (j != -1 && j > i) {
      Entry e;
      e.v = {i, j};
      e.forall_mask = sort_uses_masks[i];
      e.exists_mask = 0;
      e.subsumed = false;
      entries.push_back(e);

      if (max_e > 0) {
        e.exists_mask = e.forall_mask;
        e.forall_mask = 0;
        entries.push_back(e);
      }
    }
  }
}

pair<TopAlternatingQuantifierDesc, set<iden>> add_universal_decls(
    TopAlternatingQuantifierDesc const& taqd,
    vector<int> const& sort_counts,
    shared_ptr<Module> module)
{
  vector<Alternation> new_alts;
  int sort_idx = 0;
  vector<Alternation> alts = taqd.alternations();
  set<iden> idens_to_instantiate;

  int name_idx = 0;

  for (int i = 0; i < (int)alts.size(); i++) {
    Alternation const& alt = alts[i];

    if (alt.is_exists()) {
      new_alts.push_back(alt);
    } else {
      assert (alt.is_forall());

      int next_sort_idx;
      next_sort_idx = module->sorts.size();
      if (i < (int)alts.size() - 1) {
        Alternation const& next_alt = alts[i + 1];
        for (VarDecl const& decl : next_alt.decls) {
          next_sort_idx = min(next_sort_idx,
              sort_idx_of_module(module, decl.sort));
        }
      }

      Alternation new_alt;
      new_alt.altType = AltType::Forall;

      for (int si = sort_idx; si < next_sort_idx; si++) {
        lsort so = s_uninterp(module->sorts[si]);
        for (int j = 0; j < sort_counts[si]; j++) {
          string name = to_string(name_idx);
          name_idx++;
          while (name.size() < 4) name = "0" + name;
          name = "AA_inst_" + name;
          new_alt.decls.push_back(VarDecl(string_to_iden(name), so));
        }
      }

      for (VarDecl const& decl : alt.decls) {
        idens_to_instantiate.insert(decl.name);
        new_alt.decls.push_back(decl);
      }

      new_alts.push_back(new_alt);
    }

    for (VarDecl const& decl : alt.decls) {
      sort_idx = max(sort_idx,
          sort_idx_of_module(module, decl.sort) + 1);
    }
  }

  return make_pair(TopAlternatingQuantifierDesc(new_alts), idens_to_instantiate);
}

void inst_universals_with_stuff(
  value v,
  vector<VarDecl> const& decls,
  shared_ptr<Module> const& module,
  set<iden> const& idens_to_instantiate,
  vector<value>& res)
{
  assert(v.get() != NULL);
  if (Forall* val = dynamic_cast<Forall*>(v.get())) {
    if (val->decls.size() == 0) {
      inst_universals_with_stuff(val->body, decls, module, idens_to_instantiate, res);
    } else {
      VarDecl decl = val->decls[0];
      value body = pop_first_quantifier_variable(v);

      if (idens_to_instantiate.count(decl.name) != 0) {
        //cout << "adding bro" << endl;
        for (value subber : gen_clauses_for_sort(
            module,
            decls,
            decl.sort))
        {
          //cout << "adding subber " << subber->to_string() << endl;
          inst_universals_with_stuff(body->subst(decl.name, subber), decls, module, idens_to_instantiate, res);
        }
      } else {
        vector<VarDecl> new_decls = decls;
        new_decls.push_back(decl);
        vector<value> new_r;
        inst_universals_with_stuff(body, new_decls, module, idens_to_instantiate, new_r);
        for (value w : new_r) {
          res.push_back(v_forall({decl}, w));
        }
      }
    }
  }
  else if (Exists* val = dynamic_cast<Exists*>(v.get())) {
    if (val->decls.size() == 0) {
      inst_universals_with_stuff(val->body, decls, module, idens_to_instantiate, res);
    } else {
      vector<VarDecl> new_decls = decls;
      for (VarDecl const& decl : val->decls) {
        new_decls.push_back(decl);
      }
      vector<value> new_r;
      inst_universals_with_stuff(val->body, new_decls, module, idens_to_instantiate, new_r);
      for (value w : new_r) {
        res.push_back(v_exists(val->decls, w));
      }
    }
  }
  else {
    res.push_back(v);
  }
}

void Flood::add_axioms_and_starting_formulas(std::vector<value> const& extras)
{
  for (value v : module->axioms) {
    add_starting_formula(v);
  }
  for (value v : extras) {
    add_starting_formula(v);
  }
}

void Flood::add_starting_formula(value v)
{
  //cout << "axiom adding: " << v->to_string() << endl;
  value w = v->structurally_normalize();
  //cout << "axiom adding (normalized): " << w->to_string() << endl;
  TopAlternatingQuantifierDesc taqd(w);
  auto p = add_universal_decls(taqd, forall_tspace.vars, module);
  value u = p.first.with_body(TopAlternatingQuantifierDesc::get_body(w));

  vector<value> res;
  inst_universals_with_stuff(u, {}, module, p.second, res);

  for (value w : res) {
    //cout << "adding " << w->to_string() << endl;

    // XXX there might be a performance explosion here because inst_universals_with_stuff
    // takes care of all variable permutations and then do_add does it again.
    do_add(w);
  }
}

void Flood::init_universal_instantation_substitutions()
{
  vector<VarDecl> all_decls;
  map<iden, int> m;
  int var_idx = 0;
  for (Alternation const& alt : forall_taqd.alternations()) {
    for (VarDecl const& decl : alt.decls) {
      all_decls.push_back(decl);
      m.insert(make_pair(decl.name, ((uint64_t)1) << var_idx));
      var_idx++;
    }
  }

  var_idx = 0;
  for (int sort_idx = 0; sort_idx < nsorts; sort_idx++) {
    lsort so = s_uninterp(module->sorts[sort_idx]);
    for (int i = 0; i < forall_tspace.vars[sort_idx]; i++, var_idx++) {
      std::vector<UniversalInstantiationSubstitution> uises;

      iden id = all_decls[var_idx].name;
      vector<VarDecl> other_decls = all_decls;
      other_decls.erase(other_decls.begin() + var_idx);

      for (value new_expr : 
        gen_clauses_for_sort(module, other_decls, so))
      {
        uint64_t new_uses = 0;
        set<iden> used_vars;
        new_expr->get_used_vars(used_vars /* output */);
        for (iden id : used_vars) {
          auto it = m.find(id);
          assert(it != m.end());
          new_uses |= it->second;
        }

        assert ((new_uses & (1 << var_idx)) == 0);

        UniversalInstantiationSubstitution uis;
        uis.new_uses = new_uses;
        uis.mapping.resize(clauses.size());
        for (int k = 0; k < (int)clauses.size(); k++) {
          value new_clause = TopAlternatingQuantifierDesc::get_body(clauses[k])
              ->subst(id, new_expr);
          value r = order_and_or_eq(new_clause);
          int ri = get_index_of_piece(r);
          int val;
          if (ri != -1) {
            val = ri;
          } else if (is_taut_true(r)) {
            val = SUBST_VALUE_TRUE;
          } else if (is_taut_false(r)) {
            val = SUBST_VALUE_FALSE;
          } else {
            assert (false);
          }
          uis.mapping[k] = val;
        }

        uises.push_back(uis);
      }

      universal_instantiation_substitutions.push_back(move(uises));
    }
  }
}
