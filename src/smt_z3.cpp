#include "smt.h"

#include "z3++.h"

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
      return shared_ptr<_expr>(new expr(z3::mk_or(ex_vec)));
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
      //cout << "adding " << e->ex << endl;
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
  bool is_z3_context(smt::context& ctx)
  {
    return dynamic_cast<smt_z3::context*>(ctx.p.get()) != NULL;
  }

  std::shared_ptr<_context> make_z3_context()
  {
    return shared_ptr<_context>(new smt_z3::context());
  }
}

////////////////////
///// Model stuff

shared_ptr<Model> Model::extract_z3(
    smt::context& smt_ctx,
    smt::solver& smt_solver,
    std::shared_ptr<Module> module,
    ModelEmbedding const& e)
{
  smt_z3::context& ctx = *dynamic_cast<smt_z3::context*>(smt_ctx.p.get());
  smt_z3::solver& solver = *dynamic_cast<smt_z3::solver*>(smt_solver.p.get());

  std::unordered_map<std::string, SortInfo> sort_info;
  std::unordered_map<iden, FunctionInfo> function_info;

  z3::model z3model = solver.z3_solver.get_model();

  map<string, z3::expr_vector> universes;

  for (auto p : e.ctx->sorts) {
    string name = p.first;
    z3::sort s = dynamic_cast<smt_z3::sort*>(p.second.p.get())->so;

    // The C++ api doesn't seem to have the functionality we need.
    // Go down to the C API.
    Z3_ast_vector c_univ = Z3_model_get_sort_universe(ctx.ctx, z3model, s);
    int len;
    if (c_univ) {
      z3::expr_vector univ(ctx.ctx, c_univ);
      universes.insert(make_pair(name, univ));
      len = univ.size();
    } else {
      z3::expr_vector univ(ctx.ctx);
      univ.push_back(ctx.ctx.constant(::name("whatever").c_str(), s));
      universes.insert(make_pair(name, univ));
      len = univ.size();
    }

    SortInfo sinfo;
    sinfo.domain_size = len;
    sort_info[name] = sinfo;
  }

  auto get_value = [&z3model, &ctx, &universes](
        Sort* sort, z3::expr expression1) -> object_value {
    z3::expr expression = z3model.eval(expression1, true);
    if (dynamic_cast<BooleanSort*>(sort)) {
      if (z3::eq(expression, ctx.ctx.bool_val(true))) {
        return 1;
      } else if (z3::eq(expression, ctx.ctx.bool_val(false))) {
        return 0;
      } else {
        assert(false);
      }
    } else if (UninterpretedSort* usort = dynamic_cast<UninterpretedSort*>(sort)) {
      auto iter = universes.find(usort->name);
      assert(iter != universes.end());
      z3::expr_vector& vec = iter->second;
      for (object_value i = 0; i < vec.size(); i++) {
        if (z3::eq(expression, vec[i])) {
          return i;
        }
      }
      assert(false);
    } else {
      assert(false && "expected boolean sort or uninterpreted sort");
    }
  };

  auto get_expr = [&ctx, &universes](
        Sort* sort, object_value v) -> z3::expr {
    if (dynamic_cast<BooleanSort*>(sort)) {
      if (v == 0) {
        return ctx.ctx.bool_val(false);
      } else if (v == 1) {
        return ctx.ctx.bool_val(true);
      } else {
        assert(false);
      }
    } else if (UninterpretedSort* usort = dynamic_cast<UninterpretedSort*>(sort)) {
      auto iter = universes.find(usort->name);
      assert(iter != universes.end());
      z3::expr_vector& vec = iter->second;
      assert (0 <= v && v < vec.size());
      return vec[v];
    } else {
      assert(false && "expected boolean sort or uninterpreted sort");
    }
  };

  for (VarDecl decl : module->functions) {
    iden name = decl.name;
    z3::func_decl fdecl = dynamic_cast<smt_z3::func_decl*>(e.getFunc(name).p.get())->fd;

    int num_args;
    Sort* range_sort;
    vector<Sort*> domain_sorts;
    vector<int> domain_sort_sizes;
    if (FunctionSort* functionSort = dynamic_cast<FunctionSort*>(decl.sort.get())) {
      num_args = functionSort->domain.size();
      range_sort = functionSort->range.get();
      for (auto ptr : functionSort->domain) {
        Sort* argsort = ptr.get();
        domain_sorts.push_back(argsort);
        size_t sz;
        if (dynamic_cast<BooleanSort*>(argsort)) {
          sz = 2;
        } else if (UninterpretedSort* usort = dynamic_cast<UninterpretedSort*>(argsort)) {
          sz = sort_info[usort->name].domain_size;
        } else {
          assert(false && "expected boolean sort or uninterpreted sort");
        }
        domain_sort_sizes.push_back(sz);
      }
    } else {
      num_args = 0;
      range_sort = decl.sort.get();
    }

    function_info.insert(make_pair(name, FunctionInfo()));
    FunctionInfo& finfo = function_info[name];

    if (z3model.has_interp(fdecl)) {
      if (fdecl.is_const()) {
        z3::expr e = z3model.get_const_interp(fdecl);
        finfo.else_value = 0;
        finfo.table.reset(new FunctionTable());
        finfo.table->value = get_value(range_sort, e);
      } else {
        z3::func_interp finterp = z3model.get_func_interp(fdecl);

        vector<object_value> args;
        for (int i = 0; i < num_args; i++) {
          args.push_back(0);
        }
        while (true) {
          z3::expr_vector args_exprs(ctx.ctx);
          unique_ptr<FunctionTable>* table = &finfo.table;
          for (int argnum = 0; argnum < num_args; argnum++) {
            object_value argvalue = args[argnum];
            args_exprs.push_back(get_expr(domain_sorts[argnum], argvalue));
            if (!table->get()) {
              table->reset(new FunctionTable());
              (*table)->children.resize(domain_sort_sizes[argnum]);
            }
            assert(0 <= argvalue && (int)argvalue < domain_sort_sizes[argnum]);
            table = &(*table)->children[argvalue];
          }
          object_value result_value =
              get_value(range_sort, finterp.else_value().substitute(args_exprs));

          assert (table != NULL);
          if (!table->get()) {
            table->reset(new FunctionTable());
          }
          (*table)->value = result_value;

          int i;
          for (i = num_args - 1; i >= 0; i--) {
            args[i]++;
            if ((int)args[i] == domain_sort_sizes[i]) {
              args[i] = 0;
            } else {
              break;
            }
          }
          if (i == -1) {
            break;
          }
        }

        for (size_t i = 0; i < finterp.num_entries(); i++) {
          z3::func_entry fentry = finterp.entry(i);
          
          unique_ptr<FunctionTable>* table = &finfo.table;
          for (int argnum = 0; argnum < num_args; argnum++) {
            object_value argvalue = get_value(domain_sorts[argnum], fentry.arg(argnum));

            if (!table->get()) {
              table->reset(new FunctionTable());
              (*table)->children.resize(domain_sort_sizes[argnum]);
            }
            assert(0 <= argvalue && (int)argvalue < domain_sort_sizes[argnum]);
            table = &(*table)->children[argvalue];
          }

          (*table)->value = get_value(range_sort, fentry.value());
        }
      }
    } else {
      finfo.else_value = 0;
    }
  }

  return shared_ptr<Model>(new Model(module, move(sort_info), move(function_info)));
}

extern bool enable_smt_logging;

namespace smt_z3 {

  //////////////////////
  ///// solving

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

  smt::SolverResult solver::check_result()
  {
    auto t1 = now();
    z3::check_result res;
    try {
      res = z3_solver.check();
    } catch (z3::exception exc) {
      cout << "got z3 exception" << endl;
      res = z3::unknown;
    }
    auto t2 = now();

    long long ms = as_ms(t2 - t1);
    smt::log_to_stdout(ms, false, log_info, res_to_string(res));
    if (enable_smt_logging) {
      log_smtlib(ms, res_to_string(res));
    }

    global_stats.add_z3(ms);

    if (res == z3::sat) return smt::SolverResult::Sat;
    else if (res == z3::unsat) return smt::SolverResult::Unsat;
    return smt::SolverResult::Unknown;
  }

  void solver::dump(ofstream& of) {
    of << z3_solver << endl;
  }

}
