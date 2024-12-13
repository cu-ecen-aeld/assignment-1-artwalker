#!/bin/bash

# Check if both arguments are provided
if [ $# -ne 2 ]; then
    echo "Usage: $0 <writefile> <writestr>"
    exit 1
fi

writefile="$1"
writestr="$2"

# Ensure the directory path for the file exists
mkdir -p "$(dirname "$writefile")"

# Write the content to the file, overwriting if it already exists
if ! echo "$writestr" > "$writefile"; then
    echo "Error: Could not write to file $writefile"
    exit 1
fi

# If we reach here, the operation was successful
echo "Successfully wrote '$writestr' to $writefile"
