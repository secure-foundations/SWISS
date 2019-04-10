#!/bin/bash

python file_to_json.py $1 > ./src.json
shift
./synthesis "$@" < ./src.json
