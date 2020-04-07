#!/bin/bash

set -e

IVY_FILE=$1
shift

TEMP=$(mktemp)
python file_to_json.py $IVY_FILE > $TEMP
#./synthesis "$@" < $TEMP
python3 scripts/driver.py "$IVY_FILE" "$TEMP" "$@"
