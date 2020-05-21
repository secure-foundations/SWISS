class Benchmark(object):
  def __init__(self, ivy_file, *args):
    self.ivy_file = ivy_file
    self.args = args

BENCHMARKS = {

### Leader election

"lock-server" :
  Benchmark("benchmarks/lock_server.ivy", "--with-conjs",
      "--finisher --conj-arity 1 --disj-arity 2"),

"breadth-lock-server" :
  Benchmark("benchmarks/lock_server.ivy",
      "--breadth --conj-arity 1 --disj-arity 2"),

"breadth-2pc" :
  Benchmark("benchmarks/2PC.ivy",
      "--breadth --conj-arity 1 --disj-arity 3"),

"leader-election" :
  Benchmark("benchmarks/leader-election.ivy",
      "--finisher --conj-arity 3 --disj-arity 3"),

"leader-election-depth2" :
  Benchmark("benchmarks/leader-election.ivy",
      "--finisher --disj-arity 5 --depth2-shape"),

"inc-leader-election" : 
  Benchmark("benchmarks/leader-election.ivy",
      "--incremental --conj-arity 1 --disj-arity 3"),

"breadth-leader-election" : 
  Benchmark("benchmarks/leader-election.ivy",
      "--breadth --conj-arity 1 --disj-arity 3"),

"sat-leader-election" : 
  Benchmark("benchmarks/leader-election.ivy", "--enum-sat",
      "--finisher --arity 3 --depth 4 --conj"),

"sat-inc-leader-election" : 
  Benchmark("benchmarks/leader-election.ivy", "--enum-sat",
      "--incremental --arity 3 --depth 3"),

"sat-breadth-leader-election" : 
  Benchmark("benchmarks/leader-election.ivy", "--enum-sat",
      "--breadth --arity 3 --depth 3"),

### Learning switch

"sat-inc-learning-switch" : 
  Benchmark("benchmarks/learning-switch.ivy", "--enum-sat",
      "--incremental --arity 3 --depth 3"),

"sat-breadth-learning-switch" : 
  Benchmark("benchmarks/learning-switch.ivy", "--enum-sat",
      "--breadth --arity 3 --depth 3"),

"inc-learning-switch" : 
  Benchmark("benchmarks/learning-switch.ivy",
      "--incremental --conj-arity 1 --disj-arity 3"),

"breadth-learning-switch" : 
  Benchmark("benchmarks/learning-switch.ivy",
      "--breadth --conj-arity 1 --disj-arity 3"),

### Chord

"sat-breadth-chord" : 
  Benchmark("benchmarks/chord.ivy", "--enum-sat",
      "--breadth --arity 4 --depth 3"),

"breadth-chord-size3" : 
  Benchmark("benchmarks/chord.ivy",
      "--breadth --conj-arity 1 --disj-arity 3"),

"inc-chord-size3" : 
  Benchmark("benchmarks/chord.ivy",
      "--incremental --conj-arity 1 --disj-arity 3"),

"breadth-chord" : 
  Benchmark("benchmarks/chord.ivy",
      "--breadth --conj-arity 1 --disj-arity 4"),

"finisher-chord-last-2" : 
  Benchmark("benchmarks/chord-last-2.ivy",
      "--finisher --depth2-shape --disj-arity 6"),

### Paxos

"sat-inc-paxos" : 
  Benchmark("benchmarks/paxos_epr.ivy", "--enum-sat",
      "--incremental --arity 4 --depth 3"),

"breadth-paxos-3" : 
  Benchmark("benchmarks/paxos_epr_3.ivy",
      "--breadth --conj-arity 1 --disj-arity 3"),

"breadth-paxos-4-r2" : 
  Benchmark("benchmarks/paxos_epr_4_r2.ivy",
      "--breadth --conj-arity 1 --disj-arity 4"),

"breadth-paxos-4-r3" : 
  Benchmark("benchmarks/paxos_epr_4_r3.ivy",
      "--breadth --conj-arity 1 --disj-arity 4"),

"breadth-paxos-exist-1" :
  Benchmark("benchmarks/paxos_epr_existential_1.ivy",
      #"--start-with-existing-conjectures",
      "--breadth --conj-arity 1 --disj-arity 3 --strat-alt --impl-shape"),

"finisher-paxos-exist-1" : # the hard one
  Benchmark("benchmarks/paxos_epr_full_existential_1.ivy",
      "--with-conjs",
      "--finisher --conj-arity 1 --disj-arity 3 --strat-alt --impl-shape"),

"finisher-paxos-exist-1-depth2" : # the hard one
  Benchmark("benchmarks/paxos_epr_full_existential_1.ivy",
      "--with-conjs",
      "--finisher --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape"),

"breadth-paxos-exist-2" : # the easy one
  Benchmark("benchmarks/paxos_epr_existential_2.ivy", 
      "--breadth --conj-arity 1 --disj-arity 3 --strat-alt"),

"sat-breadth-paxos-3" : 
  Benchmark("benchmarks/paxos_epr_3.ivy", "--enum-sat",
      "--breadth --arity 3 --depth 3"),

"sat-breadth-paxos-4-r2" : 
  Benchmark("benchmarks/paxos_epr_4_r2.ivy", "--enum-sat",
      "--breadth --arity 4 --depth 3"),

"sat-breadth-paxos-4-r3" : 
  Benchmark("benchmarks/paxos_epr_4_r3.ivy", "--enum-sat",
      "--breadth --arity 4 --depth 3"),

"full-paxos" :
  Benchmark("benchmarks/paxos_epr_full.ivy", "--with-conjs",
      "--breadth --template 0 --conj-arity 1 --disj-arity 4",
      "--breadth --template 1 --conj-arity 1 --disj-arity 3 --strat-alt",
      "--finisher --template 2 --conj-arity 1 --disj-arity 3 --strat-alt --impl-shape"),

"full-multi-paxos" :
  Benchmark("benchmarks/paxos_multi_full.ivy", "--with-conjs",
      "--breadth --template 0 --conj-arity 1 --disj-arity 4",
      "--breadth --template 1 --conj-arity 1 --disj-arity 3 --strat-alt",
      "--finisher --template 2 --conj-arity 1 --disj-arity 3 --strat-alt --impl-shape"),

"full-flexible-paxos" :
  Benchmark("benchmarks/paxos_flexible_full.ivy", "--with-conjs",
      "--breadth --template 0 --conj-arity 1 --disj-arity 4",
      "--breadth --template 1 --conj-arity 1 --disj-arity 3 --strat-alt",
      "--finisher --template 2 --conj-arity 1 --disj-arity 3 --strat-alt --impl-shape"),

"full-paxos-depth2" :
  Benchmark("benchmarks/paxos_epr_full.ivy", "--with-conjs",
      "--breadth --template 0 --conj-arity 1 --disj-arity 4",
      "--breadth --template 1 --conj-arity 1 --disj-arity 3 --strat-alt",
      "--finisher --template 2 --conj-arity 1 --disj-arity 6 --depth2-shape"),

"full-flexible-paxos-depth2" :
  Benchmark("benchmarks/paxos_flexible_full.ivy", "--with-conjs",
      "--breadth --template 0 --conj-arity 1 --disj-arity 4",
      "--breadth --template 1 --conj-arity 1 --disj-arity 3 --strat-alt",
      "--finisher --template 2 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape"),

"breadth-flexible-paxos-4-r3" :
  Benchmark("benchmarks/paxos_flexible_full.ivy", "--with-conjs",
      "--breadth --template 0 --conj-arity 1 --disj-arity 4"),

"breadth-flexible-paxos-exist-2" :
  Benchmark("benchmarks/paxos_flexible_full.ivy", "--with-conjs",
      "--breadth --template 1 --conj-arity 1 --disj-arity 3 --strat-alt"),

"finisher-flexible-paxos-exist-1-depth2" :
  Benchmark("benchmarks/paxos_flexible_full.ivy", "--with-conjs",
      "--finisher --template 2 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape"),

"full-multi-paxos-depth2" :
  Benchmark("benchmarks/paxos_multi_full.ivy", "--with-conjs",
      "--breadth --template 0 --conj-arity 1 --disj-arity 4",
      "--breadth --template 1 --conj-arity 1 --disj-arity 3 --strat-alt",
      "--finisher --template 2 --conj-arity 1 --disj-arity 6 --depth2-shape"),

"finisher-paxos-minus-size4" :
  Benchmark("benchmarks/paxos_epr_minus_size4.ivy", "--with-conjs",
      "--finisher --template 0 --conj-arity 1 --disj-arity 4"),

"breadth-paxos-minus-size4" :
  Benchmark("benchmarks/paxos_epr_minus_size4.ivy", "--with-conjs",
      "--breadth --template 0 --conj-arity 1 --disj-arity 4"),

"decentralized-lock" :
  Benchmark("benchmarks/decentralized-lock.ivy", "--with-conjs",
      "--finisher --disj-arity 9 --depth2-shape"),
  
"full-fast-paxos-depth2" :
  Benchmark("benchmarks/stretch_fast_paxos_epr.ivy", "--with-conjs",
      "--breadth --template 0 --conj-arity 1 --disj-arity 4",
      "--breadth --template 1 --conj-arity 1 --disj-arity 3 --strat-alt",
      "--finisher --template 2 --conj-arity 1 --disj-arity 9 --depth2-shape"),

"full-stoppable-paxos-depth2" :
  Benchmark("benchmarks/stretch_stoppable_paxos_epr.ivy", "--with-conjs",
      "--breadth --template 0 --conj-arity 1 --disj-arity 4",
      "--breadth --template 1 --conj-arity 1 --disj-arity 3 --strat-alt",
      "--finisher --template 2 --conj-arity 1 --disj-arity 8 --depth2-shape"),

"full-vertical-paxos-depth2" :
  Benchmark("benchmarks/stretch_vertical_paxos_epr.ivy", "--with-conjs",
      "--breadth --template 0 --conj-arity 1 --disj-arity 4",
      "--breadth --template 1 --conj-arity 1 --disj-arity 3 --strat-alt",
      "--breadth --template 2 --conj-arity 1 --disj-arity 5 --strat-alt",
      "--finisher --template 3 --conj-arity 1 --disj-arity 8 --depth2-shape"),


"breadth-multi-paxos-4-r3" :
  Benchmark("benchmarks/paxos_multi_full.ivy", "--with-conjs",
      "--breadth --template 0 --conj-arity 1 --disj-arity 4"),

"breadth-multi-paxos-exist-2" :
  Benchmark("benchmarks/paxos_multi_full.ivy", "--with-conjs",
      "--breadth --template 1 --conj-arity 1 --disj-arity 3 --strat-alt"),

"finisher-multi-paxos-exist-1-depth2" :
  Benchmark("benchmarks/paxos_multi_full.ivy", "--with-conjs",
      "--finisher --template 2 --conj-arity 1 --disj-arity 6 --depth2-shape"),

"sdl":
  Benchmark("benchmarks/simple-de-lock.ivy", "--with-conjs",
      "--finisher --template 0 --conj-arity 1 --disj-arity 5 --depth2-shape"),

# size 5 is 7482678873
"chain":
  Benchmark("benchmarks/chord.ivy", "--with-conjs",
      "--breadth --conj-arity 1 --disj-arity 4",
      "--finisher --conj-arity 1 --disj-arity 5 --depth2-shape"),

# 43,893,203,822
"fail-delock":
  Benchmark("benchmarks/decentralized-lock.ivy", "--with-conjs",
      "--finisher --template 0 --conj-arity 1 --disj-arity 5 --depth2-shape"),

# 603,223,828,972
"fail-chord":
  Benchmark("benchmarks/chord.ivy", "--with-conjs",
      "--breadth --conj-arity 1 --disj-arity 4",
      "--finisher --template 0 --conj-arity 1 --disj-arity 5 --depth2-shape"),
  

"fail-full-stoppable-paxos-depth2" :
  Benchmark("benchmarks/stretch_stoppable_paxos_epr.ivy", "--with-conjs",
      "--breadth --template 0 --conj-arity 1 --disj-arity 4",
      "--breadth --template 1 --conj-arity 1 --disj-arity 3 --strat-alt",
      "--finisher --template 2 --conj-arity 1 --disj-arity 6 --depth2-shape"),

"fail-full-vertical-paxos-depth2" :
  Benchmark("benchmarks/stretch_vertical_paxos_epr.ivy", "--with-conjs",
      "--breadth --template 0 --conj-arity 1 --disj-arity 4",
      "--breadth --template 1 --conj-arity 1 --disj-arity 3 --strat-alt",
      "--breadth --template 2 --conj-arity 1 --disj-arity 5 --strat-alt",
      "--finisher --template 3 --conj-arity 1 --disj-arity 6 --depth2-shape"),

"breadth_paxos_epr_minus_easy_existential" :
  Benchmark("benchmarks/paxos_epr_minus_easy_existential.ivy", "--with-conjs",
      "--breadth --template 1 --conj-arity 1 --disj-arity 3 --strat-alt"),

"finisher_paxos_epr_minus_easy_existential" :
  Benchmark("benchmarks/paxos_epr_minus_easy_existential.ivy", "--with-conjs",
      "--finisher --template 1 --conj-arity 1 --disj-arity 3 --strat-alt"),

"chord-gimme-1":
  Benchmark("benchmarks/chord-gimme-1.ivy", "--with-conjs",
      "--breadth --conj-arity 1 --disj-arity 4",
      "--finisher --conj-arity 1 --disj-arity 4"),

"decentralized-lock-gimme-1":
  Benchmark("benchmarks/decentralized-lock-gimme-1.ivy", "--with-conjs",
      "--finisher --conj-arity 1 --disj-arity 7 --depth2-shape"),

"full-paxos-mini" :
  Benchmark("benchmarks/paxos_epr_full.ivy", "--with-conjs",
      "--breadth --template 0 --conj-arity 1 --disj-arity 3",
      "--breadth --template 1 --conj-arity 1 --disj-arity 3 --strat-alt",
      "--finisher --template 2 --conj-arity 1 --disj-arity 3 --strat-alt --impl-shape"),

"better-template-paxos" :
  Benchmark("benchmarks/paxos_epr_full.ivy", "--with-conjs",
      "--breadth --template 0 --conj-arity 1 --disj-arity 3",
      "--breadth --template 1 --conj-arity 1 --disj-arity 3 --strat-alt",
      "--breadth --template 2 --conj-arity 1 --disj-arity 3 --strat-alt",
      "--finisher --template 0 --conj-arity 1 --disj-arity 5 --strat-alt --depth2-shape",
      "--finisher --template 1 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape",
      "--finisher --template 2 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape",
    ),

"better-template-paxos2" :
  Benchmark("benchmarks/paxos_epr_full.ivy", "--with-conjs",
      "--breadth --template 0 --conj-arity 1 --disj-arity 3",
      "--breadth --template 1 --conj-arity 1 --disj-arity 3 --strat-alt",
      "--breadth --template 4 --conj-arity 1 --disj-arity 3 --strat-alt",
      "--finisher --template 0 --conj-arity 1 --disj-arity 5 --strat-alt --depth2-shape",
      "--finisher --template 1 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape",
      "--finisher --template 4 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape",
    ),

}
