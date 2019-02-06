#include "logic.h"
#include "contexts.h"
#include "model.h"
#include "grammar.h"
#include "smt.h"
#include "benchmarking.h"

#include <iostream>
#include <iterator>
#include <string>

using namespace std;

bool try_to_add_invariant(
    shared_ptr<InitContext> initctx,
    shared_ptr<InductionContext> indctx,
    shared_ptr<ConjectureContext> conjctx,
    shared_ptr<InvariantsContext> invctx,
    shared_ptr<Value> conjecture
) {
  shared_ptr<Value> not_conjecture = shared_ptr<Value>(new Not(conjecture));

  // Check if INIT ==> INV
  z3::solver& init_solver = initctx->ctx->solver;
  init_solver.push();
  init_solver.add(initctx->e->value2expr(not_conjecture));
  z3::check_result init_res = init_solver.check();

  assert (init_res == z3::sat || init_res == z3::unsat);

  if (init_res == z3::sat) {
    init_solver.pop();
    return false;
  } else {
    init_solver.pop();
  }

  z3::solver& solver = indctx->ctx->solver;

  solver.push();
  solver.add(indctx->e1->value2expr(conjecture));
  solver.push();
  solver.add(indctx->e2->value2expr(not_conjecture));

  z3::check_result res = solver.check();
  assert (res == z3::sat || res == z3::unsat);

  /*
  printf("'%s'\n", solver.to_smt2().c_str());

  if (res == z3::sat) {
    z3::model m = solver.get_model();
    std::cout << m << "\n";
  }
  */

  if (res == z3::unsat) {
    solver.pop();

    z3::solver& conj_solver = conjctx->ctx->solver;
    z3::solver& inv_solver = invctx->ctx->solver;

    solver.add(indctx->e2->value2expr(conjecture));
    init_solver.add(initctx->e->value2expr(conjecture));
    conj_solver.add(conjctx->e->value2expr(conjecture));
    inv_solver.add(invctx->e->value2expr(conjecture));

    return true;
  } else {
    // NOT INVARIANT
    // pop back to last good state
    solver.pop();
    solver.pop();
    return false;
  }
}

bool do_invariants_imply_conjecture(shared_ptr<ConjectureContext> conjctx) {
  z3::solver& solver = conjctx->ctx->solver;
  z3::check_result res = solver.check();
  assert (res == z3::sat || res == z3::unsat);
  return (res == z3::unsat);
}

bool is_redundant(
    shared_ptr<InvariantsContext> invctx,
    shared_ptr<Value> formula)
{
  z3::solver& solver = invctx->ctx->solver;
  solver.push();
  solver.add(invctx->e->value2expr(shared_ptr<Value>(new Not(formula))));

  z3::check_result res = solver.check();
  assert (res == z3::sat || res == z3::unsat);
  solver.pop();
  return (res == z3::unsat);
}

// get a Model that can be an initial state
shared_ptr<Model> get_dumb_model(
    shared_ptr<InitContext> initctx,
    shared_ptr<Module> module)
{
  z3::solver& solver = initctx->ctx->solver;

  solver.push();

  shared_ptr<Sort> node_sort = shared_ptr<Sort>(new UninterpretedSort("node"));
  VarDecl decl_a = VarDecl("A", node_sort);
  VarDecl decl_b = VarDecl("B", node_sort);
  VarDecl decl_c = VarDecl("C", node_sort);

  shared_ptr<Value> var_a = shared_ptr<Value>(new Var("A", node_sort));
  shared_ptr<Value> var_b = shared_ptr<Value>(new Var("B", node_sort));
  shared_ptr<Value> var_c = shared_ptr<Value>(new Var("C", node_sort));

  solver.add(initctx->e->value2expr(shared_ptr<Value>(new Exists(
      { decl_a, decl_b, decl_c },
      shared_ptr<Value>(new And({
        shared_ptr<Value>(new Not(shared_ptr<Value>(new Eq(var_a, var_b)))),
        shared_ptr<Value>(new Not(shared_ptr<Value>(new Eq(var_b, var_c)))),
        shared_ptr<Value>(new Not(shared_ptr<Value>(new Eq(var_c, var_a))))
      }))
    ))));

  z3::check_result res = solver.check();
  assert (res == z3::sat);
  
  shared_ptr<Model> result = Model::extract_model_from_z3(
      initctx->ctx->ctx,
      solver,
      module,
      *(initctx->e));

  solver.pop();

  return result;
}

bool try_to_add_invariants(
    shared_ptr<Module> module,
    shared_ptr<InitContext> initctx,
    shared_ptr<InductionContext> indctx,
    shared_ptr<ConjectureContext> conjctx,
    shared_ptr<InvariantsContext> invctx,
    vector<shared_ptr<Value>> const& invariants
) {
  Benchmarking bench;

  printf("have a list of %d candidate invariants\n", (int)invariants.size());

  bool solved = false;

  shared_ptr<Model> root_model = get_dumb_model(initctx, module);
  vector<shared_ptr<Model>> models = get_tree_of_models(initctx->ctx->ctx,
      module, root_model, 5);
  printf("using %d models\n", (int)models.size());

  int count_iterations = 0;
  int count_evals = 0;
  int count_redundancy_checks = 0;
  int count_invariance_checks = 0;
  int count_invariants_added = 0;

  vector<bool> is_good_candidate;
  is_good_candidate.resize(invariants.size());
  for (int i = 0; i < invariants.size(); i++) {
    is_good_candidate[i] = true;
  }

  for (int i_ = 0; i_ < invariants.size()*2; i_++) {
    int i = i_ % invariants.size();

    if (!is_good_candidate[i]) {
      continue;
    }

    count_iterations++;

    auto invariant = invariants[i];
    if (i_ % 100 == 0) printf("doing %d\n", i_);

    for (auto model : models) {
      count_evals++;
      bench.start("eval");
      bool model_eval = model->eval_predicate(invariant);
      bench.end();
      if (!model_eval) {
        is_good_candidate[i] = false;
        break;
      }
    }
    if (!is_good_candidate[i]) {
      continue;
    }

    count_redundancy_checks++;
    bench.start("redundancy");
    bool isr = is_redundant(invctx, invariant);
    bench.end();
    if (isr) {
      is_good_candidate[i] = false;
    } else {
      count_invariance_checks++;
      bench.start("try_to_add_invariant");
      bool ttai = try_to_add_invariant(initctx, indctx, conjctx, invctx, invariant);
      bench.end();
      if (ttai) {
        is_good_candidate[i] = false;
        // We added an invariant!
        // Now check if we're done.
        printf("found invariant (%d): %s\n", i, invariant->to_string().c_str());
        count_invariants_added++;
        if (do_invariants_imply_conjecture(conjctx)) {
          solved = true;
          break;
        }
      }
    }
  }

  printf("solved: %s\n", solved ? "yes" : "no");
  printf("total iterations: %d\n", count_iterations);
  printf("total evals: %d\n", count_evals);
  printf("total redundancy checks: %d\n", count_redundancy_checks);
  printf("total invariance checks: %d\n", count_invariance_checks);
  printf("total invariants added: %d\n", count_invariants_added);

  bench.dump();

  return false;
}

// FIXME: create the grammar from the json file instead of manually specifying it
Grammar createGrammar() {
  Type node_type = Type("node");
  Type id_type = Type("id");
  Type bool_type = Type("bool");
  Type empty_type = Type("empty");
  std::vector<Type> types{node_type, id_type, bool_type, empty_type};

  std::vector<Function> functions;
  std::vector<Type> input_node{node_type};
  std::vector<Type> input_id_id{id_type, id_type};
  std::vector<Type> input_node_empty{node_type, empty_type};
  std::vector<Type> input_id_node{id_type, node_type};
  std::vector<Type> input_node_node{node_type, node_type};
  std::vector<Type> input_empty{empty_type};
  // this production controls the number of conjunctions
  std::vector<Type> input_and{bool_type, bool_type, bool_type};
  std::vector<Type> input_node_node_node{node_type, node_type, node_type};
  functions.push_back(Function("btw",input_node_node_node, bool_type));
  functions.push_back(Function("~btw",input_node_node_node, bool_type));
  functions.push_back(Function("nid", input_node, id_type));
  functions.push_back(Function("le", input_id_id, bool_type));
  functions.push_back(Function("~le", input_id_id, bool_type));
  functions.push_back(Function("leader", input_node_empty, bool_type));
  functions.push_back(Function("~leader", input_node_empty, bool_type));
  functions.push_back(Function("pnd", input_id_node, bool_type));
  functions.push_back(Function("~=", input_node_node, bool_type));
  functions.push_back(Function("=", input_node_node, bool_type));
  functions.push_back(Function("empty", input_empty, empty_type));
  functions.push_back(Function("A", input_empty, node_type));
  functions.push_back(Function("B", input_empty, node_type));
  functions.push_back(Function("C", input_empty, node_type));
  functions.push_back(Function("and",input_and, bool_type));

  Grammar g = Grammar(types, functions);
  return g;
}

vector<shared_ptr<Value>> get_values_list_qle() {
  vector<shared_ptr<Value>> result;

  vector<shared_ptr<Value>> pieces;

  shared_ptr<Sort> node_sort = shared_ptr<Sort>(new UninterpretedSort("node"));
  shared_ptr<Sort> nset_sort = shared_ptr<Sort>(new UninterpretedSort("nset"));
  shared_ptr<Sort> bool_sort = shared_ptr<Sort>(new BooleanSort());


  shared_ptr<Value> isleader_func = shared_ptr<Value>(new Const(
      "isleader",
      shared_ptr<Sort>(new FunctionSort({node_sort}, bool_sort))));

  shared_ptr<Value> voted_func = shared_ptr<Value>(new Const(
      "voted",
      shared_ptr<Sort>(new FunctionSort({node_sort, node_sort}, bool_sort))));

  shared_ptr<Value> member_func = shared_ptr<Value>(new Const(
      "member",
      shared_ptr<Sort>(new FunctionSort({node_sort, nset_sort}, bool_sort))));

  shared_ptr<Value> majority_func = shared_ptr<Value>(new Const(
      "majority",
      shared_ptr<Sort>(new FunctionSort({nset_sort}, bool_sort))));

  shared_ptr<Value> quorum_func = shared_ptr<Value>(new Const(
      "quorum",
      shared_ptr<Sort>(new FunctionSort({}, nset_sort))));

  vector<VarDecl> decls;
  decls.push_back(VarDecl("A", node_sort));
  decls.push_back(VarDecl("B", node_sort));
  decls.push_back(VarDecl("C", node_sort));

  vector<shared_ptr<Value>> node_atoms;
  vector<shared_ptr<Value>> nset_atoms;

  node_atoms.push_back(shared_ptr<Value>(new Var("A", node_sort)));
  node_atoms.push_back(shared_ptr<Value>(new Var("B", node_sort)));
  node_atoms.push_back(shared_ptr<Value>(new Var("C", node_sort)));

  nset_atoms.push_back(shared_ptr<Value>(new Apply(quorum_func, {})));

  for (int i = 0; i < node_atoms.size(); i++) {
    pieces.push_back(shared_ptr<Value>(
      new Apply(isleader_func, { node_atoms[i] })
    ));
    pieces.push_back(shared_ptr<Value>(new Not(shared_ptr<Value>(
      new Apply(isleader_func, { node_atoms[i] })
    ))));
  }

  for (int i = 0; i < node_atoms.size(); i++) {
    for (int j = 0; j < node_atoms.size(); j++) {
      pieces.push_back(shared_ptr<Value>(
        new Apply(voted_func, { node_atoms[i], node_atoms[j] })
      ));
      pieces.push_back(shared_ptr<Value>(new Not(shared_ptr<Value>(
        new Apply(voted_func, { node_atoms[i], node_atoms[j] })
      ))));
    }
  }

  for (int i = 0; i < node_atoms.size(); i++) {
    for (int j = 0; j < nset_atoms.size(); j++) {
      pieces.push_back(shared_ptr<Value>(
        new Apply(member_func, { node_atoms[i], nset_atoms[j] })
      ));
      pieces.push_back(shared_ptr<Value>(new Not(shared_ptr<Value>(
        new Apply(member_func, { node_atoms[i], nset_atoms[j] })
      ))));
    }
  }

  for (int j = 0; j < nset_atoms.size(); j++) {
    pieces.push_back(shared_ptr<Value>(
      new Apply(majority_func, { nset_atoms[j] })
    ));
    pieces.push_back(shared_ptr<Value>(new Not(shared_ptr<Value>(
      new Apply(majority_func, { nset_atoms[j] })
    ))));
  }

  for (int i = 0; i < nset_atoms.size(); i++) {
    for (int j = i+1; j < nset_atoms.size(); j++) {
      pieces.push_back(shared_ptr<Value>(
        new Eq(nset_atoms[i], nset_atoms[j])
      ));
      pieces.push_back(shared_ptr<Value>(new Not(shared_ptr<Value>(
        new Eq(nset_atoms[i], nset_atoms[j])
      ))));
    }
  }

  for (int i = 0; i < node_atoms.size(); i++) {
    for (int j = i+1; j < node_atoms.size(); j++) {
      pieces.push_back(shared_ptr<Value>(
        new Eq(node_atoms[i], node_atoms[j])
      ));
      pieces.push_back(shared_ptr<Value>(new Not(shared_ptr<Value>(
        new Eq(node_atoms[i], node_atoms[j])
      ))));
    }
  }

  for (int i = 0; i < pieces.size(); i++) {
    for (int j = i+1; j < pieces.size(); j++) {
      for (int k = j+1; k < pieces.size(); k++) {
        result.push_back(shared_ptr<Value>(new Forall(decls,
            shared_ptr<Value>(new Not(
              shared_ptr<Value>(new And(
                { pieces[i], pieces[j], pieces[k] })))))));
      }
    }
  }

  return result;
}

vector<shared_ptr<Value>> get_values_list() {
  vector<shared_ptr<Value>> result;

  vector<shared_ptr<Value>> pieces;

  shared_ptr<Sort> node_sort = shared_ptr<Sort>(new UninterpretedSort("node"));
  shared_ptr<Sort> id_sort = shared_ptr<Sort>(new UninterpretedSort("id"));
  shared_ptr<Sort> bool_sort = shared_ptr<Sort>(new BooleanSort());

  shared_ptr<Value> id_func = shared_ptr<Value>(new Const(
      "nid",
      shared_ptr<Sort>(new FunctionSort({node_sort}, id_sort))));
  shared_ptr<Value> btw_func = shared_ptr<Value>(new Const(
      "btw",
      shared_ptr<Sort>(new FunctionSort({node_sort, node_sort, node_sort}, bool_sort))));
  shared_ptr<Value> leader_func = shared_ptr<Value>(new Const(
      "leader",
      shared_ptr<Sort>(new FunctionSort({node_sort}, bool_sort))));
  shared_ptr<Value> pnd_func = shared_ptr<Value>(new Const(
      "pnd",
      shared_ptr<Sort>(new FunctionSort({id_sort, node_sort}, bool_sort))));
  shared_ptr<Value> le_func = shared_ptr<Value>(new Const(
      "<=",
      shared_ptr<Sort>(new FunctionSort({id_sort, id_sort}, bool_sort))));

  vector<VarDecl> decls;
  decls.push_back(VarDecl("A", node_sort));
  decls.push_back(VarDecl("B", node_sort));
  decls.push_back(VarDecl("C", node_sort));

  vector<shared_ptr<Value>> node_atoms;
  vector<shared_ptr<Value>> id_atoms;

  node_atoms.push_back(shared_ptr<Value>(new Var("A", node_sort)));
  node_atoms.push_back(shared_ptr<Value>(new Var("B", node_sort)));
  node_atoms.push_back(shared_ptr<Value>(new Var("C", node_sort)));

  for (auto node_atom : node_atoms) {
    id_atoms.push_back(shared_ptr<Value>(new Apply(id_func, { node_atom })));
  }

  for (int i = 0; i < node_atoms.size(); i++) {
    for (int j = i+1; j < node_atoms.size(); j++) {
      for (int k = j+1; k < node_atoms.size(); k++) {
        pieces.push_back(shared_ptr<Value>(
          new Apply(btw_func, { node_atoms[i], node_atoms[j], node_atoms[k] })
        ));
        pieces.push_back(shared_ptr<Value>(new Not(shared_ptr<Value>(
          new Apply(btw_func, { node_atoms[i], node_atoms[j], node_atoms[k] })
        ))));
      }
    }
  }

  for (int i = 0; i < node_atoms.size(); i++) {
    pieces.push_back(shared_ptr<Value>(
      new Apply(leader_func, { node_atoms[i] })
    ));
    pieces.push_back(shared_ptr<Value>(new Not(shared_ptr<Value>(
      new Apply(leader_func, { node_atoms[i] })
    ))));
  }

  for (int i = 0; i < id_atoms.size(); i++) {
    for (int j = 0; j < node_atoms.size(); j++) {
      pieces.push_back(shared_ptr<Value>(
        new Apply(pnd_func, { id_atoms[i], node_atoms[j] })
      ));
      pieces.push_back(shared_ptr<Value>(new Not(shared_ptr<Value>(
        new Apply(pnd_func, { id_atoms[i], node_atoms[j] })
      ))));
    }
  }

  for (int i = 0; i < id_atoms.size(); i++) {
    for (int j = 0; j < id_atoms.size(); j++) {
      if (i != j) {
        pieces.push_back(shared_ptr<Value>(
          new Apply(le_func, { id_atoms[i], id_atoms[j] })
        ));
      }
    }
  }

  for (int i = 0; i < id_atoms.size(); i++) {
    for (int j = i+1; j < id_atoms.size(); j++) {
      pieces.push_back(shared_ptr<Value>(
        new Eq(id_atoms[i], id_atoms[j])
      ));
      pieces.push_back(shared_ptr<Value>(new Not(shared_ptr<Value>(
        new Eq(id_atoms[i], id_atoms[j])
      ))));
    }
  }

  for (int i = 0; i < node_atoms.size(); i++) {
    for (int j = i+1; j < node_atoms.size(); j++) {
      pieces.push_back(shared_ptr<Value>(new Not(shared_ptr<Value>(
        new Eq(node_atoms[i], node_atoms[j])
      ))));
    }
  }

  for (int i = 0; i < pieces.size(); i++) {
    for (int j = i+1; j < pieces.size(); j++) {
      for (int k = j+1; k < pieces.size(); k++) {
        result.push_back(shared_ptr<Value>(new Forall(decls,
            shared_ptr<Value>(new Not(
              shared_ptr<Value>(new And(
                { pieces[i], pieces[j], pieces[k] })))))));
      }
    }
  }

  return result;
}

void add_constraints(shared_ptr<Module> module, SMT& solver) {
  solver.createBinaryAssociativeConstraints("=.node"); // a = b <-> b = a
  solver.createBinaryAssociativeConstraints("~=.node"); // a ~= b <- b ~= a
  solver.createAllDiffConstraints("=.node"); // does not allow forall. x: x = x
  solver.createAllDiffConstraints("~=.node"); // does not allow forall. x: x ~= x
  solver.createAllDiffGrandChildrenConstraints("<="); // does not allow forall. x: le (pnd x) (pnd x)
  solver.createAllDiffGrandChildrenConstraints("~<="); // does not allow forall. x: ~le (pnd x) (pnd x)
  // ring properties:
  // ABC -> BCA -> CAB -> ABC
  // ACB -> CBA -> BAC -> ACB
  // only allow ABC or ACB, i.e. do not allow the others
  solver.breakOccurrences("btw","B","C","A");
  solver.breakOccurrences("btw","C","A","B");
  solver.breakOccurrences("btw","C","B","A");
  solver.breakOccurrences("btw","B","A","C");
  solver.breakOccurrences("~btw","B","C","A");
  solver.breakOccurrences("~btw","C","A","B");
  solver.breakOccurrences("~btw","C","B","A");
  solver.breakOccurrences("~btw","B","A","C");

  solver.createAllDiffConstraints("~btw");
  solver.createAllDiffConstraints("btw");
}

int main() {

  // FIXME: quick hack to control which enumeration to use
  bool smt_enumeration = false;

  std::istreambuf_iterator<char> begin(std::cin), end;
  std::string json_src(begin, end);

  shared_ptr<Module> module = parse_module(json_src);

  try {
    if (!smt_enumeration) {
      //vector<shared_ptr<Value>> candidates = get_values_list_qle();

      printf("enumerating candidates...\n");
      vector<shared_ptr<Value>> candidates;
      Grammar grammar = createGrammarFromModule(module);
      context z3_ctx;
      solver z3_solver(z3_ctx);
      SMT solver = SMT(grammar, z3_ctx, z3_solver, 3);
      add_constraints(module, solver);
      while (solver.solve()){
        candidates.push_back(solver.solutionToValue());
        if (candidates.size() % 100 == 0) printf("%d\n", (int) candidates.size());
      }
      printf("done enumerating.\n");

      z3::context ctx;

      auto indctx = shared_ptr<InductionContext>(new InductionContext(ctx, module));
      auto initctx = shared_ptr<InitContext>(new InitContext(ctx, module));
      auto conjctx = shared_ptr<ConjectureContext>(new ConjectureContext(ctx, module));
      auto invctx = shared_ptr<InvariantsContext>(new InvariantsContext(ctx, module));

      try_to_add_invariants(module, initctx, indctx, conjctx, invctx, candidates);
      return 0;

      //for (int i = module->conjectures.size() - 1; i >= 0; i--) {
      //  add_invariant(indctx, initctx, conjctx, module->conjectures[i]);
      //}

      /*
      initctx->ctx->solver.add(initctx->e->value2expr(shared_ptr<Value>(new Not(module->conjectures[0]))));
      z3::check_result res = initctx->ctx->solver.check();
      assert(res == z3::sat);
      Model m = Model::extract_model_from_z3(ctx, initctx->ctx->solver, module, *initctx->e);
      m.dump();
      */
    } else {

      // TODO: connect with the add invariant code
      Grammar grammar = createGrammarFromModule(module);
      context z3_ctx;
      solver z3_solver(z3_ctx);
      SMT solver = SMT(grammar, z3_ctx, z3_solver, 3);
      add_constraints(module, solver);
      int program = 1;
      while (solver.solve()){
        std::cout << "#program= " << program << std::endl;
        program++;
        std::cout << solver.solutionToString() << std::endl;
      }
      std::cout << "end" << std::endl;
    }

  } catch (z3::exception exc) {
    printf("got z3 exception: %s\n", exc.msg());
    throw;
  }
}
