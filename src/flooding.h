#ifndef FLOODING_H
#define FLOODING_H

#include "template_counter.h"

struct RedundantDesc {
  std::vector<int> v;
  uint32_t quant_mask; // 1 for exists
};

struct Entry {
  std::vector<int> v;
  uint32_t forall_mask;
  uint32_t exists_mask;
  bool subsumed;
};

//struct Generalization {
//  std::vector<int> atom_to_atom;
//};

class Flood {
  public:
    std::vector<RedundantDesc> get_initial_redundant_descs();
    std::vector<RedundantDesc> add_formula(value v);

    Flood(
        std::shared_ptr<Module> module,
        TemplateSpace const& forall_tspace,
        int max_e);

  private:

    std::shared_ptr<Module> module;
    int nsorts;
    int max_k;
    int max_e;

    // This is all 'forall' even though Flood can account for 'exists' as well.
    TopAlternatingQuantifierDesc forall_taqd;

    std::vector<int> negation_map;
    std::vector<uint32_t> sort_uses_masks;
    std::vector<uint64_t> var_uses_masks;
    std::vector<uint64_t> var_of_sort_masks;

    std::vector<value> clauses;
    std::map<ComparableValue, int> piece_to_index;

    void init_piece_to_index();
    void init_negation_map();
    void init_masks();

    std::vector<Entry> entries;

    std::vector<RedundantDesc> make_results(int start);

    // Results include all variable permutations
    std::vector<Entry> value_to_entries(value v);
    int get_index_of_piece(value p);
    bool get_single_entry_of_value(value inv, std::vector<int>& t);

    void add_negations();
    void add_axioms();

    void do_add(value v);

    void add_checking_subsumes(Entry const& e);
    Entry make_entry(std::vector<int> const& t, uint32_t forall, uint32_t exists);

    void process(Entry const& e);
    void process2(Entry const& e, Entry const& e2);
    void process_impl(Entry const& a, Entry const& b);
    void process_replace_forall_with_exists(Entry const& e);

    bool in_bounds(Entry const& e);
    bool does_subsume(Entry const& e1, Entry const& e2);
    bool are_negations(int a, int b);
    int exists_count(Entry const& e);
    uint32_t get_sort_uses_mask(std::vector<int> const&);
};

#endif
