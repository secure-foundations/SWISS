parallel "$@" ./run_benchmarks.py ::: \
  "naive-strat2-breadth-paxos-4-r3" \
  "naive-strat2-breadth-paxos-4-r3 --pre-bmc --post-bmc" \
  "naive-strat2-breadth-paxos-4-r3 --minimal-models" \
  "naive-strat2-breadth-paxos-4-r3 --minimal-models --pre-bmc --post-bmc" \
  "naive-breadth-paxos-exist-2" \
  "naive-breadth-paxos-exist-2 --pre-bmc --post-bmc" \
  "naive-breadth-paxos-exist-2 --minimal-models" \
  "naive-breadth-paxos-exist-2 --minimal-models --pre-bmc --post-bmc" \
  "naive-breadth-paxos-exist-1" \
  "naive-breadth-paxos-exist-1 --pre-bmc --post-bmc" \
  "naive-breadth-paxos-exist-1 --minimal-models" \
  "naive-breadth-paxos-exist-1 --minimal-models --pre-bmc --post-bmc" \
  "naive-full-paxos-exist-1" \
  "naive-full-paxos-exist-1 --pre-bmc --post-bmc" \
  "naive-full-paxos-exist-1 --minimal-models" \
  "naive-full-paxos-exist-1 --minimal-models --pre-bmc --post-bmc" \
  "naive-full-paxos-exist-1-solve" \
  "naive-strat2-breadth-learning-switch" \
  "naive-strat2-breadth-learning-switch --pre-bmc --post-bmc" \
  "naive-strat2-breadth-learning-switch --minimal-models" \
  "naive-strat2-breadth-learning-switch --minimal-models --pre-bmc --post-bmc" \
