#!/bin/bash

BENCHES=$(python print_all_benchmark_names.py)
#parallel "$@" -k ./run_benchmarks.py ::: $BENCHES
parallel "$@" ./run_benchmarks.py -- $BENCHES
