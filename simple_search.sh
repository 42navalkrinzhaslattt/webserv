#!/bin/bash

# Check if a search pattern was provided
if [ $# -eq 0 ]; then
    echo "Usage: $0 <search_pattern> [directory]"
    echo "Example: $0 \"errno\" src"
    echo "If no directory is provided, defaults to src"
    exit 1
fi

# Get the search pattern
PATTERN="$1"

# Set default directory if none provided
if [ $# -gt 1 ]; then
    DIRECTORY="$2"
else
    DIRECTORY="src"
fi

echo "Searching for '$PATTERN' in $DIRECTORY directory..."
echo "----------------------------------------"

# Find all source files
find "$DIRECTORY" -type f -name "*.cpp" -o -name "*.h" -o -name "*.c" -o -name "*.hpp" -o -name "*.cc" | while read -r file; do
    # Search for the pattern in each file
    line_numbers=$(grep -n "$PATTERN" "$file" | cut -d: -f1)
    
    # If pattern found in this file
    if [ -n "$line_numbers" ]; then
        for line_num in $line_numbers; do
            content=$(sed -n "${line_num}p" "$file")
            echo "File: $file"
            echo "Line: $line_num"
            echo "Content: $content"
            echo "----------------------------------------"
        done
    fi
done

# Check if we found any matches
if [ -z "$line_numbers" ]; then
    echo "No matches found for '$PATTERN'"
fi
