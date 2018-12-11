package cmu.edu;

public class Type {
	
	protected String type;
	protected String name;
	protected boolean terminal;
	
	public Type(String type) {
		this.type = type;
		terminal = false;
	}
	
	public Type(String name, String type) {
		this.name = name;
		this.type = type;
		terminal = true;
	}
	
	public boolean isTerminal() {
		return terminal;
	}
	
	public String getName() {
		return name;
	}
	
	public String getType() {
		return type;
	}

}
