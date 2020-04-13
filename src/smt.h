#ifndef SMT_H
#define SMT_H

#include <memory>
#include <vector>
#include <string>
#include <fstream>
#include <cassert>

namespace smt {
  struct _sort {
    virtual ~_sort() { }
  };

  struct sort {
    std::shared_ptr<_sort> p;

    sort(std::shared_ptr<_sort> p) : p(p) { }
  };

  struct _expr {
    virtual ~_expr() { }
    virtual std::shared_ptr<_expr> equals(_expr* a) const = 0;
    virtual std::shared_ptr<_expr> not_equals(_expr* a) const = 0;
    virtual std::shared_ptr<_expr> logic_negate() const = 0;
    virtual std::shared_ptr<_expr> logic_or(_expr* a) const = 0;
    virtual std::shared_ptr<_expr> logic_and(_expr* a) const = 0;

    virtual std::shared_ptr<_expr> ite(_expr*, _expr*) const = 0;
    virtual std::shared_ptr<_expr> implies(_expr*) const = 0;
  };

  struct expr {
    std::shared_ptr<_expr> p;
    expr(std::shared_ptr<_expr> p) : p(p) { }

    expr operator==(expr const& a) const { return p->equals(a.p.get()); }
    expr operator!=(expr const& a) const { return p->not_equals(a.p.get()); }
    expr operator!() const { return p->logic_negate(); }
    expr operator||(expr const& a) const { return p->logic_or(a.p.get()); }
    expr operator&&(expr const& a) const { return p->logic_and(a.p.get()); }
  };

  struct expr_vector;
  struct _expr_vector;

  struct sort_vector;
  struct _sort_vector;

  struct solver;
  struct _solver;

  struct _func_decl {
    virtual ~_func_decl() { }
    virtual size_t arity() = 0;
    virtual std::shared_ptr<_sort> domain(int i) = 0;
    virtual std::shared_ptr<_sort> range() = 0;
    virtual std::shared_ptr<_expr> call(_expr_vector* args) = 0;
    virtual std::shared_ptr<_expr> call() = 0;
    virtual std::string get_name() = 0;
    virtual bool eq(_func_decl*) = 0;
  };

  struct func_decl {
    std::shared_ptr<_func_decl> p;
    func_decl(std::shared_ptr<_func_decl> p) : p(p) { }

    size_t arity() { return p->arity(); }
    sort domain(int i) { return p->domain(i); }
    sort range() { return p->range(); }
    inline expr call(expr_vector args);
    inline expr call();
    std::string get_name() { return p->get_name(); }
  };

  enum class Backend {
    z3,
    cvc4
  };

  struct _context {
    virtual ~_context() { }
    virtual void set_timeout(int ms) = 0;
    virtual std::shared_ptr<_sort> bool_sort() = 0;
    virtual std::shared_ptr<_expr> bool_val(bool) = 0;
    virtual std::shared_ptr<_sort> uninterpreted_sort(std::string const& name) = 0;
    virtual std::shared_ptr<_func_decl> function(
        std::string const& name,
        _sort_vector* domain,
        _sort* range) = 0;
    virtual std::shared_ptr<_func_decl> function(
        std::string const& name,
        _sort* range) = 0;
    virtual std::shared_ptr<_expr> var(
        std::string const& name,
        _sort* range) = 0;
    virtual std::shared_ptr<_expr> bound_var(
        std::string const& name,
        _sort* so) = 0;
    virtual std::shared_ptr<_expr_vector> new_expr_vector() = 0;
    virtual std::shared_ptr<_sort_vector> new_sort_vector() = 0;

    virtual std::shared_ptr<_solver> make_solver() = 0;
  };

  std::shared_ptr<_context> make_z3_context();
  std::shared_ptr<_context> make_cvc4_context();

  struct context {
    std::shared_ptr<_context> p;

    context(Backend backend) {
      if (backend == Backend::z3) {
        p = make_z3_context();
      } else {
        p = make_cvc4_context();
      }
    }

    void set_timeout(int ms) {
      p->set_timeout(ms);
    }

    sort bool_sort() { return p->bool_sort(); }
    expr bool_val(bool b) { return p->bool_val(b); }
    sort uninterpreted_sort(std::string const& name) {
      return p->uninterpreted_sort(name);
    }

    inline func_decl function(
        std::string const& name,
        sort_vector domain,
        sort range);

    inline func_decl function(
        std::string const& name,
        sort range);

    inline expr var(
        std::string const& name,
        sort so);

    inline expr bound_var(
        std::string const& name,
        sort so);

    inline solver make_solver();
  };

  struct _sort_vector {
    virtual ~_sort_vector() { }
    virtual void push_back(_sort* s) = 0;
    virtual size_t size() = 0;
    virtual std::shared_ptr<_sort> get_at(int i) = 0;
  };

  struct sort_vector {
    std::shared_ptr<_sort_vector> p;
    sort_vector(context& ctx) : p(ctx.p->new_sort_vector()) { }

    void push_back(sort s) { p->push_back(s.p.get()); }
    size_t size() { return p->size(); }
    sort operator[] (int i) { return p->get_at(i); }
  };

  struct _expr_vector {
    virtual ~_expr_vector() { }
    virtual void push_back(_expr* e) = 0;
    virtual size_t size() = 0;
    virtual std::shared_ptr<_expr> get_at(int i) = 0;

    virtual std::shared_ptr<_expr> forall(_expr* body) = 0;
    virtual std::shared_ptr<_expr> exists(_expr* body) = 0;
    virtual std::shared_ptr<_expr> mk_and() = 0;
    virtual std::shared_ptr<_expr> mk_or() = 0;
  };

  struct expr_vector {
    std::shared_ptr<_expr_vector> p;
    expr_vector(context& ctx) : p(ctx.p->new_expr_vector()) { }

    void push_back(expr s) { p->push_back(s.p.get()); }
    size_t size() { return p->size(); }
    expr operator[] (int i) { return p->get_at(i); }
  };

  enum class SolverResult {
    Sat,
    Unsat,
    Unknown
  };

  struct _solver {
    virtual ~_solver() { }

    std::string log_info;
    void set_log_info(std::string const& s) { log_info = s; }
    void log_smtlib(long long ms, std::string const& res);
    virtual void dump(std::ofstream& of) = 0;

    virtual SolverResult check_result() = 0;
    bool check_sat() {
      SolverResult res = check_result();
      assert (res == SolverResult::Sat || res == SolverResult::Unsat);
      return res == SolverResult::Sat;
    }

    virtual void push() = 0;
    virtual void pop() = 0;
    virtual void add(_expr* e) = 0;
  };

  struct solver {
    std::shared_ptr<_solver> p;
    solver(std::shared_ptr<_solver> p) : p(p) { }

    void set_log_info(std::string const& s) { p->set_log_info(s); }
    
    SolverResult check_result() { return p->check_result(); }
    bool check_sat() { return p->check_sat(); }

    void push() { p->push(); }
    void pop() { p->pop(); }
    void add(expr e) { p->add(e.p.get()); }
  };

  inline expr forall(expr_vector args, expr body) {
    return args.p->forall(body.p.get());
  }

  inline expr exists(expr_vector args, expr body) {
    return args.p->exists(body.p.get());
  }

  inline expr mk_and(expr_vector args) {
    return args.p->mk_and();
  }

  inline expr mk_or(expr_vector args) {
    return args.p->mk_or();
  }

  inline expr ite(expr a, expr b, expr c) {
    return a.p->ite(b.p.get(), c.p.get());
  }

  inline expr implies(expr a, expr b) {
    return a.p->implies(b.p.get());
  }

  inline expr func_decl::call(expr_vector args) {
    return this->p->call(args.p.get());
  }

  inline expr func_decl::call() {
    return this->p->call();
  }

  inline func_decl context::function(
      std::string const& name,
      sort_vector domain,
      sort range)
  {
    return p->function(name, domain.p.get(), range.p.get());
  }

  inline func_decl context::function(
      std::string const& name,
      sort range)
  {
    return p->function(name, range.p.get());
  }

  inline expr context::var(
      std::string const& name,
      sort so)
  {
    return p->var(name, so.p.get());
  }

  inline expr context::bound_var(
      std::string const& name,
      sort so)
  {
    return p->bound_var(name, so.p.get());
  }

  inline bool func_decl_eq(func_decl a, func_decl b) {
    return a.p->eq(b.p.get());
  }

  inline solver context::make_solver() {
    return p->make_solver();
  }

  void log_to_stdout(long long ms, bool is_cvc4,
    std::string const& log_info, std::string const& res);

  bool is_z3_context(context&);
  void dump_smt_stats();
}

#endif
