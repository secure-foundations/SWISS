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

"breadth-lock-server-with-conjs" :
  Benchmark("benchmarks/lock_server.ivy", "--breadth-with-conjs",
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
      "--breadth --template 0 --conj-arity 1 --disj-arity 3",
      "--finisher --template 0 --conj-arity 1 --disj-arity 5 --depth2-shape"),

# size 5 is 7,482,678,873
"chain":
  Benchmark("benchmarks/chain.ivy", "--with-conjs",
      "--breadth --conj-arity 1 --disj-arity 4",
      "--finisher --conj-arity 1 --disj-arity 5 --depth2-shape"),

"chain6":
  Benchmark("benchmarks/chain.ivy", "--with-conjs",
      "--breadth --conj-arity 1 --disj-arity 4",
      "--finisher --conj-arity 1 --disj-arity 6 --depth2-shape"),

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
      "--finisher --conj-arity 1 --disj-arity 5 --depth2-shape"),

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

"better-template-paxos-breadth4" :
  Benchmark("benchmarks/paxos_epr_full.ivy", "--with-conjs",
      "--breadth --template 0 --conj-arity 1 --disj-arity 4",
      "--breadth --template 1 --conj-arity 1 --disj-arity 3 --strat-alt",
      "--breadth --template 2 --conj-arity 1 --disj-arity 3 --strat-alt",
    ),

"better-template-paxos-finisher" :
  Benchmark("benchmarks/paxos_epr_full_existential_1_threetemplates.ivy", "--with-conjs",
      "--finisher --template 0 --conj-arity 1 --disj-arity 5 --strat-alt --depth2-shape",
      "--finisher --template 1 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape",
      "--finisher --template 2 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape",
    ),

"better_template_breadth_paxos_epr_minus_easy_existential" :
  Benchmark("benchmarks/paxos_epr_minus_easy_existential.ivy", "--with-conjs",
      "--breadth --template 0 --conj-arity 1 --disj-arity 4",
      "--breadth --template 1 --conj-arity 1 --disj-arity 3 --strat-alt",
      "--breadth --template 2 --conj-arity 1 --disj-arity 3 --strat-alt",
    ),

"better_template_finisher_paxos_epr_minus_easy_existential" :
  Benchmark("benchmarks/paxos_epr_minus_easy_existential.ivy", "--with-conjs",
      "--finisher --template 0 --conj-arity 1 --disj-arity 4",
      "--finisher --template 1 --conj-arity 1 --disj-arity 3 --strat-alt",
      "--finisher --template 2 --conj-arity 1 --disj-arity 3 --strat-alt",
    ),

"better-template-flexible-paxos" :
  Benchmark("benchmarks/paxos_flexible_full.ivy", "--with-conjs",
      "--breadth --template 0 --conj-arity 1 --disj-arity 4",
      "--breadth --template 1 --conj-arity 1 --disj-arity 3 --strat-alt",
      "--breadth --template 2 --conj-arity 1 --disj-arity 3 --strat-alt",
      "--finisher --template 0 --conj-arity 1 --disj-arity 5 --strat-alt --depth2-shape",
      "--finisher --template 1 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape",
      "--finisher --template 2 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape",
  ),

"better-template-multi-paxos" :
  Benchmark("benchmarks/paxos_multi_full.ivy", "--with-conjs",
      "--breadth --template 0 --conj-arity 1 --disj-arity 4",
      "--breadth --template 1 --conj-arity 1 --disj-arity 3 --strat-alt",
      "--breadth --template 2 --conj-arity 1 --disj-arity 3 --strat-alt",
      "--finisher --template 0 --conj-arity 1 --disj-arity 5 --strat-alt --depth2-shape",
      "--finisher --template 1 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape",
      "--finisher --template 2 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape",
  ),

"better-template-vertical-paxos" :
  Benchmark("benchmarks/stretch_vertical_paxos_epr.ivy", "--with-conjs",
      "--breadth --template 0 --conj-arity 1 --disj-arity 4",
      "--breadth --template 2 --conj-arity 1 --disj-arity 5 --strat-alt",
      "--breadth --template 3 --conj-arity 1 --disj-arity 3 --strat-alt",

      "--finisher --template 0 --conj-arity 1 --disj-arity 5 --strat-alt --depth2-shape",
      "--finisher --template 2 --conj-arity 1 --disj-arity 5 --strat-alt --depth2-shape",
      "--finisher --template 3 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape",
  ),

"better-template-stoppable-paxos" :
  Benchmark("benchmarks/stretch_stoppable_paxos_epr.ivy", "--with-conjs",
      "--breadth --template 0 --conj-arity 1 --disj-arity 4",
      "--breadth --template 1 --conj-arity 1 --disj-arity 3 --strat-alt",
      "--breadth --template 2 --conj-arity 1 --disj-arity 4 --strat-alt",
      "--finisher --template 0 --conj-arity 1 --disj-arity 5 --strat-alt --depth2-shape",
      "--finisher --template 1 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape",
      "--finisher --template 2 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape"
   ),


#"better-template-paxos2" :
#  Benchmark("benchmarks/paxos_epr_full.ivy", "--with-conjs",
#      "--breadth --template 0 --conj-arity 1 --disj-arity 3",
#      "--breadth --template 1 --conj-arity 1 --disj-arity 3 --strat-alt",
#      "--breadth --template 4 --conj-arity 1 --disj-arity 3 --strat-alt",
#      "--finisher --template 0 --conj-arity 1 --disj-arity 5 --strat-alt --depth2-shape",
#      "--finisher --template 1 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape",
#      "--finisher --template 4 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape",
#    ),

"final-leader-election" :
  Benchmark("benchmarks/final-leader-election.ivy",
  *("""
  --with-conjs
--breadth --template 0 --conj-arity 1 --disj-arity 4
--breadth --template 1 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 2 --conj-arity 1 --disj-arity 4
--breadth --template 3 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 4 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 5 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 6 --conj-arity 1 --disj-arity 4
--breadth --template 7 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 8 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 9 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 10 --conj-arity 1 --disj-arity 4
--breadth --template 11 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 12 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 13 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 14 --conj-arity 1 --disj-arity 4
--breadth --template 15 --conj-arity 1 --disj-arity 4 --strat-alt
--finisher --template 0 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 1 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 2 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 3 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 4 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 5 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 6 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 7 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 8 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 9 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 10 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 11 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 12 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 13 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 14 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 15 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
""".split('\n'))),

"final-simple-de-lock" :
  Benchmark("benchmarks/final-simple-de-lock.ivy",
  *(""" 
  --with-conjs
--breadth --template 0 --conj-arity 1 --disj-arity 4
--breadth --template 1 --conj-arity 1 --disj-arity 4 --strat-alt
--finisher --template 0 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 1 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
""".split('\n'))),

"final-2pc" :
  Benchmark("benchmarks/final-2pc.ivy",
  *(""" 
--with-conjs
--breadth --template 0 --conj-arity 1 --disj-arity 4
--breadth --template 1 --conj-arity 1 --disj-arity 4 --strat-alt
--finisher --template 0 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 1 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
""".split('\n'))),

"final-lock-server" :
  Benchmark("benchmarks/final-lock-server.ivy",
  *(""" 
  --with-conjs
--breadth --template 0 --conj-arity 1 --disj-arity 4
--breadth --template 1 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 2 --conj-arity 1 --disj-arity 4
--breadth --template 3 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 4 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 5 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 6 --conj-arity 1 --disj-arity 4
--breadth --template 7 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 8 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 9 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 10 --conj-arity 1 --disj-arity 4
--breadth --template 11 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 12 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 13 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 14 --conj-arity 1 --disj-arity 4
--breadth --template 15 --conj-arity 1 --disj-arity 4 --strat-alt
--finisher --template 0 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 1 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 2 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 3 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 4 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 5 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 6 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 7 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 8 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 9 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 10 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 11 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 12 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 13 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 14 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 15 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
""".split('\n'))),


"final-learning-switch" :
  Benchmark("benchmarks/final-learning-switch.ivy",
  *(""" 
  --with-conjs
--breadth --template 0 --conj-arity 1 --disj-arity 4
--breadth --template 1 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 2 --conj-arity 1 --disj-arity 4
--breadth --template 3 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 4 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 5 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 6 --conj-arity 1 --disj-arity 4
--breadth --template 7 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 8 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 9 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 10 --conj-arity 1 --disj-arity 4
--breadth --template 11 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 12 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 13 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 14 --conj-arity 1 --disj-arity 4
--breadth --template 15 --conj-arity 1 --disj-arity 4 --strat-alt
--finisher --template 0 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 1 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 2 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 3 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 4 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 5 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 6 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 7 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 8 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 9 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 10 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 11 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 12 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 13 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 14 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 15 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
""".split('\n'))),


"final-decentralized-lock" :
  Benchmark("benchmarks/final-decentralized-lock.ivy",
  *(""" 
  --with-conjs
--breadth --template 0 --conj-arity 1 --disj-arity 4
--breadth --template 1 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 2 --conj-arity 1 --disj-arity 4
--breadth --template 3 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 4 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 5 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 6 --conj-arity 1 --disj-arity 4
--breadth --template 7 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 8 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 9 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 10 --conj-arity 1 --disj-arity 4
--breadth --template 11 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 12 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 13 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 14 --conj-arity 1 --disj-arity 4
--breadth --template 15 --conj-arity 1 --disj-arity 4 --strat-alt
--finisher --template 0 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 1 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 2 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 3 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 4 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 5 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 6 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 7 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 8 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 9 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 10 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 11 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 12 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 13 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 14 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 15 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
""".split('\n'))),

"final-chord" :
  Benchmark("benchmarks/final-chord.ivy",
  *(""" 
--with-conjs
--breadth --template 0 --conj-arity 1 --disj-arity 4
--breadth --template 1 --conj-arity 1 --disj-arity 4 --strat-alt
--finisher --template 0 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 1 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
""".split('\n'))),

"final-paxos" :
  Benchmark("benchmarks/final-paxos.ivy",
  *("""
--with-conjs
--breadth --template 0 --conj-arity 1 --disj-arity 4
--breadth --template 1 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 2 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 3 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 4 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 5 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 6 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 7 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 8 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 9 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 10 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 11 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 12 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 13 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 14 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 15 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 16 --conj-arity 1 --disj-arity 4
--breadth --template 17 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 18 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 19 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 20 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 21 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 22 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 23 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 24 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 25 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 26 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 27 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 28 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 29 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 30 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 31 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 32 --conj-arity 1 --disj-arity 4
--breadth --template 33 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 34 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 35 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 36 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 37 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 38 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 39 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 40 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 41 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 42 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 43 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 44 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 45 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 46 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 47 --conj-arity 1 --disj-arity 4 --strat-alt
--finisher --template 0 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 1 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 2 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 3 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 4 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 5 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 6 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 7 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 8 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 9 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 10 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 11 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 12 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 13 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 14 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 15 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 16 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 17 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 18 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 19 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 20 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 21 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 22 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 23 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 24 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 25 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 26 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 27 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 28 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 29 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 30 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 31 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 32 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 33 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 34 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 35 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 36 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 37 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 38 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 39 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 40 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 41 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 42 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 43 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 44 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 45 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 46 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 47 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
""".split('\n'))),

"final-multi-paxos" :
  Benchmark("benchmarks/final-multi-paxos.ivy",
  *("""
--with-conjs
--breadth --template 0 --conj-arity 1 --disj-arity 4
--breadth --template 1 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 2 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 3 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 4 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 5 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 6 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 7 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 8 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 9 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 10 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 11 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 12 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 13 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 14 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 15 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 16 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 17 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 18 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 19 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 20 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 21 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 22 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 23 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 24 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 25 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 26 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 27 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 28 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 29 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 30 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 31 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 32 --conj-arity 1 --disj-arity 4
--breadth --template 33 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 34 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 35 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 36 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 37 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 38 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 39 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 40 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 41 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 42 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 43 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 44 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 45 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 46 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 47 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 48 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 49 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 50 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 51 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 52 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 53 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 54 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 55 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 56 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 57 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 58 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 59 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 60 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 61 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 62 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 63 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 64 --conj-arity 1 --disj-arity 4
--breadth --template 65 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 66 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 67 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 68 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 69 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 70 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 71 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 72 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 73 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 74 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 75 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 76 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 77 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 78 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 79 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 80 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 81 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 82 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 83 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 84 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 85 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 86 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 87 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 88 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 89 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 90 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 91 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 92 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 93 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 94 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 95 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 96 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 97 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 98 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 99 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 100 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 101 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 102 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 103 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 104 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 105 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 106 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 107 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 108 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 109 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 110 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 111 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 112 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 113 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 114 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 115 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 116 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 117 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 118 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 119 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 120 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 121 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 122 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 123 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 124 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 125 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 126 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 127 --conj-arity 1 --disj-arity 4 --strat-alt
--finisher --template 0 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 1 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 2 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 3 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 4 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 5 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 6 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 7 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 8 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 9 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 10 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 11 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 12 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 13 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 14 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 15 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 16 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 17 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 18 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 19 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 20 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 21 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 22 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 23 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 24 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 25 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 26 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 27 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 28 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 29 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 30 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 31 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 32 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 33 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 34 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 35 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 36 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 37 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 38 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 39 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 40 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 41 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 42 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 43 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 44 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 45 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 46 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 47 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 48 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 49 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 50 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 51 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 52 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 53 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 54 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 55 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 56 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 57 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 58 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 59 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 60 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 61 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 62 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 63 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 64 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 65 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 66 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 67 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 68 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 69 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 70 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 71 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 72 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 73 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 74 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 75 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 76 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 77 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 78 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 79 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 80 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 81 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 82 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 83 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 84 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 85 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 86 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 87 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 88 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 89 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 90 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 91 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 92 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 93 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 94 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 95 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 96 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 97 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 98 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 99 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 100 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 101 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 102 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 103 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 104 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 105 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 106 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 107 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 108 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 109 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 110 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 111 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 112 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 113 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 114 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 115 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 116 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 117 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 118 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 119 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 120 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 121 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 122 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 123 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 124 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 125 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 126 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 127 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
""".split('\n'))),


"final-flexible-paxos" :
  Benchmark("benchmarks/final-flexible-paxos.ivy",
  *("""
--with-conjs
--breadth --template 0 --conj-arity 1 --disj-arity 4
--breadth --template 1 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 2 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 3 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 4 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 5 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 6 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 7 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 8 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 9 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 10 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 11 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 12 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 13 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 14 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 15 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 16 --conj-arity 1 --disj-arity 4
--breadth --template 17 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 18 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 19 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 20 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 21 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 22 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 23 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 24 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 25 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 26 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 27 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 28 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 29 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 30 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 31 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 32 --conj-arity 1 --disj-arity 4
--breadth --template 33 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 34 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 35 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 36 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 37 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 38 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 39 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 40 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 41 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 42 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 43 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 44 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 45 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 46 --conj-arity 1 --disj-arity 4 --strat-alt
--breadth --template 47 --conj-arity 1 --disj-arity 4 --strat-alt
--finisher --template 0 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 1 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 2 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 3 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 4 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 5 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 6 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 7 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 8 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 9 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 10 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 11 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 12 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 13 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 14 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 15 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 16 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 17 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 18 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 19 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 20 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 21 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 22 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 23 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 24 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 25 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 26 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 27 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 28 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 29 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 30 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 31 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 32 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 33 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 34 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 35 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 36 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 37 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 38 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 39 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 40 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 41 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 42 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 43 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 44 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 45 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 46 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
--finisher --template 47 --conj-arity 1 --disj-arity 6 --strat-alt --depth2-shape
""".split('\n'))),
}
