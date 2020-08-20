#!/bin/bash

set -e

make

DT=$(date +"%Y-%m-%d_%H.%M.%S")

if [[ -z "${SYNTHESIS_LOGFILE}" ]]; then
  if [[ -z "${SCRATCH}" ]]; then
    LOGFILE=$(mktemp "./logs/log.$DT-XXXXXXXXX")
  else
    LOGFILE=$(mktemp "$SCRATCH/log.$DT-XXXXXXXXX")
  fi
else
  LOGFILE=$SYNTHESIS_LOGFILE
fi

echo "logging to $LOGFILE"

echo "./run.sh $@" >> $LOGFILE
echo "" >> $LOGFILE

z3 --version >> $LOGFILE
echo "" >> $LOGFILE
git rev-parse --verify HEAD >> $LOGFILE
echo "" >> $LOGFILE
git diff >> $LOGFILE
echo "" >> $LOGFILE

set +e

{ time ./run.sh $@ --logfile $LOGFILE ; } 2>&1 | tee -a $LOGFILE

echo "logged to $LOGFILE"

exit $RETCODE
