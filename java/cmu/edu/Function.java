package cmu.edu;

import java.util.ArrayList;

public class Function {
	
	protected String name;
	protected ArrayList<Type> inputs;
	protected Type output;
	
	public Function(String name, ArrayList<Type> inputs, Type output){
		this.inputs = inputs;
		this.output = output;
		this.name = name;
	}
	
	public String getName() {
		return name;
	}
	
	public ArrayList<Type> getInputs(){
		return inputs;
	}
	
	public Type getOutput(){
		return output;
	}

}
