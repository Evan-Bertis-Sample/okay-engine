#!/usr/bin/env bash

# Exit on error
set -e

# Directory to format (default = current directory)
TARGET_DIR="${1:-.}"

# Check if clang-format exists
if ! command -v clang-format &> /dev/null; then
    echo "Error: clang-format not found in PATH."
    echo "Install it with: sudo apt install clang-format"
    exit 1
fi

echo "Formatting all .c and .h files under: $TARGET_DIR"
echo

# Find and format files in-place, but skip the okay/vendor directory and any .okay/ directories
find "$TARGET_DIR" -type f \( -name "*.cpp" -o -name "*.hpp" \) -not -path "*/okay/vendor/*" -not -path "*/.okay/*" | while read -r file; do
    echo "Formatting: $file"
    clang-format -i "$file"
done

echo
echo "Done!"