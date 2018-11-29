package cmu.edu;

import java.util.ArrayList;
import java.util.Arrays;

import org.sat4j.specs.ContradictionException;

public class Enumerator {

	/*
	 * We assume a fixed AST structure of 5 nodes with: root -> a root -> b a -> c b
	 * -> d
	 */

	public ArrayList<Type> types;
	public ArrayList<Type> terminals;
	public ArrayList<Function> functions;

	ArrayList<Type> getTypes() {
		return types;
	}

	ArrayList<Function> getFunctions() {
		return functions;
	}

	Enumerator() {
		types = new ArrayList<>();
		functions = new ArrayList<>();
		terminals = new ArrayList<>();
	}

	ArrayList<Node> createNodes(int id) {

		int i = id;

		// AST structure
		Node root = new Node(i++, true, false);
		Node a = new Node(i++, false, false);
		Node b = new Node(i++, false, false);
		Node c = new Node(i++, false, true);
		Node d = new Node(i++, false, true);

		ArrayList<Node> root_children = new ArrayList<Node>(Arrays.asList(a, b));
		ArrayList<Node> a_children = new ArrayList<Node>(Arrays.asList(c));
		ArrayList<Node> b_children = new ArrayList<Node>(Arrays.asList(d));

		root.setChildren(root_children);
		a.setParent(root);
		b.setParent(root);
		a.setChildren(a_children);
		b.setChildren(b_children);
		c.setParent(a);
		d.setParent(b);

		ArrayList<Node> nodes = new ArrayList<Node>(Arrays.asList(root, a, b, c, d));
		return nodes;
	}

	public static void main(String[] args) {

		Enumerator enumerator = new Enumerator();

		// Grammar
		Type node_type = new Type("node");
		Type id_type = new Type("id");
		Type bool_type = new Type("bool");
		Type empty_type = new Type("empty");
		enumerator.types = new ArrayList<Type>(Arrays.asList(node_type, id_type, bool_type, empty_type));
		enumerator.functions.add(new Function("nid", new ArrayList<Type>(Arrays.asList(node_type)), id_type));
		enumerator.functions.add(new Function("le", new ArrayList<Type>(Arrays.asList(id_type, id_type)), bool_type));
		enumerator.functions
				.add(new Function("leader", new ArrayList<Type>(Arrays.asList(node_type, empty_type)), bool_type));
		enumerator.functions
				.add(new Function("pnd", new ArrayList<Type>(Arrays.asList(id_type, node_type)), bool_type));
		enumerator.functions
				.add(new Function("~=", new ArrayList<Type>(Arrays.asList(node_type, node_type)), bool_type));
		enumerator.functions.add(new Function("empty", new ArrayList<Type>(Arrays.asList(empty_type)), empty_type));
		enumerator.functions.add(new Function("A", new ArrayList<Type>(Arrays.asList(empty_type)), node_type));
		enumerator.functions.add(new Function("B", new ArrayList<Type>(Arrays.asList(empty_type)), node_type));
		enumerator.functions.add(new Function("C", new ArrayList<Type>(Arrays.asList(empty_type)), node_type));

		int nb_conjunctions = 3; // change this value to control the number of conjunctions
		assert (nb_conjunctions >= 1 && nb_conjunctions <= 3);

		// TODO: generalize this to any number of conjunctions
		ArrayList<Node> nodes1 = enumerator.createNodes(1);
		ArrayList<Node> nodes2 = enumerator.createNodes(6);
		ArrayList<Node> nodes3 = enumerator.createNodes(11);

		ArrayList<ArrayList<Node>> all_nodes = new ArrayList<>();
		all_nodes.add(nodes1);
		if (nb_conjunctions >= 2)
			all_nodes.add(nodes2);
		if (nb_conjunctions >= 3)
			all_nodes.add(nodes3);

		Encoding encoding = new Encoding();
		encoding.createVariables(enumerator, all_nodes);
		try {
			encoding.createConstraints(enumerator, all_nodes);
			boolean res = true;
			int models = 0;
			while (res) {
				res = encoding.solve();
				String result = "conjecture forall A : node . forall B: node . forall C: node . ~(" + encoding.convertModel(0);
				String ast2 = "";
				String ast3 = "";
				if (nb_conjunctions >= 2) {
					ast2 = encoding.convertModel(1);
					result += " && " + ast2;
				}
				if (nb_conjunctions >= 3) {
					ast3 = encoding.convertModel(2);
					result += " && " + ast3;
				}
				result += ")";
				System.out.println(result);
				models++;
			}
			System.out.println("#models = " + models);
		} catch (ContradictionException e) {
			// problem is unsatisfiable
		}

	}

}
