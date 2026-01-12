#!/usr/bin/env bash
# Adds an alias to the user's shell configuration file to run the okay-engine script.

# install the okay.py script
# it is located in the same directory as this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OKAY_SCRIPT="$SCRIPT_DIR/okay.py"

if [[ ! -f "$OKAY_SCRIPT" ]]; then
    echo "Error: okay.py script not found in $SCRIPT_DIR"
    exit 1
fi

# Determine the shell configuration file based on the user's shell
case "$SHELL" in
    */bash)
        CONFIG_FILE="$HOME/.bashrc"
        ;;
    */zsh)
        CONFIG_FILE="$HOME/.zshrc"
        ;;
    */fish)
        CONFIG_FILE="$HOME/.config/fish/config.fish"
        ;;
    *)
        echo "Unsupported shell: $SHELL"
        exit 1
        ;;
esac

# Add the alias to the configuration file
ALIAS_LINE="alias okay='python3 \"$OKAY_SCRIPT\"'"
if ! grep -qF "$ALIAS_LINE" "$CONFIG_FILE"; then
    echo "Adding alias to $CONFIG_FILE"
    echo "$ALIAS_LINE" >> "$CONFIG_FILE"
else
    echo "Alias already exists in $CONFIG_FILE"
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
