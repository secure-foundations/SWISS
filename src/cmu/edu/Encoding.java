package cmu.edu;

import java.util.ArrayList;
import java.util.HashMap;

import org.sat4j.core.VecInt;
import org.sat4j.pb.IPBSolver;
import org.sat4j.specs.ContradictionException;
import org.sat4j.specs.TimeoutException;

public class Encoding {

	protected HashMap<Integer, Variable> id2vars = new HashMap<>();
	protected HashMap<Variable, Integer> vars2id = new HashMap<>();
	protected IPBSolver solver = org.sat4j.pb.SolverFactory.newDefault();
	protected int[] model;
	protected boolean ok;

	public Encoding() {
		ok = true;
	}

	public void createVariables(Enumerator enumerator, ArrayList<ArrayList<Node>> nodes) {
		int v = 1;
		for (ArrayList<Node> node_list : nodes) {
			for (Node node : node_list) {
				for (Function function : enumerator.functions) {
					Variable var = new Variable(node, function);
					id2vars.put(v, var);
					vars2id.put(var, v);
					v++;
				}
			}
		}

		solver.newVar(v);
	}

	public void createConstraints(Enumerator enumerator, ArrayList<Node> nodes) throws ContradictionException {
		// Each node has at least a function
		for (Node node : nodes) {
			VecInt constraint = new VecInt();
			for (Function function : enumerator.functions) {
				Variable var = new Variable(node, function);
				constraint.push(vars2id.get(var));
			}
			solver.addExactly(constraint, 1);
		}

		// If function is != then we do not want to compare the same node
		for (Node node : nodes) {
			if (node.root) {
				for (Function function : enumerator.functions) {
					if (function.name.equals("~=")) {
						for (Function inner : enumerator.functions) {
							if (inner.name.equals("A") || inner.name.equals("B") || inner.name.equals("C")) {
								Variable v1 = new Variable(node.getChildren().get(0), inner);
								Variable v2 = new Variable(node.getChildren().get(1), inner);
								Variable v3 = new Variable(node, function);
								VecInt constraint = new VecInt(
										new int[] { -vars2id.get(v3), -vars2id.get(v2), -vars2id.get(v1) });
								solver.addClause(constraint);
							}
						}
					}
				}
			}
		}

		// Root node must return a boolean
		for (Node node : nodes) {
			if (node.root) {
				VecInt alo = new VecInt();
				for (Function function : enumerator.functions) {
					Variable var = new Variable(node, function);
					if (!function.output.type.equals("bool")) {
						VecInt constraint = new VecInt(new int[] { -vars2id.get(var) });
						solver.addClause(constraint);
					} else {
						alo.push(vars2id.get(var));
					}
				}
				solver.addClause(alo);
			}
		}

		// Leafs node can only be A or B or empty
		for (Node node : nodes) {
			if (node.leaf) {
				VecInt alo = new VecInt();
				for (Function function : enumerator.functions) {
					Variable var = new Variable(node, function);
					if (!function.name.equals("A") && !function.name.equals("B") && !function.name.equals("C")
							&& !function.name.equals("empty")) {
						VecInt constraint = new VecInt(new int[] { -vars2id.get(var) });
						solver.addClause(constraint);
					} else {
						alo.push(vars2id.get(var));
					}
				}
				solver.addClause(alo);
			}
		}

		// Each function restricts the children
		for (Function function : enumerator.functions) {
			for (Node node : nodes) {
				Variable v1 = new Variable(node, function);
				if (node.children.size() != function.inputs.size())
					continue;
				for (int i = 0; i < node.children.size(); i++) {
					VecInt alo = new VecInt(new int[] { -vars2id.get(v1) });
					for (Function inner : enumerator.functions) {
						Variable v2 = new Variable(node.children.get(i), inner);
						if (!function.inputs.get(i).type.equals(inner.output.type)) {
							VecInt constraint = new VecInt(new int[] { -vars2id.get(v1), -vars2id.get(v2) });
							solver.addClause(constraint);
						} else {
							alo.push(vars2id.get(v2));
						}
					}
					solver.addClause(alo);

				}
			}
		}
	}

	public boolean solve() {
		try {
			if (!ok)
				return false;
			boolean res = solver.isSatisfiable();
			if (res == true) {
				model = solver.model().clone();
				// block model
				VecInt constraint = new VecInt();
				for (int i = 0; i < model.length; i++) {
					if (model[i] > 0)
						constraint.push(-model[i]);
				}
				try {
					solver.addClause(constraint);
				} catch (ContradictionException e) {
					ok = false;
				}
			}
			return res;
		} catch (TimeoutException e) {
			return false;
		}
	}

	public void printModel() {
		HashMap<Integer, String> conjecture = new HashMap<>();
		for (int i = 0; i < model.length; i++) {
			if (model[i] > 0) {
				Variable var = id2vars.get(i + 1);
				conjecture.put(var.node.id, var.function.name);
			}
		}

		String result = "conjecture forall A : node . forall B: node . forall C: node . ~(";

		if (conjecture.get(1).equals("~=")) {
			result += conjecture.get(2) + conjecture.get(1) + conjecture.get(3);
		} else {
			result += conjecture.get(1) + "(";

			assert (!conjecture.get(2).startsWith("empty"));
			if (!conjecture.get(2).equals("A") && !conjecture.get(2).equals("B") && !conjecture.get(2).equals("C")) {
				result += conjecture.get(2) + "(";
				result += conjecture.get(4) + ")";
			} else
				result += conjecture.get(2);
			
			
			if (!conjecture.get(3).equals("A") && !conjecture.get(3).equals("B") && !conjecture.get(3).equals("C")
					&& !conjecture.get(3).equals("empty")) {
				result += ",";
				result += conjecture.get(3) + "(";
				result += conjecture.get(5) + ")";
			} else {
				if (!conjecture.get(3).equals("empty"))
					result += "," + conjecture.get(3);
			}
			
			result += ")";
		}

		result += ")";
		System.out.println(result);

	}

}
