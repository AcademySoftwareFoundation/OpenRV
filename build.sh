#!/usr/bin/env bash
#
# UTV Build Script
# Matches CI/CD execution for local development.
# Copyright (C) 2026 Makai Systems. All Rights Reserved.
#

set -e

# Default settings
BUILD_TYPE="Release"
CLEAN_BUILD=0
INSTALL=0
LOG_FILE=""
BMD_SDK=""
PRORES_SDK=""
CUSTOM_VERSION=""

# Parse arguments
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --debug) BUILD_TYPE="Debug"; shift ;;
        --release) BUILD_TYPE="Release"; shift ;;
        --clean) CLEAN_BUILD=1; shift ;;
        --install) INSTALL=1; shift ;;
        --bmd-sdk) BMD_SDK="$2"; shift 2 ;;
        --prores-sdk) PRORES_SDK="$2"; shift 2 ;;
        --version) CUSTOM_VERSION="$2"; shift 2 ;;
        --log)
            if [[ -n "$2" && "$2" != -* ]]; then
                LOG_FILE="$2"
                shift 2
            else
                mkdir -p logs
                LOG_FILE="logs/build_$(date +%Y%m%d_%H%M%S).log"
                shift 1
            fi
            ;;
        -h|--help)
            echo "Usage: ./build.sh [OPTIONS]"
            echo "Options:"
            echo "  --debug    Build in Debug mode"
            echo "  --release  Build in Release mode (default)"
            echo "  --clean    Remove build directory and virtual environment before building"
            echo "  --install  Install the build to the _install directory"
            echo "  --log [f]  Log output to a file (default: logs/build_TIMESTAMP.log)"
            echo "  --bmd-sdk  Path to the Blackmagic Decklink SDK zip file"
            echo "  --prores-sdk Path to the Apple ProRes SDK zip file"
            echo "  --version  Custom semantic version (e.g. 2026.1)"
            exit 0
            ;;
        *) echo "Unknown parameter: $1"; exit 1 ;;
    esac
done

if [ -n "$LOG_FILE" ]; then
    mkdir -p "$(dirname "$LOG_FILE")"
    echo "Logging build output to: $LOG_FILE"
    exec > >(tee -a "$LOG_FILE") 2>&1
fi

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_ROOT}/_build"
INST_DIR="${PROJECT_ROOT}/_install"
VENV_DIR="${PROJECT_ROOT}/.venv"

echo "=== UTV Build Script ==="
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

if [ -n "$BMD_SDK" ]; then
    CMAKE_ARGS+=("-DRV_DEPS_BMD_DECKLINK_SDK_ZIP_PATH=${BMD_SDK}")
fi

if [ -n "$PRORES_SDK" ]; then
    CMAKE_ARGS+=("-DRV_DEPS_APPLE_PRORES_SDK_ZIP_PATH=${PRORES_SDK}")
fi

if [ -n "$CUSTOM_VERSION" ]; then
    MAJOR=$(echo "$CUSTOM_VERSION" | cut -d'.' -f1)
    MINOR=$(echo "$CUSTOM_VERSION" | cut -d'.' -f2)
    CMAKE_ARGS+=("-DRV_MAJOR_VERSION=${MAJOR}" "-DRV_MINOR_VERSION=${MINOR}" "-DRV_VERSION_YEAR=${MAJOR}")
fi

# Add Windows specifics if running in MSYS/Cygwin
if [[ "$OSTYPE" == "msys"* || "$OSTYPE" == "cygwin"* ]]; then
    CMAKE_ARGS+=("-T" "v143,version=14.40" "-A" "x64")
fi

cmake "${CMAKE_ARGS[@]}"

# 5. Build
echo "--- Building UTV ---"
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
    echo "--- Installing UTV ---"
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
    echo "Executable is at: ${BUILD_DIR}/stage/app/UTV.app/Contents/MacOS/UTV"
else
    echo "Executable is at: ${BUILD_DIR}/stage/app/bin/utv"
fi
