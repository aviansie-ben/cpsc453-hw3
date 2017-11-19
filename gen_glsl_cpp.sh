#!/bin/bash

INPUT_FILE=$1
OUTPUT_FILE=$2
VAR_NAME=$3
INCLUDE_FILE=$4

mkdir -p $(dirname $OUTPUT_FILE)
echo "#include \"$INCLUDE_FILE\"" > $OUTPUT_FILE
echo -n "const char* $VAR_NAME = R\"####(" >> $OUTPUT_FILE
cat $INPUT_FILE >> $OUTPUT_FILE
echo ")####\";" >> $OUTPUT_FILE
