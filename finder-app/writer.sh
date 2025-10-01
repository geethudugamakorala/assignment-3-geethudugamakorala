#!/bin/bash

if [ $# -lt 2 ]; then
    echo "Error: Missing parameters. Usage: $0 <writefile> <writestr>"
    exit 1
fi

WRITEFILE="$1"
WRITESTR="$2"
DIRPATH=$(dirname "$WRITEFILE")
mkdir -p "$DIRPATH"

echo "$WRITESTR" > "$WRITEFILE"
if [ $? -ne 0 ]; then
    echo "Error: Could not create file '$WRITEFILE'."
    exit 1
fi