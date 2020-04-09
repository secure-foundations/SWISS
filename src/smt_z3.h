#ifndef SMT_Z3_H
#define SMT_Z3_H

#include "z3++.h"

#include <cassert>
#include <string>

namespace smt {

  struct sort {
    z3::sort so;

    sort(z3::sort s) : so(s) { }
  };

  struct expr {
    z3::expr ex;

    expr(z3::expr e) : ex(e) { }

    expr operator==(expr const& a) const { return expr(ex == a.ex); }
    expr operator!=(expr const& a) const { return expr(ex != a.ex); }
    expr operator!() const { return expr(!ex); }
    expr operator||(expr const& a) const { return expr(ex || a.ex); }
    expr operator&&(expr const& a) const { return expr(ex && a.ex); }
  };

  struct expr_vector;
  struct sort_vector;

  struct func_decl {
    z3::func_decl fd;

    func_decl(z3::func_decl fd) : fd(fd) { }
    size_t arity() { return fd.arity(); }
    sort domain(int i) { return sort(fd.domain(i)); }
    sort range() { return sort(fd.range()); }
    inline expr call(expr_vector args);
    inline expr call();
    std::string get_name() { return fd.name().str(); }
  };

  struct context {
    z3::context ctx;

    void set_timeout(int ms) {
      ctx.set("timeout", ms);
    }

    sort bool_sort() {
      return sort(ctx.bool_sort());
    }

    expr bool_val(bool b) {
      return ctx.bool_val(b);
    }

    sort uninterpreted_sort(std::string const& name) {
      return ctx.uninterpreted_sort(name.c_str());
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
  };

  struct sort_vector {
    z3::sort_vector so_vec;
    sort_vector(context& ctx) : so_vec(ctx.ctx) { }
    void push_back(sort s) { so_vec.push_back(s.so); }
    size_t size() { return so_vec.size(); }
    sort operator[] (int i) { return so_vec[i]; }
  };

  struct expr_vector {
    z3::expr_vector ex_vec;
    expr_vector(context& ctx) : ex_vec(ctx.ctx) { }
    void push_back(expr e) { ex_vec.push_back(e.ex); }
    size_t size() { return ex_vec.size(); }
    expr operator[] (int i) { return ex_vec[i]; }
  };

  enum class SolverResult {
    Sat,
    Unsat,
    Unknown
  };

  struct solver {
    z3::solver z3_solver;
    solver(context& ctx) : z3_solver(ctx.ctx) { }

    std::string log_info;
    void set_log_info(std::string const& s) { log_info = s; }

    SolverResult check_result();
    bool check_sat();
    bool is_sat_or_unknown();
    bool is_unsat_or_unknown();

    void push() { z3_solver.push(); }
    void pop() { z3_solver.pop(); }
    void add(expr e) { z3_solver.add(e.ex); }

    void log_smtlib(
        long long ms,
        std::string const& res);
  };

  inline expr forall(expr_vector args, expr body) {
    return z3::forall(args.ex_vec, body.ex);
  }

  inline expr exists(expr_vector args, expr body) {
    return z3::exists(args.ex_vec, body.ex);
  }

  inline expr mk_and(expr_vector args) {
    return expr(z3::mk_and(args.ex_vec));
  }

  inline expr mk_or(expr_vector args) {
    return expr(z3::mk_or(args.ex_vec));
  }

  inline expr ite(expr a, expr b, expr c) {
    return expr(z3::ite(a.ex, b.ex, c.ex));
  }

  inline expr implies(expr a, expr b) {
    return expr(z3::implies(a.ex, b.ex));
  }

  inline expr func_decl::call(expr_vector args) {
    return expr(fd(args.ex_vec));
  }

  inline expr func_decl::call() {
    return expr(fd());
  }

  inline func_decl context::function(
      std::string const& name,
      sort_vector domain,
      sort range)
  {
    return ctx.function(name.c_str(), domain.so_vec, range.so);
  }

  inline func_decl context::function(
      std::string const& name,
      sort range)
  {
    return ctx.function(name.c_str(), 0, 0, range.so);
  }

  inline expr context::var(
      std::string const& name,
      sort so)
  {
    return expr(ctx.constant(name.c_str(), so.so));
  }

  inline expr context::bound_var(
      std::string const& name,
      sort so)
  {
    return expr(ctx.constant(name.c_str(), so.so));
  }

  inline bool func_decl_eq(func_decl a, func_decl b) {
    return a.fd.name() == b.fd.name();
  }
}

#endif
