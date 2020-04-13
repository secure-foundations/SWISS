#include "smt.h"

#include <cvc4/cvc4.h>

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

using smt::_sort;
using smt::_expr;
using smt::_func_decl;
using smt::_solver;
using smt::_context;
using smt::_expr_vector;
using smt::_sort_vector;

namespace smt_cvc4 {

  bool been_set;

  struct sort : public _sort {
    CVC4::Type ty;

    sort(CVC4::Type ty) : ty(ty) { }
  };

  struct expr : public _expr {
    CVC4::Expr ex;

    expr(CVC4::Expr e) : ex(e) { }

    std::shared_ptr<_expr> equals(_expr* _a) const override {
      expr* a = dynamic_cast<expr*>(_a);
      assert(a != NULL);
      return shared_ptr<_expr>(new expr(ex.eqExpr(a->ex)));
    }

    std::shared_ptr<_expr> not_equals(_expr* _a) const override {
      expr* a = dynamic_cast<expr*>(_a);
      assert(a != NULL);
      return shared_ptr<_expr>(new expr(ex.eqExpr(a->ex).notExpr()));
    }

    std::shared_ptr<_expr> logic_negate() const override {
      return shared_ptr<_expr>(new expr(ex.notExpr()));
    }

    std::shared_ptr<_expr> logic_or(_expr* _a) const override {
      expr* a = dynamic_cast<expr*>(_a);
      assert(a != NULL);
      return shared_ptr<_expr>(new expr(ex.orExpr(a->ex)));
    }

    std::shared_ptr<_expr> logic_and(_expr* _a) const override {
      expr* a = dynamic_cast<expr*>(_a);
      assert(a != NULL);
      return shared_ptr<_expr>(new expr(ex.andExpr(a->ex)));
    }

    std::shared_ptr<_expr> ite(_expr* _b, _expr* _c) const override {
      expr* b = dynamic_cast<expr*>(_b);
      assert(b != NULL);
      expr* c = dynamic_cast<expr*>(_c);
      assert(c != NULL);
          
      CVC4::ExprManager* em = ex.getExprManager();
      return shared_ptr<_expr>(new expr(
          em->mkExpr(CVC4::kind::ITE, ex, b->ex, c->ex)
      ));
    }

    std::shared_ptr<_expr> implies(_expr* _b) const override {
      expr* b = dynamic_cast<expr*>(_b);
      assert(b != NULL);

      CVC4::ExprManager* em = ex.getExprManager();
      return shared_ptr<_expr>(new expr(
          em->mkExpr(CVC4::kind::IMPLIES, ex, b->ex)
      ));
    }
  };

  struct func_decl : _func_decl {
    CVC4::Expr ex;
    CVC4::FunctionType ft;
    bool is_nullary;

    func_decl(CVC4::Expr ex, CVC4::FunctionType ft, bool is_nullary)
        : ex(ex), ft(ft), is_nullary(is_nullary) { }

    size_t arity() override {
      return ft.getArity();
    }

    shared_ptr<_sort> domain(int i) override {
      return shared_ptr<_sort>(new sort(ft.getArgTypes()[i]));
    }
    shared_ptr<_sort> range() override {
      return shared_ptr<_sort>(new sort(ft.getRangeType()));
    }
    shared_ptr<_expr> call(_expr_vector* args) override;
    shared_ptr<_expr> call() override;
    std::string get_name() override { return "func_decl::get_name unimplemented"; }
    bool eq(_func_decl* _b) override {
      func_decl* b = dynamic_cast<func_decl*>(_b);
      assert (b != NULL);
      return ex == b->ex;
    }
  };

  struct context : _context {
    CVC4::ExprManager em;
    bool been_set;

    context() : been_set(false) { }

    void set_timeout(int ms) override {
      //ctx.set("timeout", ms);
      //assert (false && "set_timeout not implemented");
    }

    shared_ptr<_sort> bool_sort() override {
      return shared_ptr<_sort>(new sort(em.booleanType()));
    }

    shared_ptr<_expr> bool_val(bool b) override {
      return shared_ptr<_expr>(new expr(em.mkConst(b)));
    }

    shared_ptr<_sort> uninterpreted_sort(std::string const& name) override {
      return shared_ptr<_sort>(new sort(em.mkSort(name)));
    }

    std::shared_ptr<_func_decl> function(
        std::string const& name,
        _sort_vector* domain,
        _sort* range) override;
    std::shared_ptr<_func_decl> function(
        std::string const& name,
        _sort* range) override;
    std::shared_ptr<_expr> var(
        std::string const& name,
        _sort* range) override;
    std::shared_ptr<_expr> bound_var(
        std::string const& name,
        _sort* so) override;
    std::shared_ptr<_expr_vector> new_expr_vector() override;
    std::shared_ptr<_sort_vector> new_sort_vector() override;
    std::shared_ptr<_solver> make_solver() override;
  };

  struct sort_vector : _sort_vector {
    std::vector<CVC4::Type> so_vec;
    sort_vector(context& ctx) { }

    void push_back(_sort* _s) override {
      sort* s = dynamic_cast<sort*>(_s);
      assert (s != NULL);
      so_vec.push_back(s->ty);
    }

    size_t size() override {
      return so_vec.size();
    }

    std::shared_ptr<_sort> get_at(int i) override {
      return shared_ptr<_sort>(new sort(so_vec[i]));
    }
  };

  struct expr_vector : _expr_vector {
    std::vector<CVC4::Expr> ex_vec;
    CVC4::ExprManager* em;
    expr_vector(context& ctx) : em(&ctx.em) { }

    void push_back(_expr* _s) override {
      expr* s = dynamic_cast<expr*>(_s);
      assert (s != NULL);
      ex_vec.push_back(s->ex);
    }

    size_t size() override {
      return ex_vec.size();
    }

    std::shared_ptr<_expr> get_at(int i) override {
      return shared_ptr<_expr>(new expr(ex_vec[i]));
    }

    std::shared_ptr<_expr> forall(_expr* _body) override {
      expr* body = dynamic_cast<expr*>(_body);
      assert (body != NULL);

      CVC4::Expr var_list = em->mkExpr(
          CVC4::kind::BOUND_VAR_LIST, ex_vec);
      return shared_ptr<_expr>(new expr(
        em->mkExpr(CVC4::kind::FORALL, var_list, body->ex)
      ));
    }
    std::shared_ptr<_expr> exists(_expr* _body) override {
      expr* body = dynamic_cast<expr*>(_body);
      assert (body != NULL);

      CVC4::Expr var_list = em->mkExpr(
          CVC4::kind::BOUND_VAR_LIST, ex_vec);
      return shared_ptr<_expr>(new expr(
        em->mkExpr(CVC4::kind::EXISTS, var_list, body->ex)
      ));
    }
    std::shared_ptr<_expr> mk_and() override {
      if (size() == 0) {
        return shared_ptr<_expr>(new expr(
          em->mkConst(true)
        ));
      } else if (size() == 1) {
        return shared_ptr<_expr>(new expr(
          ex_vec[0]
        ));
      } else {
        return shared_ptr<_expr>(new expr(
          em->mkExpr(CVC4::kind::AND, ex_vec)
        ));
      }
    }
    std::shared_ptr<_expr> mk_or() override {
      if (size() == 0) {
        return shared_ptr<_expr>(new expr(
          em->mkConst(false)
        ));
      } else if (size() == 1) {
        return shared_ptr<_expr>(new expr(
          ex_vec[0]
        ));
      } else {
        return shared_ptr<_expr>(new expr(
          em->mkExpr(CVC4::kind::OR, ex_vec)
        ));
      }
    }
  };

  struct solver : _solver {
    CVC4::SmtEngine smt;
    solver(context& ctx) : smt(&ctx.em) {
      if (!ctx.been_set) {
        smt.setOption("produce-models", true);
        ctx.been_set = true;
      }
      smt.setOption("finite-model-find", true);
    }

    //void enable_models() {
      //smt.setOption("produce-models", true);
    //}

    smt::SolverResult check_result() override;

    void push() override { smt.push(); }
    void pop() override { smt.pop(); }
    void add(_expr* _e) override {
      expr* e = dynamic_cast<expr*>(_e);
      assert (e != NULL);
      smt.assertFormula(e->ex);
    }

    void dump(ofstream&) override;
  };

  shared_ptr<_expr> func_decl::call(_expr_vector* _args) {
    expr_vector* args = dynamic_cast<expr_vector*>(_args);
    assert (args != NULL);

    assert (args->size() == ft.getArgTypes().size());
    if (args->size() == 0) {
      return call();
    } else {
      assert (!is_nullary);
      CVC4::ExprManager* em = args->em;
      return shared_ptr<_expr>(new expr(
        em->mkExpr(CVC4::kind::APPLY_UF, this->ex, args->ex_vec)
      ));
    }
  }

  shared_ptr<_expr> func_decl::call() {
    assert (is_nullary);
    return shared_ptr<_expr>(new expr(this->ex));
  }

  shared_ptr<_func_decl> context::function(
      std::string const& name,
      _sort_vector* _domain,
      _sort* _range)
  {
    if (_domain->size() == 0) {
      return function(name, _range);
    } else {
      sort_vector* domain = dynamic_cast<sort_vector*>(_domain);
      assert (domain != NULL);
      sort* range = dynamic_cast<sort*>(_range);
      assert (range != NULL);

      CVC4::FunctionType ft = em.mkFunctionType(domain->so_vec, range->ty);
      return shared_ptr<_func_decl>(new func_decl(em.mkVar(name, ft), ft, false));
    }
  }

  shared_ptr<_func_decl> context::function(
      std::string const& name,
      _sort* _range)
  {
    sort* range = dynamic_cast<sort*>(_range);
    assert (range != NULL);

    std::vector<CVC4::Type> vec;
    CVC4::FunctionType ft = em.mkFunctionType(vec, range->ty);
    return shared_ptr<_func_decl>(
      new func_decl(em.mkVar(name, range->ty), ft, true)
    );
  }

  shared_ptr<_expr> context::var(
      std::string const& name,
      _sort* _so)
  {
    sort* so = dynamic_cast<sort*>(_so);
    assert (so != NULL);
    return shared_ptr<_expr>(new expr(em.mkVar(name, so->ty)));
  }

  shared_ptr<_expr> context::bound_var(
      std::string const& name,
      _sort* _so)
  {
    sort* so = dynamic_cast<sort*>(_so);
    assert (so != NULL);
    return shared_ptr<_expr>(new expr(em.mkBoundVar(name, so->ty)));
  }
}
