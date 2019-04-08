#!/bin/bash

set -e

make

DT=$(date +"%Y-%m-%d_%H.%M.%S")
LOGFILE=$(mktemp "./logs/log.$DT-XXXXXXXXX")

echo "logging to $LOGFILE"

echo "./run.sh $@" >> $LOGFILE
echo "" >> $LOGFILE

git rev-parse --verify HEAD >> $LOGFILE
echo "" >> $LOGFILE
git diff >> $LOGFILE
echo "" >> $LOGFILE

set +e

{ time ./run.sh $@ ; } 2>&1 | tee -a $LOGFILE
