# Ensure file is sourced and not executed

SOURCED=0
if [ -n "$ZSH_EVAL_CONTEXT" ]; then
  [[ $ZSH_EVAL_CONTEXT =~ :file$ ]] && SOURCED=1
  SCRIPT=$0
elif [ -n "$KSH_VERSION" ]; then
  [[ "$(cd $(dirname -- $0) && pwd -P)/$(basename -- $0)" != "$(cd $(dirname -- ${.sh.file}) && pwd -P)/$(basename -- ${.sh.file})" ]] && SOURCED=1
  SCRIPT=$0
elif [ -n "$BASH_VERSION" ]; then
  [[ $0 != "$BASH_SOURCE" ]] && SOURCED=1
  SCRIPT=${BASH_SOURCE[0]}
elif grep -q dash /proc/$$/cmdline; then
  case $0 in *dash*) SOURCED=1 ;; esac
  x=$(lsof -p $$ -Fn0 | tail -1); SCRIPT=${x#n}
fi

SCRIPT_HOME=`readlink -f $(dirname $SCRIPT)`
if [[ $SOURCED == 0 ]]; then
  echo "Please call: source $SCRIPT"
  exit 1
fi

__rv_select_vfx_platform() {
  # If RV_VFX_PLATFORM is already set to a valid value, use it.
  if [[ "$RV_VFX_PLATFORM" =~ ^CY202(3|4|5|6)$ ]]; then
    echo "RV_VFX_PLATFORM already set to $RV_VFX_PLATFORM."
    return
  fi

  # If it's set to something invalid, unset it and prompt.
  if [ -n "$RV_VFX_PLATFORM" ]; then
    echo "Invalid RV_VFX_PLATFORM: '$RV_VFX_PLATFORM'. Please choose a valid option."
    unset RV_VFX_PLATFORM
  fi

  while [ -z "$RV_VFX_PLATFORM" ]; do
    echo "Please select the VFX Platform year to build for:"
    PS3="Enter a number: "
    select opt in CY2023 CY2024 CY2025 CY2026; do
      if [[ -n "$opt" ]]; then
        export RV_VFX_PLATFORM=$opt
        echo "Using VFX Platform: $RV_VFX_PLATFORM"
        break
      else
        echo "Invalid selection. Please try again."
      fi
    done
  done
}

__rv_select_vfx_platform


# Linux
if [[ "$OSTYPE" == "linux"* ]]; then
  CMAKE_GENERATOR="${CMAKE_GENERATOR:-Ninja}"
  RV_TOOLCHAIN=""

# MacOS
elif [[ "$OSTYPE" == "darwin"* ]]; then
  CMAKE_GENERATOR="${CMAKE_GENERATOR:-Ninja}"
  RV_TOOLCHAIN=""
  export PATH="/opt/homebrew/opt/python@3.11/libexec/bin:$PATH" 

# Windows
elif [[ "$OSTYPE" == "msys"* || "$OSTYPE" == "cygwin"* ]]; then
  CMAKE_GENERATOR="${CMAKE_GENERATOR:-Visual Studio 17 2022}"
  WIN_PERL="${WIN_PERL:-c:/Strawberry/perl/bin}"
  CMAKE_WIN_ARCH="${CMAKE_WIN_ARCH:--A x64}"
  SETUPTOOLS_USE_DISTUTILS=stdlib
  RV_TOOLCHAIN="-T v143,version=14.40"

else
  echo "OS does not seem to be linux, darwin or msys/cygwin. Exiting."
  exit 1
fi

# Searching for a Qt installation path if it has not already been set
if [ -z "$QT_HOME" ]; then
  echo "Searching for Qt installation..."

  if [[ "$OSTYPE" == "linux"* ]]; then
    if [[ "$RV_VFX_PLATFORM" == "CY2026" ]]; then
      QT_HOME=$(find ~/Qt*/6.8.* -maxdepth 4 -type d -path '*/gcc_64' | sort -V | tail -n 1)
      QT_VERSION="6.8"
    elif [[ "$RV_VFX_PLATFORM" == "CY2025" || "$RV_VFX_PLATFORM" == "CY2024" ]]; then
      QT_HOME=$(find ~/Qt*/6.5* -maxdepth 4 -type d -path '*/gcc_64' | sort -V | tail -n 1)
      QT_VERSION="6.5"
    elif [[ "$RV_VFX_PLATFORM" == "CY2023" ]]; then
      QT_HOME=$(find ~/Qt*/5.15* -maxdepth 4 -type d -path '*/gcc_64' | sort -V | tail -n 1)
      QT_VERSION="5.15"
    fi
  elif [[ "$OSTYPE" == "darwin"* ]]; then
    if [[ "$RV_VFX_PLATFORM" == "CY2026" ]]; then
      QT_HOME=$(find ~/Qt*/6.8.* -maxdepth 4 -type d -path '*/macos' | sort -V | tail -n 1)
      if [ -z "$QT_HOME" ]; then
        QT_HOME=$(find ~/Qt*/6.8.* -maxdepth 4 -type d -path '*/clang_64' | sort -V | tail -n 1)
      fi
      QT_VERSION="6.8"
    elif [[ "$RV_VFX_PLATFORM" == "CY2025" || "$RV_VFX_PLATFORM" == "CY2024" ]]; then
      QT_HOME=$(find ~/Qt*/6.5* -maxdepth 4 -type d -path '*/macos' | sort -V | tail -n 1)
      if [ -z "$QT_HOME" ]; then
        QT_HOME=$(find ~/Qt*/6.5* -maxdepth 4 -type d -path '*/clang_64' | sort -V | tail -n 1)
      fi
      QT_VERSION="6.5"
    elif [[ "$RV_VFX_PLATFORM" == "CY2023" ]]; then
      QT_HOME=$(find ~/Qt*/5.15* -maxdepth 4 -type d -path '*/macos' | sort -V | tail -n 1)
      if [ -z "$QT_HOME" ]; then
        QT_HOME=$(find ~/Qt*/5.15* -maxdepth 4 -type d -path '*/clang_64' | sort -V | tail -n 1)
      fi
      QT_VERSION="5.15"
    fi
  elif [[ "$OSTYPE" == "msys"* || "$OSTYPE" == "cygwin"* ]]; then
    if [[ "$RV_VFX_PLATFORM" == "CY2026" ]]; then
      QT_HOME=$(find c:/Qt*/6.8* -maxdepth 4 -type d -path '*/msvc2019_64' | sort -V | tail -n 1)
      QT_VERSION="6.8"
    elif [[ "$RV_VFX_PLATFORM" == "CY2025" || "$RV_VFX_PLATFORM" == "CY2024" ]]; then
      QT_HOME=$(find c:/Qt*/6.5* -maxdepth 4 -type d -path '*/msvc2019_64' | sort -V | tail -n 1)
      QT_VERSION="6.5"
    elif [[ "$RV_VFX_PLATFORM" == "CY2023" ]]; then
      QT_HOME=$(find c:/Qt*/5.15* -maxdepth 4 -type d -path '*/msvc2019_64' | sort -V | tail -n 1)
      QT_VERSION="5.15"
    fi
  fi

  if [ -n "$QT_HOME" ]; then
    echo "Found Qt $QT_VERSION installation at $QT_HOME"
  else
    echo "Error: $RV_VFX_PLATFORM requires a Qt $QT_VERSION installation, but none was found."
    echo "Could not find required Qt installation. Please set QT_HOME to the correct path in your environment variables."
  fi
 
# Qt installation path already set
else 
  if [[ $QT_HOME == *"6.8"* ]]; then
    echo "Using Qt 6.8 installation already set at $QT_HOME"
    if [[ "$RV_VFX_PLATFORM" != "CY2026" ]]; then
        echo "Warning: QT_HOME is set to a Qt 6.8 path, but RV_VFX_PLATFORM is $RV_VFX_PLATFORM."
    fi
  elif [[ $QT_HOME == *"6.5"* ]]; then
    echo "Using Qt 6.5 installation already set at $QT_HOME"
    if [[ "$RV_VFX_PLATFORM" != "CY2024" && "$RV_VFX_PLATFORM" != "CY2025" ]]; then
        echo "Warning: QT_HOME is set to a Qt 6.5 path, but RV_VFX_PLATFORM is $RV_VFX_PLATFORM."
    fi
  elif [[ $QT_HOME == *"5.15"* ]]; then
    echo "Using Qt 5.15 installation already set at $QT_HOME"
    if [[ "$RV_VFX_PLATFORM" != "CY2023" ]]; then
        echo "Warning: QT_HOME is set to a Qt5 path, but RV_VFX_PLATFORM is $RV_VFX_PLATFORM."
    fi
  else
    echo "Warning: Could not determine Qt version from path: $QT_HOME. Assuming it is compatible."
  fi
fi

# Must be executed in a function as it changes the shell environment
__rv_env_shell() {
  local activate_path=".venv/bin/activate"

  # Using msys2/cygwin as a way to detect if the script is running on Windows.
  if [[ "$OSTYPE" == "msys"* || "$OSTYPE" == "cygwin"* ]]; then
    activate_path=".venv/Scripts/activate"
  fi

  # Deactivate first if already in a virtual environment to ensure fresh activation
  if [ -n "$VIRTUAL_ENV" ]; then
    deactivate 2>/dev/null || true
  fi

  if [ -d ".venv" ]; then
    source "$activate_path"
  else
    python3 -m venv .venv
    source "$activate_path"
  fi
  
  # Customize prompt based on build type
  if [ -n "$RV_BUILD_TYPE" ]; then
    if [ "$RV_BUILD_TYPE" = "Release" ]; then
      export PS1="rel (.venv) $RV_CLEAN_PROMPT"
    elif [ "$RV_BUILD_TYPE" = "Debug" ]; then
      export PS1="dbg (.venv) $RV_CLEAN_PROMPT"
    fi
  fi
  
  # Install pre-commit hooks if not already installed.
  __rv_install_precommit_hooks
}

# Install pre-commit hooks if in a git repository.
__rv_install_precommit_hooks() {
  # Check if we're in a git repository.
  if [ ! -d "${RV_HOME}/.git" ]; then
    return 0
  fi
  
  # Check if hooks are already installed to avoid redundant messages.
  if [ -f "${RV_HOME}/.git/hooks/pre-commit" ] && grep -q "pre-commit" "${RV_HOME}/.git/hooks/pre-commit" 2>/dev/null; then
    return 0
  fi
  
  echo "Installing pre-commit hooks..."
  pre-commit install 2>/dev/null
  if [ $? -eq 0 ]; then
    echo "  Pre-commit hooks installed successfully."
  else
    echo "  Failed to install pre-commit hooks."
  fi
}

# Update all path variables based on current RV_PATH_SUFFIX
__rv_update_paths() {
  RV_BUILD_DIR="${RV_HOME}/_build${RV_PATH_SUFFIX}"
  RV_INST_DIR="${RV_HOME}/_install${RV_PATH_SUFFIX}"
  RV_APP_DIR="${RV_BUILD_DIR}/stage/app/bin"
  if [[ "$OSTYPE" == "darwin"* ]]; then
    RV_APP_DIR="${RV_BUILD_DIR}/stage/app/RV.app/Contents/MacOS"
  fi
}


# Set initial prompt when not in virtual environment
__rv_set_first_prompt() {
  if [ -z "$VIRTUAL_ENV" ]; then
    local build_prefix=""
    if [ "$RV_BUILD_TYPE" = "Debug" ]; then
      build_prefix="dbg"
    elif [ "$RV_BUILD_TYPE" = "Release" ]; then
      build_prefix="rel"
    fi
    
    if [ -d "${RV_BUILD_DIR}" ]; then
      export PS1="${build_prefix} (run rvcfg/rvmk) $RV_CLEAN_PROMPT"
    else
      export PS1="${build_prefix} (run rvbootstrap) $RV_CLEAN_PROMPT"
    fi
  fi
}

# Clean build directory and virtual environment
__rv_clean_build() {
  # Deactivate virtual environment if active
  if [ -n "$VIRTUAL_ENV" ]; then
    deactivate
  fi
  
  # Remove build directory and virtual environment
  rm -rf "${RV_BUILD_DIR}" && rm -rf .venv
  
  # Reset prompt to show current build state
  __rv_set_first_prompt
}

# Internal function to handle configuration switching logic
__rv_switch_config() {
  if [ -z "$VIRTUAL_ENV" ]; then
    # First time (venv wasn't activated): set first prompt
    __rv_set_first_prompt
  else
    # venv was activated and we're switching config
    if [ -d "${RV_BUILD_DIR}" ]; then
      # Build dir exists: call rvcfg
      rvcfg
    else
      # Build dir doesn't exist: exit venv and set first prompt
      deactivate
      __rv_set_first_prompt
    fi
  fi
}

# Switch to debug build mode
rvdebug() {
  RV_BUILD_TYPE="Debug"
  RV_PATH_SUFFIX="_debug"
  
  # Update all path variables
  __rv_update_paths
  
  __rv_switch_config
}

# Switch to release build mode
rvrelease() {
  RV_BUILD_TYPE="Release"
  RV_PATH_SUFFIX=""
  
  # Update all path variables
  __rv_update_paths
  
  __rv_switch_config
}

# VARIABLES
RV_HOME="${RV_HOME:-$SCRIPT_HOME}"
RV_BUILD_PARALLELISM="${RV_BUILD_PARALLELISM:-$(python3 -c 'import os; print(os.cpu_count())')}"

# Capture current build type before any variable declarations
INIT_BUILD_TYPE="$RV_BUILD_TYPE"

# Build configuration variables (initialized by rvrelease at end of script)
RV_BUILD_TYPE=""
RV_BUILD_DIR=""
RV_INST_DIR=""
RV_APP_DIR=""
RV_PATH_SUFFIX=""

# Deactivate virtual environment if active when sourcing
if [ -n "$VIRTUAL_ENV" ]; then
  deactivate
fi

# Capture clean prompt before any RV modifications (only if not already set)
if [ -z "$RV_CLEAN_PROMPT" ]; then
  RV_CLEAN_PROMPT="$PS1"
fi

# Function to build with better error reporting
__rv_build_with_errors() {
  local target="$1"
  local build_log="${RV_BUILD_DIR}/build_errors.log"
  local error_summary="${RV_BUILD_DIR}/error_summary.txt"
  
  echo "Building target: ${target}"
  echo "Build errors will be logged to: ${build_log}"
  echo ""
  
  # Set ninja to keep going after failures to see all errors
  # The -k flag tells ninja to keep building as far as possible
  if [[ "${CMAKE_GENERATOR}" == "Ninja" ]]; then
    export NINJA_STATUS="[%f/%t %p] "
    local ninja_flags="-k 0"  # Keep going, don't stop on first error
  else
    local ninja_flags=""
  fi
  
  # Run the build, capturing output
  # Using set -o pipefail to ensure we catch the cmake exit code through the pipe
  set -o pipefail
  cmake --build ${RV_BUILD_DIR} --config ${RV_BUILD_TYPE} -v --parallel=${RV_BUILD_PARALLELISM} --target ${target} -- ${ninja_flags} 2>&1 | tee "${build_log}"
  local exit_code=$?
  set +o pipefail
  
  echo ""
  
  if [[ $exit_code -eq 0 ]]; then
    echo "✓ Build completed successfully!"
    rm -f "${error_summary}"
    return 0
  else
    echo ""
    echo "════════════════════════════════════════════════════════════════"
    echo "✗ BUILD FAILED - Error Summary"
    echo "════════════════════════════════════════════════════════════════"
    echo ""
    
    # Extract and display compilation errors
    if [[ -f "${build_log}" ]]; then
      # Extract error messages (looking for common error patterns across platforms)
      # Patterns cover:
      #   - GCC/Clang:  "error:", "fatal error:", "undefined reference"
      #   - MSVC:       "error C####:", "error LNK####:", ": error :"
      #   - Ninja:      "FAILED:"
      #   - CMake:      "CMake Error"
      #   - Linker:     "unresolved external symbol", "undefined symbol"
      grep -E "(error C[0-9]+:|error LNK[0-9]+:|: error :|error:|fatal error:|undefined reference|undefined symbol|unresolved external symbol|FAILED:|CMake Error)" "${build_log}" | \
        grep -v "warnings being treated as errors" | \
        grep -v "0 error" | \
        head -50 > "${error_summary}" 2>/dev/null || true
      
      if [[ -s "${error_summary}" ]]; then
        echo "🔥 Compilation Errors Found:"
        echo "────────────────────────────────────────────────────────────────"
        cat "${error_summary}"
        echo "────────────────────────────────────────────────────────────────"
        echo ""
        echo "📝 Full build log: ${build_log}"
        echo "📝 Error summary: ${error_summary}"
      else
        echo "Build failed but no specific errors extracted."
        echo "Check the full build log: ${build_log}"
        echo ""
        echo "Last 30 lines of build output:"
        echo "────────────────────────────────────────────────────────────────"
        tail -30 "${build_log}"
        echo "────────────────────────────────────────────────────────────────"
      fi
    fi
    
    echo ""
    echo "💡 Tips:"
    echo "  • Review the error summary above"
    echo "  • Check full log: less ${build_log}"
    echo "  • Search for specific errors: grep -i 'your_error' ${build_log}"
    echo "════════════════════════════════════════════════════════════════"
    echo ""
    
    return $exit_code
  fi
}

# Single set of aliases using current build variables
# Note: Single quotes preserve variables for expansion at execution time
alias rvappdir='cd ${RV_APP_DIR}'
alias rvhomedir='cd ${RV_HOME}'
alias rvenv='rvhomedir && __rv_env_shell'
alias rvsetup='rvenv && SETUPTOOLS_USE_DISTUTILS=${SETUPTOOLS_USE_DISTUTILS} python3 -m pip install --upgrade -r ${RV_HOME}/requirements.txt'
alias rvcfg='rvhomedir && rvenv && cmake -B ${RV_BUILD_DIR} -G "${CMAKE_GENERATOR}" ${RV_TOOLCHAIN} ${CMAKE_WIN_ARCH} -DCMAKE_BUILD_TYPE=${RV_BUILD_TYPE} -DRV_DEPS_QT_LOCATION=${QT_HOME} -DRV_VFX_PLATFORM=${RV_VFX_PLATFORM} -DRV_DEPS_WIN_PERL_ROOT=${WIN_PERL}'
alias rvbuildt='rvenv && __rv_build_with_errors'
alias rvbuild='rvenv && rvbuildt main_executable'
alias rvtest='rvenv && ctest --test-dir ${RV_BUILD_DIR} --extra-verbose'
alias rvinst='rvenv && cmake --install ${RV_BUILD_DIR} --prefix ${RV_INST_DIR} --config ${RV_BUILD_TYPE}'
alias rvclean='rvhomedir && __rv_clean_build'
alias rvmk='rvcfg && rvbuild'
alias rvbootstrap='rvsetup && rvmk'
alias rvrun='rvappdir && ./rv'
alias rverrors='less ${RV_BUILD_DIR}/build_errors.log'
alias rverrsummary='cat ${RV_BUILD_DIR}/error_summary.txt 2>/dev/null || echo "No error summary found. Build may have succeeded or not run yet."'

__rv_update_paths

echo "Please ensure you have installed any required dependencies from doc/build_system/config_[os]"
echo
echo "CMake parameters:"

echo "RV_BUILD_PARALLELISM is $RV_BUILD_PARALLELISM"
echo "RV_HOME is $RV_HOME"
echo "RV_BUILD_DIR is $RV_BUILD_DIR"
echo "RV_INST_DIR is $RV_INST_DIR"
echo "CMAKE_GENERATOR is $CMAKE_GENERATOR"
echo "QT_HOME is $QT_HOME"
if [[ "$OSTYPE" == "msys"* || "$OSTYPE" == "cygwin"* ]]; then echo "WIN_PERL is $WIN_PERL"; fi

echo 
echo "To override any of them do unset [name]; export [name]=value; source $SCRIPT"
echo
echo "Use 'rvrelease' (default) or 'rvdebug' to switch between build configurations."
echo "Call 'rvbootstrap' if its your first time building or after calling rvclean."
echo "After 'rvbootstrap', use 'rvbuild' or 'rvmk' for incremental builds."
echo
echo "If build fails, use 'rverrsummary' to see error summary or 'rverrors' to view full log."
echo

# Initialize with appropriate build mode
if [ "$INIT_BUILD_TYPE" = "Debug" ]; then
  rvdebug
else
  # Check for existing build directories to determine best default
  if [ -d "${RV_HOME}/_build_debug" ] && [ ! -d "${RV_HOME}/_build" ]; then
    rvdebug
  else
    rvrelease
  fi
fi
