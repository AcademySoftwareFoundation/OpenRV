#!/bin/bash

# Mu Language Extension Installation Script for VS Code/Cursor
# This script installs the Mu language extension from the OpenRV repository
# Usage: ./_install.sh [vscode|cursor]

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Parse command line argument
EDITOR=""
if [[ $# -eq 1 ]]; then
    EDITOR="$1"
else
    echo "Usage: $0 [vscode|cursor]"
    echo ""
    echo "Examples:"
    echo "  $0 vscode    # Install for VS Code"
    echo "  $0 cursor    # Install for Cursor"
    exit 1
fi

# Determine the target directory based on the editor and OS
if [[ "$EDITOR" == "cursor" ]]; then
    if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]] || [[ "$OSTYPE" == "win32" ]]; then
        TARGET_DIR="$USERPROFILE/.cursor/extensions/mu-language"
    else
        TARGET_DIR="$HOME/.cursor/extensions/mu-language"
    fi
    SETTINGS_DIR="$HOME/.cursor"
    if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]] || [[ "$OSTYPE" == "win32" ]]; then
        SETTINGS_DIR="$USERPROFILE/.cursor"
    fi
    echo "Installing Mu Language Extension for Cursor..."
elif [[ "$EDITOR" == "vscode" ]]; then
    if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]] || [[ "$OSTYPE" == "win32" ]]; then
        TARGET_DIR="$USERPROFILE/.vscode/extensions/mu-language"
        SETTINGS_DIR="$USERPROFILE/.vscode"
    else
        TARGET_DIR="$HOME/.vscode/extensions/mu-language"
        SETTINGS_DIR="$HOME/.vscode"
    fi
    echo "Installing Mu Language Extension for VS Code..."
else
    echo "Error: Invalid editor '$EDITOR'. Must be 'vscode' or 'cursor'"
    exit 1
fi

# Create target directory if it doesn't exist
mkdir -p "$TARGET_DIR"

# Copy extension files
echo "Copying extension files to $TARGET_DIR..."
cp -r "$SCRIPT_DIR"/* "$TARGET_DIR/"

# Create settings to force .mu file association
echo "Adding .mu file association to $EDITOR settings..."
mkdir -p "$SETTINGS_DIR"
echo '{
    "files.associations": {
        "*.mu": "mu"
    }
}' > "$SETTINGS_DIR/settings.json"

echo "Mu Language Extension installed successfully!"
echo ""
echo "To activate the extension:"
echo "1. Restart VS Code/Cursor"
echo "2. Or reload the window (Ctrl+Shift+P → 'Developer: Reload Window')"
echo ""
echo "If .mu files are still showing as 'Plain Text':"
echo "1. Open a .mu file"
echo "2. Click on 'Plain Text' in the bottom-right corner"
echo "3. Select 'Mu' from the language list"
echo "4. Or run: Ctrl+Shift+P → 'Change Language Mode' → Select 'Mu'"
echo ""
echo "The extension will automatically recognize .mu files and provide syntax highlighting."
echo ""
echo "This extension is part of the OpenRV project: https://github.com/AcademySoftwareFoundation/OpenRV"
