#!/bin/bash

echo_and_run() { echo "$*" ; "$@" ; }

if [ "$1" = "naive-leader-election" ]; then
  echo_and_run ./save.sh benchmarks/leader-election.ivy --enum-naive --conj-arity 3 --disj-arity 3
fi
if [ "$1" = "naive-inc-leader-election" ]; then
  echo_and_run ./save.sh benchmarks/leader-election.ivy --incremental --enum-naive --conj-arity 1 --disj-arity 3
fi
if [ "$1" = "naive-breadth-leader-election" ]; then
  echo_and_run ./save.sh benchmarks/leader-election.ivy --breadth --enum-naive --conj-arity 1 --disj-arity 3
fi
if [ "$1" = "naive-strat2-breadth-leader-election" ]; then
  echo_and_run ./save.sh benchmarks/leader-election.ivy --breadth --enum-naive --conj-arity 1 --disj-arity 3 --strat2
fi
if [ "$1" = "sat-leader-election" ]; then
  echo_and_run ./save.sh benchmarks/leader-election.ivy --enum-sat --arity 3 --depth 4
fi
if [ "$1" = "sat-inc-leader-election" ]; then
  echo_and_run ./save.sh benchmarks/leader-election.ivy --incremental --enum-sat --arity 3 --depth 3
fi
if [ "$1" = "sat-breadth-leader-election" ]; then
  echo_and_run ./save.sh benchmarks/leader-election.ivy --breadth --enum-sat --arity 3 --depth 3
fi




if [ "$1" = "sat-inc-learning-switch" ]; then
  echo_and_run ./save.sh benchmarks/learning-switch.ivy --incremental --enum-sat --arity 3 --depth 3
fi
if [ "$1" = "sat-breadth-learning-switch" ]; then
  echo_and_run ./save.sh benchmarks/learning-switch.ivy --breadth --enum-sat --arity 3 --depth 3
fi
if [ "$1" = "naive-inc-learning-switch" ]; then
  echo_and_run ./save.sh benchmarks/learning-switch.ivy --incremental --enum-naive --conj-arity 1 --disj-arity 3
fi
if [ "$1" = "naive-breadth-learning-switch" ]; then
  echo_and_run ./save.sh benchmarks/learning-switch.ivy --breadth --enum-naive --conj-arity 1 --disj-arity 3
fi
if [ "$1" = "naive-strat2-breadth-learning-switch" ]; then
  echo_and_run ./save.sh benchmarks/learning-switch.ivy --breadth --enum-naive --conj-arity 1 --disj-arity 3 --strat2
fi

if [ "$1" = "sat-breadth-chord" ]; then
  echo_and_run ./save.sh benchmarks/chord.ivy --enum-sat --breadth --arity 4 --depth 3
fi
if [ "$1" = "naive-breadth-chord-size3" ]; then
  echo_and_run ./save.sh benchmarks/chord.ivy --enum-naive --breadth --conj-arity 1 --disj-arity 3
fi
if [ "$1" = "naive-inc-chord-size3" ]; then
  echo_and_run ./save.sh benchmarks/chord.ivy --enum-naive --incremental --conj-arity 1 --disj-arity 3
fi
if [ "$1" = "naive-strat2-breadth-chord" ]; then
  echo_and_run ./save.sh benchmarks/chord.ivy --enum-naive --breadth --conj-arity 1 --disj-arity 4 --strat2
fi


if [ "$1" = "naive-paxos-missing1" ]; then
  echo_and_run ./save.sh benchmarks/paxos_epr_missing1.ivy --enum-naive --impl-shape --disj-arity 3 --with-conjs 
fi

if [ "$1" = "sat-inc-paxos" ]; then
  echo_and_run ./save.sh benchmarks/paxos_epr.ivy --enum-sat --incremental --arity 4 --depth 3
fi

if [ "$1" = "naive-breadth-paxos-size3" ]; then
  echo_and_run ./save.sh benchmarks/paxos_epr.ivy --enum-naive --breadth --conj-arity 1 --disj-arity 3
fi
if [ "$1" = "naive-breadth-paxos-size4" ]; then
  echo_and_run ./save.sh benchmarks/paxos_epr.ivy --enum-naive --breadth --conj-arity 1 --disj-arity 4
fi
if [ "$1" = "naive-strat2-breadth-paxos-4-r2" ]; then
  echo_and_run ./save.sh benchmarks/paxos_epr_4_r2.ivy --enum-naive --breadth --conj-arity 1 --disj-arity 4 --strat2
fi
if [ "$1" = "naive-strat2-breadth-paxos-4-r3" ]; then
  echo_and_run ./save.sh benchmarks/paxos_epr_4_r3.ivy --enum-naive --breadth --conj-arity 1 --disj-arity 4 --strat2
fi

