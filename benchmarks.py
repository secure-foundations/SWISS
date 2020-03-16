class Benchmark(object):
  def __init__(self, ivy_file, args):
    self.ivy_file = ivy_file
    self.args = args

BENCHMARKS = {

### Leader election

"naive-leader-election" :
  Benchmark("benchmarks/leader-election.ivy", "--enum-naive --conj-arity 3 --disj-arity 3 --whole-space"),

"naive-inc-leader-election" : 
  Benchmark("benchmarks/leader-election.ivy", "--incremental --enum-naive --conj-arity 1 --disj-arity 3 --whole-space"),

"naive-breadth-leader-election" : 
  Benchmark("benchmarks/leader-election.ivy", "--breadth --enum-naive --conj-arity 1 --disj-arity 3 --whole-space"),

"naive-strat2-breadth-leader-election" : 
  Benchmark("benchmarks/leader-election.ivy", "--breadth --enum-naive --conj-arity 1 --disj-arity 3 --strat2 --whole-space"),

"sat-leader-election" : 
  Benchmark("benchmarks/leader-election.ivy", "--enum-sat --arity 3 --depth 4 --whole-space"),

"sat-inc-leader-election" : 
  Benchmark("benchmarks/leader-election.ivy", "--incremental --enum-sat --arity 3 --depth 3 --whole-space"),

"sat-breadth-leader-election" : 
  Benchmark("benchmarks/leader-election.ivy", "--breadth --enum-sat --arity 3 --depth 3 --whole-space"),

### Learning switch

"sat-inc-learning-switch" : 
  Benchmark("benchmarks/learning-switch.ivy", "--incremental --enum-sat --arity 3 --depth 3 --whole-space"),

"sat-breadth-learning-switch" : 
  Benchmark("benchmarks/learning-switch.ivy", "--breadth --enum-sat --arity 3 --depth 3 --whole-space"),

"naive-inc-learning-switch" : 
  Benchmark("benchmarks/learning-switch.ivy", "--incremental --enum-naive --conj-arity 1 --disj-arity 3 --whole-space"),

"naive-breadth-learning-switch" : 
  Benchmark("benchmarks/learning-switch.ivy", "--breadth --enum-naive --conj-arity 1 --disj-arity 3 --whole-space"),

"naive-strat2-breadth-learning-switch" : 
  Benchmark("benchmarks/learning-switch.ivy", "--breadth --enum-naive --conj-arity 1 --disj-arity 3 --strat2 --whole-space"),

### Chord

"sat-breadth-chord" : 
  Benchmark("benchmarks/chord.ivy", "--enum-sat --breadth --arity 4 --depth 3 --whole-space"),

"naive-breadth-chord-size3" : 
  Benchmark("benchmarks/chord.ivy", "--enum-naive --breadth --conj-arity 1 --disj-arity 3 --whole-space"),

"naive-inc-chord-size3" : 
  Benchmark("benchmarks/chord.ivy", "--enum-naive --incremental --conj-arity 1 --disj-arity 3 --whole-space"),

"naive-strat2-breadth-chord" : 
  Benchmark("benchmarks/chord.ivy", "--enum-naive --breadth --conj-arity 1 --disj-arity 4 --strat2 --whole-space"),

### Paxos

"naive-paxos-missing1" : 
  Benchmark("benchmarks/paxos_epr_missing1.ivy", "--enum-naive --impl-shape --disj-arity 3 --with-conjs  --whole-space"),

"sat-inc-paxos" : 
  Benchmark("benchmarks/paxos_epr.ivy", "--enum-sat --incremental --arity 4 --depth 3 --whole-space"),

"naive-strat2-breadth-paxos-3" : 
  Benchmark("benchmarks/paxos_epr_3.ivy", "--enum-naive --breadth --conj-arity 1 --disj-arity 3 --strat2 --whole-space"),

"naive-strat2-breadth-paxos-4-r2" : 
  Benchmark("benchmarks/paxos_epr_4_r2.ivy", "--enum-naive --breadth --conj-arity 1 --disj-arity 4 --strat2 --whole-space"),

"naive-strat2-breadth-paxos-4-r3" : 
  Benchmark("benchmarks/paxos_epr_4_r3.ivy", "--enum-naive --breadth --conj-arity 1 --disj-arity 4 --strat2 --whole-space"),

"naive-breadth-paxos-exist-1" :
  Benchmark("benchmarks/paxos_epr_existential_1.ivy", "--enum-naive --breadth --conj-arity 1 --disj-arity 3 --strat-alt --impl-shape --whole-space --start-with-existing-conjectures"),

"naive-full-paxos-exist-1" :
  Benchmark("benchmarks/paxos_epr_full_existential_1.ivy", "--enum-naive --conj-arity 1 --disj-arity 3 --strat-alt --impl-shape --with-conjs --whole-space"),

"naive-full-paxos-exist-1-solve" :
  Benchmark("benchmarks/paxos_epr_full_existential_1.ivy", "--enum-naive --conj-arity 1 --disj-arity 3 --strat-alt --impl-shape --with-conjs"),

# note: this one is a lot easier than naive-breadth-paxos-exist-1
"naive-breadth-paxos-exist-2" :
  Benchmark("benchmarks/paxos_epr_existential_2.ivy", "--enum-naive --breadth --conj-arity 1 --disj-arity 3 --strat-alt --whole-space"),

"sat-breadth-paxos-3" : 
  Benchmark("benchmarks/paxos_epr_3.ivy", "--enum-sat --breadth --arity 3 --depth 3 --whole-space"),

"sat-breadth-paxos-4-r2" : 
  Benchmark("benchmarks/paxos_epr_4_r2.ivy", "--enum-sat --breadth --arity 4 --depth 3 --whole-space"),

"sat-breadth-paxos-4-r3" : 
  Benchmark("benchmarks/paxos_epr_4_r3.ivy", "--enum-sat --breadth --arity 4 --depth 3 --whole-space"),
}
