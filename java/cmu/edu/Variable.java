package cmu.edu;

public class Variable {
	
	protected Node node;
	protected Function function;
	
	public Variable(Node node, Function function) {
		this.node = node;
		this.function = function;
	}
	
	public boolean equals(Object o) {
		if (o instanceof Variable) {
			Variable that = (Variable) o;
			return this.node == that.node && this.function == that.function;
		}
		return false;
	}
	
	public int hashCode() {
		return (node == null ? 0 : node.hashCode()) +
			   (function == null ? 0 : function.hashCode());
	}

}
