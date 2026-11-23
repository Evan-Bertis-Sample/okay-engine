
#!/usr/bin/env bash
set -euo pipefail

CHECK=false
TARGET_DIRS=()

for arg in "$@"; do
    case "$arg" in
        --check)
            CHECK=true
            ;;
        *)
            TARGET_DIRS+=("$arg")
            ;;
    esac
done

# Default to okay/ and proto/
if [ ${#TARGET_DIRS[@]} -eq 0 ]; then
    TARGET_DIRS=(okay proto)
fi

if ! command -v clang-format >/dev/null 2>&1; then
    echo "Error: clang-format not found in PATH."
    exit 1
fi

FORMAT_ARGS=(-i)

if [ "$CHECK" = true ]; then
    FORMAT_ARGS=(--dry-run --Werror)
fi

find "${TARGET_DIRS[@]}" \
    -type d \( -name "vendor" -o -name ".okay" \) -prune \
    -o -type f \( -name "*.cpp" -o -name "*.hpp" \) -print0 |
    xargs -0 -r clang-format "${FORMAT_ARGS[@]}"
