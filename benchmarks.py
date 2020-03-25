class Benchmark(object):
  def __init__(self, ivy_file, args):
    self.ivy_file = ivy_file
    self.args = args

BENCHMARKS = {

### Leader election

"lock-server" :
  Benchmark("benchmarks/lock_server.ivy", "--finisher --conj-arity 1 --disj-arity 2 --with-conjs"),

"breadth-lock-server" :
  Benchmark("benchmarks/lock_server.ivy", "--breadth --conj-arity 1 --disj-arity 2"),

"breadth-2pc" :
  Benchmark("benchmarks/2PC.ivy", "--breadth --conj-arity 1 --disj-arity 3"),

"leader-election" :
  Benchmark("benchmarks/leader-election.ivy", "--finisher --conj-arity 3 --disj-arity 3"),

"inc-leader-election" : 
  Benchmark("benchmarks/leader-election.ivy", "--incremental --conj-arity 1 --disj-arity 3"),

"breadth-leader-election" : 
  Benchmark("benchmarks/leader-election.ivy", "--breadth --conj-arity 1 --disj-arity 3"),

"sat-leader-election" : 
  Benchmark("benchmarks/leader-election.ivy", "--finisher --enum-sat --arity 3 --depth 4 --conj"),

"sat-inc-leader-election" : 
  Benchmark("benchmarks/leader-election.ivy", "--incremental --enum-sat --arity 3 --depth 3"),

"sat-breadth-leader-election" : 
  Benchmark("benchmarks/leader-election.ivy", "--breadth --enum-sat --arity 3 --depth 3"),

### Learning switch

"sat-inc-learning-switch" : 
  Benchmark("benchmarks/learning-switch.ivy", "--incremental --enum-sat --arity 3 --depth 3"),

"sat-breadth-learning-switch" : 
  Benchmark("benchmarks/learning-switch.ivy", "--breadth --enum-sat --arity 3 --depth 3"),

"inc-learning-switch" : 
  Benchmark("benchmarks/learning-switch.ivy", "--incremental --conj-arity 1 --disj-arity 3"),

"breadth-learning-switch" : 
  Benchmark("benchmarks/learning-switch.ivy", "--breadth --conj-arity 1 --disj-arity 3"),

### Chord

"sat-breadth-chord" : 
  Benchmark("benchmarks/chord.ivy", "--enum-sat --breadth --arity 4 --depth 3"),

"breadth-chord-size3" : 
  Benchmark("benchmarks/chord.ivy", "--breadth --conj-arity 1 --disj-arity 3"),

"inc-chord-size3" : 
  Benchmark("benchmarks/chord.ivy", "--incremental --conj-arity 1 --disj-arity 3"),

"breadth-chord" : 
  Benchmark("benchmarks/chord.ivy", "--breadth --conj-arity 1 --disj-arity 4"),

### Paxos

"sat-inc-paxos" : 
  Benchmark("benchmarks/paxos_epr.ivy", "--enum-sat --incremental --arity 4 --depth 3"),

"breadth-paxos-3" : 
  Benchmark("benchmarks/paxos_epr_3.ivy", "--breadth --conj-arity 1 --disj-arity 3"),

"breadth-paxos-4-r2" : 
  Benchmark("benchmarks/paxos_epr_4_r2.ivy", "--breadth --conj-arity 1 --disj-arity 4"),

"breadth-paxos-4-r3" : 
  Benchmark("benchmarks/paxos_epr_4_r3.ivy", "--breadth --conj-arity 1 --disj-arity 4"),

"breadth-paxos-exist-1" :
  Benchmark("benchmarks/paxos_epr_existential_1.ivy", "--breadth --conj-arity 1 --disj-arity 3 --strat-alt --impl-shape --start-with-existing-conjectures"),

"finisher-paxos-exist-1" :
  Benchmark("benchmarks/paxos_epr_full_existential_1.ivy", "--finisher --conj-arity 1 --disj-arity 3 --strat-alt --impl-shape --with-conjs"),

# note: this one is a lot easier than breadth-paxos-exist-1
"breadth-paxos-exist-2" :
  Benchmark("benchmarks/paxos_epr_existential_2.ivy", "--breadth --conj-arity 1 --disj-arity 3 --strat-alt --start-with-existing-conjectures"),

"sat-breadth-paxos-3" : 
  Benchmark("benchmarks/paxos_epr_3.ivy", "--enum-sat --breadth --arity 3 --depth 3"),

"sat-breadth-paxos-4-r2" : 
  Benchmark("benchmarks/paxos_epr_4_r2.ivy", "--enum-sat --breadth --arity 4 --depth 3"),

"sat-breadth-paxos-4-r3" : 
  Benchmark("benchmarks/paxos_epr_4_r3.ivy", "--enum-sat --breadth --arity 4 --depth 3"),
}
