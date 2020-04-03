#!/bin/bash

set -e

TEMP=$(mktemp)
python file_to_json.py $1 > $TEMP
shift
./synthesis "$@" < $TEMP
#python3 src/driver.py "$TEMP" "$@"
