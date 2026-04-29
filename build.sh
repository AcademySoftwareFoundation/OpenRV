#!/usr/bin/env bash
#
# Visto Build Script
# Matches CI/CD execution for local development.
# Copyright (C) 2026 Makai Systems. All Rights Reserved.
#

set -e

# Default settings
BUILD_TYPE="Release"
CLEAN_BUILD=0
INSTALL=0

# Parse arguments
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --debug) BUILD_TYPE="Debug"; shift ;;
        --release) BUILD_TYPE="Release"; shift ;;
        --clean) CLEAN_BUILD=1; shift ;;
        --install) INSTALL=1; shift ;;
        -h|--help)
            echo "Usage: ./build.sh [OPTIONS]"
            echo "Options:"
            echo "  --debug    Build in Debug mode"
            echo "  --release  Build in Release mode (default)"
            echo "  --clean    Remove build directory and virtual environment before building"
            echo "  --install  Install the build to the _install directory"
            exit 0
            ;;
        *) echo "Unknown parameter: $1"; exit 1 ;;
    esac
done

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_ROOT}/_build"
INST_DIR="${PROJECT_ROOT}/_install"
VENV_DIR="${PROJECT_ROOT}/.venv"

echo "=== Visto Build Script ==="
echo "Build Type: ${BUILD_TYPE}"

if [ "${CLEAN_BUILD}" -eq 1 ]; then
    echo "Cleaning build and environment directories..."
    rm -rf "${BUILD_DIR}"
    rm -rf "${VENV_DIR}"
fi

# 1. Setup Python Environment
echo "--- Setting up Python Environment ---"
if command -v uv >/dev/null 2>&1; then
    if [ ! -d "${VENV_DIR}" ]; then
        uv venv "${VENV_DIR}" --python 3.14
    fi
    source "${VENV_DIR}/bin/activate"
    uv pip install -r "${PROJECT_ROOT}/requirements.txt"
else
    echo "uv not found, falling back to standard venv/pip."
    if [ ! -d "${VENV_DIR}" ]; then
        python3 -m venv "${VENV_DIR}"
    fi
    source "${VENV_DIR}/bin/activate"
    python3 -m pip install -r "${PROJECT_ROOT}/requirements.txt"
fi

# 2. Locate Qt6
echo "--- Locating Qt6 ---"
if [ -z "$QT_HOME" ]; then
    if [[ "$OSTYPE" == "linux"* ]]; then
        QT_HOME=$(find /usr/lib64/qt6 /usr/lib/qt6 ~/Qt*/6.* -maxdepth 4 -type d -path '*/gcc_64' 2>/dev/null | sort -V | tail -n 1)
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        if [ -d "/opt/homebrew/opt/qtbase/lib/cmake/Qt6" ]; then
            QT_HOME="/opt/homebrew/opt/qtbase"
        elif [ -d "/opt/homebrew/opt/qt/lib/cmake/Qt6" ]; then
            QT_HOME="/opt/homebrew/opt/qt"
        else
            QT_HOME=$(find /opt/homebrew/Cellar/qtbase/*/lib/cmake/Qt6 /opt/homebrew/Cellar/qt/*/lib/cmake/Qt6 -maxdepth 0 2>/dev/null | sort -V | tail -n 1 | sed 's|/lib/cmake/Qt6||')
        fi
        if [ -z "$QT_HOME" ]; then
            QT_HOME=$(find ~/Qt*/6.* -maxdepth 4 -type d -path '*/macos' 2>/dev/null | sort -V | tail -n 1)
        fi
    fi
fi

if [ -z "$QT_HOME" ]; then
    echo "ERROR: Could not find required Qt 6 installation. Please set QT_HOME."
    exit 1
fi
echo "Using QT_HOME=${QT_HOME}"

# 3. macOS Specific Fixes
if [[ "$OSTYPE" == "darwin"* ]]; then
    if command -v xcodebuild >/dev/null 2>&1; then
        XCODE_MAJOR_VERSION=$(xcodebuild -version | head -n 1 | awk '{print $2}' | cut -d. -f1)
        if [[ -n "$XCODE_MAJOR_VERSION" && "$XCODE_MAJOR_VERSION" =~ ^[0-9]+$ && "$XCODE_MAJOR_VERSION" -ge 26 ]]; then
            QT_BASE_DIR="$(dirname "$(dirname "$QT_HOME")")"
            if [ -d "$QT_BASE_DIR" ]; then
                echo "Xcode 26+ detected, ensuring Qt AGL fix is applied..."
                QT_HOME="$QT_BASE_DIR" bash "${PROJECT_ROOT}/apply_qt_fix.sh"
            fi
        fi
    fi
fi

# 4. Configure CMake
echo "--- Configuring CMake ---"
CMAKE_GENERATOR=${CMAKE_GENERATOR:-Ninja}
CMAKE_ARGS=(
    "-B" "${BUILD_DIR}"
    "-G" "${CMAKE_GENERATOR}"
    "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"
    "-DRV_DEPS_QT_LOCATION=${QT_HOME}"
    "-DRV_VFX_PLATFORM=CY2026"
    "-DRV_USE_SYSTEM_DEPS=ON"
)

# Add Windows specifics if running in MSYS/Cygwin
if [[ "$OSTYPE" == "msys"* || "$OSTYPE" == "cygwin"* ]]; then
    CMAKE_ARGS+=("-T" "v143,version=14.40" "-A" "x64")
fi

cmake "${CMAKE_ARGS[@]}"

# 5. Build
echo "--- Building Visto ---"
PARALLELISM=${RV_BUILD_PARALLELISM:-$(python3 -c 'import os; print(os.cpu_count())')}

echo "Building dependencies target..."
cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" --parallel "${PARALLELISM}" --target dependencies

echo "Building main_executable target..."
cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" --parallel "${PARALLELISM}" --target main_executable

# 6. Sanitize Homebrew Links
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "--- Sanitizing Homebrew Links ---"
    python3 "${PROJECT_ROOT}/src/build/sanitize_homebrew_links.py" "${BUILD_DIR}/stage"
fi

if [ "${INSTALL}" -eq 1 ]; then
    echo "--- Installing Visto ---"
    cmake --install "${BUILD_DIR}" --prefix "${INST_DIR}" --config "${BUILD_TYPE}"
    
    if [[ "$OSTYPE" == "darwin"* ]]; then
        echo "--- Sanitizing Installed Homebrew Links ---"
        python3 "${PROJECT_ROOT}/src/build/sanitize_homebrew_links.py" "${INST_DIR}"
    fi
fi

echo "=== Build Complete ==="
if [ "${INSTALL}" -eq 1 ]; then
    echo "Installed to: ${INST_DIR}"
fi
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "Executable is at: ${BUILD_DIR}/stage/app/Visto.app/Contents/MacOS/visto"
else
    echo "Executable is at: ${BUILD_DIR}/stage/app/bin/visto"
fi
