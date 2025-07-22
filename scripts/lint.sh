#!/usr/bin/env bash
set -euo pipefail

echo "🔎 Running lint checks across the whole project…"

# ─── clang-format ─────────────────────────────────────────────────────────
echo "→ clang-format (dry-run)…"
find . -type f \
    ! -path "./.pio/*" ! -path "./scripts/*" \
    \( -iname '*.c' -o -iname '*.cpp' -o -iname '*.hpp' -o -iname '*.h' \) \
    -print0 \
  | xargs -0 clang-format --dry-run --Werror
echo "✔ clang-format passed"

# ─── clang-tidy ─────────────────────────────────────────────────────────
# echo "→ clang-tidy (project-only)…"
# find . -type f \
#     ! -path "./.pio/*" ! -path "./scripts/*" \
#     \( -iname '*.cpp' -o -iname '*.hpp' \) \
#     -print0 \
# | while IFS= read -r -d '' file; do
#     echo "  linting $file"
#     clang-tidy \
#       --warnings-as-errors='*' \
#       --header-filter='^(.\/src|.\/include)\/.*' \
#       --system-headers=false \
#       "$file" -- -std=c++17
# done
# echo "✔ clang-tidy passed"

# ─── Header‐suffix check ─────────────────────────────────────────────────
echo "→ Checking header suffix (must be .hpp)…"
BAD_HDRS=$(find . -type f \
    ! -path "./.pio/*" ! -path "./scripts/*" \
    \( -name '*.h' -o -name '*.hh' \))
if [ -n "$BAD_HDRS" ]; then
  echo "❌ Found wrong header suffix (.h/.hh):"
  echo "$BAD_HDRS"
  exit 1
else
  echo "✔ All headers use .hpp"
fi

# ─── Doxygen‐stub check ──────────────────────────────────────────────────
echo "→ Checking for missing Doxygen comments…"
MISSING=$(grep --include='*.hpp' -R -n '^[[:space:]]*[A-Za-z_].*;' include/ \
          | grep -Ev '///|/\*\*|@brief' || true)
if [ -n "$MISSING" ]; then
  echo "❌ Declarations without Doxygen comments:"
  echo "$MISSING"
  exit 1
else
  echo "✔ Doxygen comments present"
fi

echo "🎉 All lint checks passed!"
