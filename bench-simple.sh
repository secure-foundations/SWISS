#!/bin/bash

set -e

ARGS=$(python print_benchmark_args.py "$@")
shift
echo "./save.sh $ARGS"
./save-simple.sh $ARGS
