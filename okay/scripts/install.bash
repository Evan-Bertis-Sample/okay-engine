#!/usr/bin/env bash
set -euo pipefail


SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OKAY_SCRIPT="$SCRIPT_DIR/okay.py"

if [[ ! -f "$OKAY_SCRIPT" ]]; then
    echo "Error: okay.py script not found in $SCRIPT_DIR"
    exit 1
fi

# Directory where the okay wrapper will live
BIN_DIR="$HOME/.local/bin"
OKAY_WRAPPER="$BIN_DIR/okay"

mkdir -p "$BIN_DIR"

# Create/update the wrapper script
cat > "$OKAY_WRAPPER" <<EOF
#!/usr/bin/env bash
exec python3 "$OKAY_SCRIPT" "\$@"
EOF

chmod +x "$OKAY_WRAPPER"

echo "Installed okay wrapper at: $OKAY_WRAPPER"

# Choose correct shell config file for PATH exports
case "$SHELL" in
    */bash)
        CONFIG_FILE="$HOME/.profile"
        ;;
    */zsh)
        CONFIG_FILE="$HOME/.zprofile"
        ;;
    */fish)
        CONFIG_FILE="$HOME/.config/fish/config.fish"
        ;;
    *)
        echo "Unsupported shell: $SHELL"
        exit 1
        ;;
esac

# Export PATH (shell-specific)
if [[ "$SHELL" == */fish ]]; then
    EXPORT_LINE="set -gx PATH \$HOME/.local/bin \$PATH"
else
    EXPORT_LINE='export PATH="$HOME/.local/bin:$PATH"'
fi

# Add PATH export if missing
if ! grep -qF "$EXPORT_LINE" "$CONFIG_FILE" 2>/dev/null; then
    echo "Adding PATH export to $CONFIG_FILE"
    echo "$EXPORT_LINE" >> "$CONFIG_FILE"
else
    echo "PATH export already exists in $CONFIG_FILE"
fi

# Inform the user to source their configuration file
echo "To use the 'okay' command, please run:"
case "$SHELL" in
    */bash)
        echo "source $CONFIG_FILE"
        ;;
    */zsh)
        echo "source $CONFIG_FILE"
        ;;
    */fish)
        echo "source $CONFIG_FILE"
        ;;
    *)
        echo "Please manually source your shell configuration file."
        ;;
esac
