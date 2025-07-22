#!/usr/bin/env bash
set -euo pipefail

echo "🛠  Auto-formatting all source files with clang-format…"

find . \
  -path "./.pio" -prune -o \
  -path "./scripts" -prune -o \
  -type f \( -iname '*.c' -o -iname '*.cpp' -o -iname '*.hpp' -o -iname '*.h' \) -print0 \
| xargs -0 clang-format -i

echo "✅ Formatting complete."
