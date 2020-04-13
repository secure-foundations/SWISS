#include "smt.h"

#include "z3++.h"

#include "benchmarking.h"

using namespace std;

using smt::_sort;
using smt::_expr;
using smt::_func_decl;
using smt::_solver;
using smt::_context;
using smt::_expr_vector;
using smt::_sort_vector;

namespace smt_z3 {

  struct sort : public _sort {
    z3::sort so;

    sort(z3::sort s) : so(s) { }
  };

  struct expr : public _expr {
    z3::expr ex;

    expr(z3::expr e) : ex(e) { }

    std::shared_ptr<_expr> equals(_expr* _a) const override {
      expr* a = dynamic_cast<expr*>(_a);
      assert(a != NULL);
      return shared_ptr<_expr>(new expr(ex == a->ex));
    }

    std::shared_ptr<_expr> not_equals(_expr* _a) const override {
      expr* a = dynamic_cast<expr*>(_a);
      assert(a != NULL);
      return shared_ptr<_expr>(new expr(ex != a->ex));
    }

    std::shared_ptr<_expr> logic_negate() const override {
      return shared_ptr<_expr>(new expr(!ex));
    }

    std::shared_ptr<_expr> logic_or(_expr* _a) const override {
      expr* a = dynamic_cast<expr*>(_a);
      assert(a != NULL);
      return shared_ptr<_expr>(new expr(ex || a->ex));
    }

    std::shared_ptr<_expr> logic_and(_expr* _a) const override {
      expr* a = dynamic_cast<expr*>(_a);
      assert(a != NULL);
      return shared_ptr<_expr>(new expr(ex && a->ex));
    }

    std::shared_ptr<_expr> ite(_expr* _b, _expr* _c) const override {
      expr* b = dynamic_cast<expr*>(_b);
      assert(b != NULL);
      expr* c = dynamic_cast<expr*>(_c);
      assert(c != NULL);
      return shared_ptr<_expr>(new expr(z3::ite(ex, b->ex, c->ex)));
    }

    std::shared_ptr<_expr> implies(_expr* _b) const override {
      expr* b = dynamic_cast<expr*>(_b);
      assert(b != NULL);
      return shared_ptr<_expr>(new expr(z3::implies(ex, b->ex)));
    }
  };

  struct func_decl : _func_decl {
    z3::func_decl fd;
    func_decl(z3::func_decl fd) : fd(fd) { }

    size_t arity() override {
      return fd.arity();
    }
    std::shared_ptr<_sort> domain(int i) override {
      return shared_ptr<_sort>(new sort(fd.domain(i)));
    }
    std::shared_ptr<_sort> range() override {
      return shared_ptr<_sort>(new sort(fd.range()));
    }
    std::shared_ptr<_expr> call(_expr_vector* args) override;
    std::shared_ptr<_expr> call() override;
    std::string get_name() override {
      return fd.name().str();
    }
    bool eq(_func_decl* _b) override {
      func_decl* b = dynamic_cast<func_decl*>(_b);
      assert (b != NULL);
      return fd.name() == b->fd.name();
    }
  };

  struct context : _context {
    z3::context ctx;

    void set_timeout(int ms) override {
      ctx.set("timeout", ms);
    }
    std::shared_ptr<_sort> bool_sort() override {
      return shared_ptr<_sort>(new sort(ctx.bool_sort()));
    }
    std::shared_ptr<_expr> bool_val(bool b) override {
      return shared_ptr<_expr>(new expr(ctx.bool_val(b)));
    }
    std::shared_ptr<_sort> uninterpreted_sort(std::string const& name) override {
      return shared_ptr<_sort>(new sort(ctx.uninterpreted_sort(name.c_str())));
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
    z3::sort_vector so_vec;
    sort_vector(context& ctx) : so_vec(ctx.ctx) { }

    void push_back(_sort* _s) override {
      sort* s = dynamic_cast<sort*>(_s);
      assert (s != NULL);
      so_vec.push_back(s->so);
    }

    size_t size() override {
      return so_vec.size();
    }

    std::shared_ptr<_sort> get_at(int i) override {
      return shared_ptr<_sort>(new sort(so_vec[i]));
    }
  };

  struct expr_vector : _expr_vector {
    z3::expr_vector ex_vec;
    expr_vector(context& ctx) : ex_vec(ctx.ctx) { }

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
      return shared_ptr<_expr>(new expr(z3::forall(ex_vec, body->ex)));
    }
    std::shared_ptr<_expr> exists(_expr* _body) override {
      expr* body = dynamic_cast<expr*>(_body);
      assert (body != NULL);
      return shared_ptr<_expr>(new expr(z3::exists(ex_vec, body->ex)));
    }
    std::shared_ptr<_expr> mk_and() override {
      return shared_ptr<_expr>(new expr(z3::mk_and(ex_vec)));
    }
    std::shared_ptr<_expr> mk_or() override {
      return shared_ptr<_expr>(new expr(z3::mk_and(ex_vec)));
    }
  };

  struct solver : _solver {
    z3::solver z3_solver;
    solver(context& ctx) : z3_solver(ctx.ctx) { }

    smt::SolverResult check_result() override;

    void push() override { z3_solver.push(); }
    void pop() override { z3_solver.pop(); }
    void add(_expr* _e) override {
      expr* e = dynamic_cast<expr*>(_e);
      assert (e != NULL);
      z3_solver.add(e->ex);
    }

    void dump(ofstream&) override;
  };

  std::shared_ptr<_expr> func_decl::call(_expr_vector* _args) {
    expr_vector* args = dynamic_cast<expr_vector*>(_args);
    assert (args != NULL);
    return shared_ptr<_expr>(new expr(fd(args->ex_vec)));
  }

  std::shared_ptr<_expr> func_decl::call() {
    return shared_ptr<_expr>(new expr(fd()));
  }

  std::shared_ptr<_func_decl> context::function(
      std::string const& name,
      _sort_vector* _domain,
      _sort* _range)
  {
    sort_vector* domain = dynamic_cast<sort_vector*>(_domain);
    assert (domain != NULL);
    sort* range = dynamic_cast<sort*>(_range);
    assert (range != NULL);
    return shared_ptr<_func_decl>(new func_decl(
      ctx.function(name.c_str(), domain->so_vec, range->so)));
  }

  std::shared_ptr<_func_decl> context::function(
      std::string const& name,
      _sort* _range)
  {
    sort* range = dynamic_cast<sort*>(_range);
    assert (range != NULL);
    return shared_ptr<_func_decl>(new func_decl(
      ctx.function(name.c_str(), 0, 0, range->so)));
  }

  std::shared_ptr<_expr> context::var(
      std::string const& name,
      _sort* _so)
  {
    sort* so = dynamic_cast<sort*>(_so);
    assert (so != NULL);
    return shared_ptr<_expr>(new expr(
      ctx.constant(name.c_str(), so->so)));
  }

  std::shared_ptr<_expr> context::bound_var(
      std::string const& name,
      _sort* _so)
  {
    sort* so = dynamic_cast<sort*>(_so);
    assert (so != NULL);
    return shared_ptr<_expr>(new expr(
      ctx.constant(name.c_str(), so->so)));
  }

  string res_to_string(z3::check_result res) {
    if (res == z3::sat) {
      return "sat";
    } else if (res == z3::unsat) {
      return "unsat";
    } else if (res == z3::unknown) {
      return "timeout/unknown";
    } else {
      assert(false);
    }
  }

  extern bool enable_smt_logging;

  smt::SolverResult solver::check_result()
  {
    auto t1 = now();
    z3::check_result res = z3_solver.check();
    auto t2 = now();

    long long ms = as_ms(t2 - t1);
    smt::log_to_stdout(ms, false, log_info, res_to_string(res));
    if (enable_smt_logging) {
      log_smtlib(ms, res_to_string(res));
    }

    if (res == z3::sat) return smt::SolverResult::Sat;
    else if (res == z3::unsat) return smt::SolverResult::Unsat;
    return smt::SolverResult::Unknown;
  }

  void solver::dump(ofstream& of) {
    of << z3_solver << endl;
  }

}
