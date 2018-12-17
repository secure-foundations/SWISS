#!/bin/bash

python file_to_json.py $1 > ./src.json
./synthesis < ./src.json
