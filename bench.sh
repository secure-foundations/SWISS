#!/bin/bash

echo_and_run() { echo "$*" ; "$@" ; }

if [ "$1" = "naive-leader-election" ]; then
  echo_and_run ./save.sh benchmarks/leader-election.ivy --enum-naive --conj-arity 3 --disj-arity 3
fi
if [ "$1" = "naive-inc-leader-election" ]; then
  echo_and_run ./save.sh benchmarks/leader-election.ivy --incremental --enum-naive --conj-arity 1 --disj-arity 3
fi
if [ "$1" = "sat-leader-election" ]; then
  echo_and_run ./save.sh benchmarks/leader-election.ivy --enum-sat --arity 3 --depth 4
fi
if [ "$1" = "sat-inc-leader-election" ]; then
  echo_and_run ./save.sh benchmarks/leader-election.ivy --incremental --enum-sat --arity 3 --depth 3
fi



if [ "$1" = "naive-paxos-missing1" ]; then
  echo_and_run ./save.sh benchmarks/paxos_epr_missing1.ivy --enum-naive --impl-shape --disj-arity 3 --with-conjs 
fi

if [ "$1" = "sat-inc-paxos" ]; then
  echo_and_run ./save.sh benchmarks/paxos_epr.ivy --enum-sat --incremental --arity 4 --depth 3
fi



if [ "$1" = "sat-inc-learning-switch" ]; then
  echo_and_run ./save.sh benchmarks/learning-switch.ivy --incremental --enum-sat --arity 3 --depth 3

fi

if [ "$1" = "naive-inc-learning-switch" ]; then
  echo_and_run ./save.sh benchmarks/learning-switch.ivy --incremental --enum-naive --conj-arity 1 --disj-arity 3
fi

