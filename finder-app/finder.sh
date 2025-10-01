#!/bin/bash

if [ $# -ne 2 ]; then
    echo "Error: Missing parameters. Usage: $0 <filesdir> <searchstr>"
    exit 1
fi
FILESDIR="$1"
SEARCHSTR="$2"
if [ ! -d "$FILESDIR" ]; then
    echo "Error: '$FILESDIR' is not a directory."
    exit 1
fi
NUM_FILES=$(find "$FILESDIR" -type f | wc -l)
NUM_MATCHES=$(find "$FILESDIR" -type f -exec grep -H "$SEARCHSTR" {} \; | wc -l)
echo "The number of files are $NUM_FILES and the number of matching lines are $NUM_MATCHES"