#include "smt.h"

#include <cvc4/cvc4.h>

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "benchmarking.h"
#include "model.h"
#include "stats.h"

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
    int timeout_ms;

    context() : been_set(false), timeout_ms(-1) { }

    void set_timeout(int ms) override {
      timeout_ms = ms;
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
      if (ctx.timeout_ms != -1) {
        smt.setTimeLimit(ctx.timeout_ms);
      }
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

  std::shared_ptr<_solver> context::make_solver() {
    return shared_ptr<_solver>(new solver(*this));
  }

  std::shared_ptr<_expr_vector> context::new_expr_vector() {
    return shared_ptr<_expr_vector>(new expr_vector(*this));
  }

  std::shared_ptr<_sort_vector> context::new_sort_vector() {
    return shared_ptr<_sort_vector>(new sort_vector(*this));
  }
}

namespace smt {
  std::shared_ptr<_context> make_cvc4_context()
  {
    return shared_ptr<_context>(new smt_cvc4::context());
  }
}

////////////////////
///// Model stuff

shared_ptr<Model> Model::extract_cvc4(
  smt::context& smt_ctx,
  smt::solver& smt_solver,
  std::shared_ptr<Module> module,
  ModelEmbedding const& e)
{
  smt_cvc4::context& ctx = *dynamic_cast<smt_cvc4::context*>(smt_ctx.p.get());
  smt_cvc4::solver& solver = *dynamic_cast<smt_cvc4::solver*>(smt_solver.p.get());

  CVC4::ExprManager& em = ctx.em;
  CVC4::SmtEngine& smt = solver.smt;

  // The cvc4 API, at least at the time of writing, does
  // not appear to expose any way of accessing the model.
  // We use a version with some modifications I made to expose
  // the Model interface.
  // Since the Model interface isn't intended to be exposed,
  // it's a little hard to use, and it segfaults if you don't
  // create an SmtScope object (whatever that is), which took
  // me a while to figure out. The SmtScope object *also* isn't
  // exposed, so I also modified the library by adding this
  // SmtScopeContainer object which handles the SmtScope for us.
  //
  // tl;dr
  // Make sure you're using the fork of the cvc4 lib which
  // is checked in to this repo.

  CVC4::Model* cvc4_model = smt.getModel();
  CVC4::SmtScopeContainer smt_scope_container(cvc4_model);

  //std::cout << endl;
  //std::cout << endl;
  //std::cout << endl;
  //std::cout << *cvc4_model << endl;
  //std::cout << endl;
  //std::cout << endl;
  //std::cout << endl;

  map<string, vector<CVC4::Expr>> universes;

  std::unordered_map<std::string, SortInfo> sort_info;
  for (auto p : e.ctx->sorts) {
    string name = p.first;
    //cout << "doing sort " << name << endl;
    CVC4::Type ty = dynamic_cast<smt_cvc4::sort*>(p.second.p.get())->ty;
    vector<CVC4::Expr> exprs = cvc4_model->getDomainElements(ty);
    assert (exprs.size() > 0);
    for (int i = 0; i < (int)exprs.size(); i++) {
      //cout << "e is " << e << endl;
      exprs[i] = cvc4_model->getValue(exprs[i]);
      //cout << "e' is " << cvc4_model->getValue(e) << endl;
    }
    universes.insert(make_pair(name, exprs));
    SortInfo si;
    si.domain_size = exprs.size();
    sort_info.insert(make_pair(name, si));
  }

  std::unordered_map<iden, FunctionInfo> function_info;

  for (VarDecl decl : module->functions) {
    iden name = decl.name;
    //cout << "doing function " << iden_to_string(name) << endl;
    smt::func_decl fd = e.getFunc(name);

    //cout << "doing " << iden_to_string(name) << endl;

    function_info.insert(make_pair(name, FunctionInfo()));
    FunctionInfo& fi = function_info.find(name)->second;
    fi.else_value = 0;

    vector<shared_ptr<Sort>> domain =
        decl.sort->get_domain_as_function();

    shared_ptr<Sort> range =
        decl.sort->get_range_as_function();

    vector<size_t> domain_sizes;
    for (int i = 0; i < (int)domain.size(); i++) {
      if (dynamic_cast<BooleanSort*>(domain[i].get())) {
        domain_sizes.push_back(2);
      } else if (UninterpretedSort* us = dynamic_cast<UninterpretedSort*>(domain[i].get())) {
        auto it = universes.find(us->name);
        assert (it != universes.end());
        domain_sizes.push_back(it->second.size());
      } else {
        assert (false);
      }
      assert(domain_sizes[i] > 0);
    }

    vector<object_value> args;
    args.resize(domain.size());
    for (int i = 0; i < (int)domain.size(); i++) {
      args[i] = 0;
    }

    CVC4::Expr const_true = em.mkConst(true);
    CVC4::Expr const_false = em.mkConst(false);

    while (true) {
      smt_cvc4::expr_vector args_exprs(ctx);
      for (int i = 0; i < (int)domain.size(); i++) {
        if (dynamic_cast<BooleanSort*>(domain[i].get())) {
          args_exprs.ex_vec.push_back(args[i] == 0 ? const_false : const_true);
        } else if (UninterpretedSort* us = dynamic_cast<UninterpretedSort*>(domain[i].get())) {
          auto it = universes.find(us->name);
          assert (it != universes.end());
          assert (0 <= args[i] && args[i] < it->second.size());
          args_exprs.ex_vec.push_back(it->second[args[i]]);
        } else {
          assert (false);
        }
      }

      auto c = dynamic_cast<smt_cvc4::func_decl*>(fd.p.get())->call(&args_exprs);
      CVC4::Expr result_expr =
          cvc4_model->getValue(dynamic_cast<smt_cvc4::expr*>(c.get())->ex);
      object_value result;

      if (dynamic_cast<BooleanSort*>(range.get())) {
        if (result_expr == const_true) {
          result = 1;
        } else if (result_expr == const_false) {
          result = 0;
        } else {
          assert (false);
        }
      } else if (UninterpretedSort* us = dynamic_cast<UninterpretedSort*>(range.get())) {
        auto it = universes.find(us->name);
        assert (it != universes.end());
        vector<CVC4::Expr>& univ = it->second;
        bool found = false;
        for (int j = 0; j < (int)univ.size(); j++) {
          if (univ[j] == result_expr) {
            result = j;
            found = true;
            break;
          }
        }
        //cout << "result_expr " << result_expr << endl;
        assert (found);
      } else {
        assert (false);
      }

      if (fi.table == nullptr) {
        fi.table = unique_ptr<FunctionTable>(new FunctionTable());
        if (0 < domain_sizes.size()) {
          fi.table->children.resize(domain_sizes[0]);
        }
      }
      FunctionTable* ft = fi.table.get();
      for (int i = 0; i < (int)domain.size(); i++) {
        if (ft->children[args[i]] == nullptr) {
          ft->children[args[i]] = unique_ptr<FunctionTable>(new FunctionTable());
          if (i + 1 < (int)domain.size()) {
            ft->children[args[i]]->children.resize(domain_sizes[i+1]);
          }
        }
        ft = ft->children[args[i]].get();
      }
      ft->value = result;

      int i;
      for (i = 0; i < (int)domain.size(); i++) {
        args[i]++;
        if (args[i] == domain_sizes[i]) {
          args[i] = 0;
        } else {
          break;
        }
      }
      if (i == (int)domain.size()) {
        break;
      }
    }
  }

  auto result = shared_ptr<Model>(new Model(module, move(sort_info), move(function_info)));
  //result->dump();
  return result;
}

extern bool enable_smt_logging;

namespace smt_cvc4 {
  //////////////////////
  ///// solving

  string res_to_string(CVC4::Result::Sat res) {
    if (res == CVC4::Result::SAT) {
      return "sat";
    } else if (res == CVC4::Result::UNSAT) {
      return "unsat";
    } else if (res == CVC4::Result::SAT_UNKNOWN) {
      return "timeout/unknown";
    } else {
      assert(false);
    }
  }

  smt::SolverResult solver::check_result()
  {
    //cout << "check_sat" << endl;
    auto t1 = now();
    CVC4::Result::Sat res = smt.checkSat().isSat();
    auto t2 = now();

    long long ms = as_ms(t2 - t1);
    smt::log_to_stdout(ms, true, log_info, res_to_string(res));
    if (enable_smt_logging) {
      log_smtlib(ms, res_to_string(res));
    }

    global_stats.add_cvc4(ms);

    if (res == CVC4::Result::SAT) return smt::SolverResult::Sat;
    else if (res == CVC4::Result::UNSAT) return smt::SolverResult::Unsat;
    return smt::SolverResult::Unknown;
  }

  void solver::dump(ofstream& of) {
    of << "solver::dump not implemented for cvc4" << endl;
  }
}
