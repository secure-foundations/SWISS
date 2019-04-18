

vector<State> transition(Context const& context, State const& state, map<string, object_value> scope, shared_ptr<Action> action) {

  if (LocalAction* action = dynamic_cast<LocalAction*>(a.get())) {
    vector<object_value> values;
    for (int i = 0; i < action->args.size(); i++) {
      values.push_back(0);
    }
    while (true) {

    }
    
  } else if (SequenceAction* action = dynamic_cast<SequenceAction*>(a.get())) {
    vector<State> res = {state};
    for (auto subaction : action->actions) {
      vector<State> next;
      for (State const& s : res) {
        extend_vector(next, transition(context, s, scope, subaction));
      }
      res = move(next);
    }
    return res;
  }
  else if (Assume* action = dynamic_cast<Assume*>(a.get())) {
    if (evaluate(context, state, scope, action->body)) {
      return {state};
    } else {
      return {};
    }
  }
  else if (If* action = dynamic_cast<If*>(a.get())) {
    if (evaluate(context, state, scope, condition)) {
      return transition(context, state, scope, action->then_body);
    } else {
      return {state};
    }
  }
  else if (IfElse* action = dynamic_cast<IfElse*>(a.get())) {
    if (evaluate(context, state, scope, condition)) {
      return transition(context, state, scope, action->then_body);
    } else {
      return transition(context, state, scope, action->else_body);
    }
  }
  else if (ChoiceAction* action = dynamic_cast<ChoiceAction*>(a.get())) {
    vector<State> res;
    for (auto subaction : action->actions) {
      extend(res, transition(context, state, scope, subaction));
    }
    return res;
  }
  else if (Assign* action = dynamic_cast<Assign*>(a.get())) {
  }
  else {
    assert(false && "transition does not implement this unknown case");
  }
}
