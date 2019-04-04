#!/bin/bash

python file_to_json.py $1 > ./src.json
./synthesis $2 $3 < ./src.json
