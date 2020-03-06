#ifndef SMT_CVC4_H
#define SMT_CVC4_H

#include <cvc4/cvc4.h>

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace smt {

  extern bool been_set;

  struct sort {
    CVC4::Type ty;

    sort(CVC4::Type ty) : ty(ty) { }
  };

  struct expr {
    CVC4::Expr ex;

    expr(CVC4::Expr e) : ex(e) { }

    expr operator==(expr const& a) const { return ex.eqExpr(a.ex); }
    expr operator!=(expr const& a) const { return ex.eqExpr(a.ex).notExpr(); }
    expr operator!() const { return ex.notExpr(); }
    expr operator||(expr const& a) const { return ex.orExpr(a.ex); }
    expr operator&&(expr const& a) const { return ex.andExpr(a.ex); }
  };

  struct expr_vector;
  struct sort_vector;

  struct func_decl {
    CVC4::Expr ex;
    CVC4::FunctionType ft;
    bool is_nullary;

    func_decl(CVC4::Expr ex, CVC4::FunctionType ft, bool is_nullary)
        : ex(ex), ft(ft), is_nullary(is_nullary) { }

    size_t arity() {
      return ft.getArity();
    }

    sort domain(int i) { return sort(ft.getArgTypes()[i]); }
    sort range() { return sort(ft.getRangeType()); }
    inline expr call(expr_vector args);
    inline expr call();
    std::string get_name() { return "func_decl::get_name unimplemented"; }
  };

  struct context {
    CVC4::ExprManager em;
    bool been_set;

    context() : been_set(false) { }

    void set_timeout(int ms) {
      //ctx.set("timeout", ms);
      //assert (false && "set_timeout not implemented");
    }

    sort bool_sort() {
      return sort(em.booleanType());
    }

    expr bool_val(bool b) {
      return em.mkConst(b);
    }

    sort uninterpreted_sort(std::string const& name) {
      return em.mkSort(name);
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
    std::vector<CVC4::Type> so_vec;
    sort_vector(context& ctx) { }
    void push_back(sort s) { so_vec.push_back(s.ty); }
    size_t size() { return so_vec.size(); }
    sort operator[] (int i) { return so_vec[i]; }
  };

  struct expr_vector {
    std::vector<CVC4::Expr> ex_vec;
    CVC4::ExprManager* em;

    expr_vector(context& ctx) : em(&ctx.em) { }
    void push_back(expr e) { ex_vec.push_back(e.ex); }
    size_t size() { return ex_vec.size(); }
    expr operator[] (int i) { return ex_vec[i]; }
  };

  struct solver {
    CVC4::SmtEngine smt;
    solver(context& ctx) : smt(&ctx.em) {
      if (!ctx.been_set) {
        smt.setOption("produce-models", true);
        ctx.been_set = true;
      }
      smt.setOption("finite-model-find", true);
    }

    void enable_models() {
      //smt.setOption("produce-models", true);
    }

    std::string log_info;
    void set_log_info(std::string const& s) { log_info = s; }

    bool check_sat();
    bool is_sat_or_unknown();
    bool is_unsat_or_unknown();

    void push() { smt.push(); }
    void pop() { smt.pop(); }
    void add(expr e) { smt.assertFormula(e.ex); }

    //void log_smtlib(
    //    long long ms,
    //    z3::check_result res);
  };

  inline expr forall(expr_vector args, expr body) {
    CVC4::ExprManager* em = body.ex.getExprManager();
    CVC4::Expr var_list = em->mkExpr(
        CVC4::kind::BOUND_VAR_LIST, args.ex_vec);
    return em->mkExpr(CVC4::kind::FORALL, var_list, body.ex);
  }

  inline expr exists(expr_vector args, expr body) {
    CVC4::ExprManager* em = body.ex.getExprManager();
    CVC4::Expr var_list = em->mkExpr(
        CVC4::kind::BOUND_VAR_LIST, args.ex_vec);
    return em->mkExpr(CVC4::kind::EXISTS, var_list, body.ex);
  }

  inline expr mk_and(expr_vector args) {
    CVC4::ExprManager* em = args.em;
    if (args.size() == 0) {
      return em->mkConst(true);
    } else if (args.size() == 1) {
      return args[0];
    } else {
      return em->mkExpr(CVC4::kind::AND, args.ex_vec);
    }
  }

  inline expr mk_or(expr_vector args) {
    CVC4::ExprManager* em = args.em;
    if (args.size() == 0) {
      return em->mkConst(false);
    } else if (args.size() == 1) {
      return args[0];
    } else {
      return em->mkExpr(CVC4::kind::OR, args.ex_vec);
    }
  }

  inline expr ite(expr a, expr b, expr c) {
    CVC4::ExprManager* em = a.ex.getExprManager();
    return em->mkExpr(CVC4::kind::ITE, a.ex, b.ex, c.ex);
  }

  inline expr implies(expr a, expr b) {
    CVC4::ExprManager* em = a.ex.getExprManager();
    return em->mkExpr(CVC4::kind::IMPLIES, a.ex, b.ex);
  }

  inline expr func_decl::call(expr_vector args) {
    assert (args.size() == ft.getArgTypes().size());
    if (args.size() == 0) {
      return call();
    } else {
      assert (!is_nullary);
      CVC4::ExprManager* em = args.em;
      return em->mkExpr(CVC4::kind::APPLY_UF, this->ex, args.ex_vec);
    }
  }

  inline expr func_decl::call() {
    assert (is_nullary);
    return this->ex;
  }

  inline func_decl context::function(
      std::string const& name,
      sort_vector domain,
      sort range)
  {
    if (domain.size() == 0) {
      return function(name, range);
    } else {
      CVC4::FunctionType ft = em.mkFunctionType(domain.so_vec, range.ty);
      return func_decl(em.mkVar(name, ft), ft, false);
    }
  }

  inline func_decl context::function(
      std::string const& name,
      sort range)
  {
    std::vector<CVC4::Type> vec;
    CVC4::FunctionType ft = em.mkFunctionType(vec, range.ty);
    return func_decl(em.mkVar(name, range.ty), ft, true);
  }

  inline expr context::var(
      std::string const& name,
      sort so)
  {
    return expr(em.mkVar(name, so.ty));
  }

  inline expr context::bound_var(
      std::string const& name,
      sort so)
  {
    return expr(em.mkBoundVar(name, so.ty));
  }

  inline bool func_decl_eq(func_decl a, func_decl b) {
    return a.ex == b.ex;
  }
}

#endif
