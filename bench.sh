#!/bin/bash

set -e

ARGS=$(python print_benchmark_args.py "$1")
shift
echo "./save.sh $ARGS $@"
./save.sh $ARGS "$@"
