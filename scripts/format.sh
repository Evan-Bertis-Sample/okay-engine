#!/usr/bin/env bash
set -euo pipefail

TARGET_DIR="${1:-.}"

if ! command -v clang-format >/dev/null 2>&1; then
    echo "Error: clang-format not found in PATH."
    exit 1
fi

find "$TARGET_DIR" \
    -type d \( -path "*/okay/vendor" -o -path "*/.okay" \) -prune \
    -o -type f \( -name "*.cpp" -o -name "*.hpp" \) -print0 |
xargs -0 clang-format -i