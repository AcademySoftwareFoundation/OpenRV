#!/bin/bash

# Convenience script to install Mu Language Extension for Cursor
# This script calls the main _install.sh script with the 'cursor' argument

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Call the main install script with cursor argument
"$SCRIPT_DIR/_install.sh" cursor
