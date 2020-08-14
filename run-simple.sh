#!/bin/bash

set -e

IVY_FILE=$1
TEMP=$(mktemp)

if [ ${IVY_FILE: -4} == ".pyv" ]
then
  python3 file_mypyvy_to_json.py $IVY_FILE > $TEMP
else
  python file_to_json.py $IVY_FILE > $TEMP
fi

shift
./synthesis --input-module $TEMP "$@"
#python3 src/driver.py "$TEMP" "$@"
