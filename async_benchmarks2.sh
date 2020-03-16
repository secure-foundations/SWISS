parallel "$@" ::: \
  "./run_benchmarks.py naive-strat2-breadth-paxos-4-r3" \
  "./run_benchmarks.py naive-strat2-breadth-paxos-4-r3 --pre-bmc --post-bmc" \
  "./run_benchmarks.py naive-strat2-breadth-paxos-4-r3 --minimal-models" \
  "./run_benchmarks.py naive-strat2-breadth-paxos-4-r3 --minimal-models --pre-bmc --post-bmc" \
  "./run_benchmarks.py naive-breadth-paxos-exist-2" \
  "./run_benchmarks.py naive-breadth-paxos-exist-2 --pre-bmc --post-bmc" \
  "./run_benchmarks.py naive-breadth-paxos-exist-2 --minimal-models" \
  "./run_benchmarks.py naive-breadth-paxos-exist-2 --minimal-models --pre-bmc --post-bmc" \
  "./run_benchmarks.py naive-breadth-paxos-exist-1" \
  "./run_benchmarks.py naive-breadth-paxos-exist-1 --pre-bmc --post-bmc" \
  "./run_benchmarks.py naive-breadth-paxos-exist-1 --minimal-models" \
  "./run_benchmarks.py naive-breadth-paxos-exist-1 --minimal-models --pre-bmc --post-bmc" \
  "./run_benchmarks.py naive-full-paxos-exist-1" \
  "./run_benchmarks.py naive-full-paxos-exist-1 --pre-bmc --post-bmc" \
  "./run_benchmarks.py naive-full-paxos-exist-1 --minimal-models" \
  "./run_benchmarks.py naive-full-paxos-exist-1 --minimal-models --pre-bmc --post-bmc" \
  "./run_benchmarks.py naive-full-paxos-exist-1-solve" \
  "./run_benchmarks.py naive-strat2-breadth-learning-switch" \
  "./run_benchmarks.py naive-strat2-breadth-learning-switch --pre-bmc --post-bmc" \
  "./run_benchmarks.py naive-strat2-breadth-learning-switch --minimal-models" \
  "./run_benchmarks.py naive-strat2-breadth-learning-switch --minimal-models --pre-bmc --post-bmc" \
