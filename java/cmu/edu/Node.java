package cmu.edu;

import java.util.ArrayList;

public class Node {
	
	protected boolean root;
	protected boolean leaf;
	protected int id;
	protected String decision;
	protected Node parent;
	protected ArrayList<Node> children;
	
	Node(int id, boolean root, boolean leaf){
		this.root = root;
		this.leaf = leaf;
		this.id = id;
		parent = null;
		children = new ArrayList<>();
	}
	
	public int getId() {
		return id;
	}
	
	public String getDecision() {
		return decision;
	}
	
	public void setDecision(String name) {
		decision = name;
	}
	
	public Node getParent() {
		return parent;
	}
	
	public ArrayList<Node> getChildren() {
		return children;
	}
	
	public void setParent(Node parent) {
		this.parent = parent;
	}
	
	public void setChildren(ArrayList<Node> children) {
		this.children.clear();
		this.children.addAll(children);
	}
	

}
