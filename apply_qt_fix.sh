#!/bin/bash

# This script applies a patch to fix an issue in 
# Qt versions 6.5.3 and 6.8.3 that require AGL when 
# Xcode26 no longer includes it.
#
# The script requires the QT_HOME environment variable to be set to the directory
# where your Qt versions are installed.
#
# Refereces: 
# https://qt-project.atlassian.net/browse/QTBUG-137687
# https://github.com/qt/qtbase/commit/cdb33c3d5621ce035ad6950c8e2268fe94b73de5
# https://github.com/Homebrew/homebrew-core/commit/9ef09378761c7d975da890566451726fba53ea51

if [[ "$(uname)" != "Darwin" ]]; then
    echo "Error: This script is only intended for macOS (Darwin)."
    exit 0
fi

if [ -z "$QT_HOME" ]; then
    echo "Error: QT_HOME environment variable is not set."
    echo "Please set it to your Qt installation directory (e.g., /Users/moliver/Qt)."
    exit 1
fi

if [ ! -d "$QT_HOME" ]; then
    echo "Error: QT_HOME directory '$QT_HOME' does not exist."
    exit 1
fi

PATCH_FILE="$(dirname "$0")/qt_fix.patch"

if [ ! -f "$PATCH_FILE" ]; then
    echo "Error: Patch file not found at '$PATCH_FILE'."
    exit 1
fi

QT_VERSIONS=("6.5.3" "6.8.3")

for version in "${QT_VERSIONS[@]}"; do
    QT_DIR="$QT_HOME/$version/macos"
    if [ -d "$QT_DIR" ]; then
        echo "Checking Qt version $version in $QT_DIR..."
        
        # Check if patch is already applied (by checking if AGL is still in FindWrapOpenGL.cmake)
        if ! grep -q "AGL" "$QT_DIR/lib/cmake/Qt6/FindWrapOpenGL.cmake"; then
             echo "Patch is already applied for version $version. Skipping."
             continue
        fi

        # Check if patch applies cleanly
        if patch -p1 -d "$QT_DIR" --dry-run --batch < "$PATCH_FILE" >/dev/null 2>&1; then
            echo "Applying patch to $version..."
            patch -p1 -d "$QT_DIR" --batch < "$PATCH_FILE"
            echo "Successfully patched $version."
        else
             echo "Error: Could not apply patch for version $version. It might be incompatible or already modified."
             echo "Please check the files in $QT_DIR manually."
             # Show dry run output for debugging
             patch -p1 -d "$QT_DIR" --dry-run --batch < "$PATCH_FILE"
        fi
    else
        echo "Warning: Qt version $version not found at $QT_DIR. Skipping."
    fi
done

echo "Done."