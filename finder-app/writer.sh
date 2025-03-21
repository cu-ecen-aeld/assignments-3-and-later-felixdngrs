#!/bin/bash

# Check the number of arguments
if [ "$#" -ne 2 ]; then
    echo "Error: Two arguments are required." >&2
    echo "Usage: $0 <file path> <text to write>" >&2
    exit 1
fi

writefile="$1"
writestr="$2"

dirpath="$(dirname "$writefile")"

# Create directory if it does not exist
if ! mkdir -p "$dirpath"; then
    echo "Error: Could not create directory: $dirpath" >&2
    exit 1
fi

# Create or overwrite file with content
if ! echo "$writestr" > "$writefile"; then
    echo "Error: Could not write to file: $writefile" >&2
	exit 1
fi

exit 0
