#!/bin/bash

set -e

make

if [[ -z "${SYNTHESIS_LOGDIR}" ]]; then
  DT=$(date +"%Y-%m-%d_%H.%M.%S")
  if [[ -z "${SCRATCH}" ]]; then
    LOGDIR=$(mktemp -d "./logs/log.$DT-XXXXXXXXX")
  else
    LOGDIR=$(mktemp -d "$SCRATCH/log.$DT-XXXXXXXXX")
  fi
else
  LOGDIR=$SYNTHESIS_LOGDIR
fi

echo "logging to $LOGDIR/driver"

echo "python3 scripts/driver.py $@" >> $LOGDIR/driver
echo "" >> $LOGDIR/driver

z3 --version >> $LOGDIR/driver
echo "" >> $LOGDIR/driver
git rev-parse --verify HEAD >> $LOGDIR/driver
echo "" >> $LOGDIR/driver
git diff >> $LOGDIR/driver
echo "" >> $LOGDIR/driver

set +e

{ time python3 ./scripts/driver.py $@ --logdir $LOGDIR ; } 2>&1 | tee -a $LOGDIR/driver

echo "logged to $LOGDIR/driver"

exit $RETCODE
