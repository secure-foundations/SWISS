#!/bin/bash

set -e

ARGS=$(python print_benchmark_args.py "$1")
echo "./save.sh $ARGS"
./save.sh $ARGS
