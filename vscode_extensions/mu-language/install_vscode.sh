#!/bin/bash

# Convenience script to install Mu Language Extension for VS Code
# This script calls the main _install.sh script with the 'vscode' argument

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Call the main install script with vscode argument
"$SCRIPT_DIR/_install.sh" vscode
