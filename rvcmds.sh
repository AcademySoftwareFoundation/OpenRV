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
    QT_HOME_6=$(find ~/Qt*/6.5* -maxdepth 4 -type d -path '*/gcc_64' | sort -V | tail -n 1)
    QT_HOME_5=$(find ~/Qt*/5.15* -maxdepth 4 -type d -path '*/gcc_64' | sort -V | tail -n 1)
  elif [[ "$OSTYPE" == "darwin"* ]]; then
    QT_HOME_6=$(find ~/Qt*/6.5* -maxdepth 4 -type d -path '*/macos' | sort -V | tail -n 1)
    QT_HOME_5=$(find ~/Qt*/5.15* -maxdepth 4 -type d -path '*/macos' | sort -V | tail -n 1)

    # If no macos installation found, try clang_64
    if [ -z "$QT_HOME_6" ]; then
      QT_HOME_6=$(find ~/Qt*/6.5* -maxdepth 4 -type d -path '*/clang_64' | sort -V | tail -n 1)
    fi

    if [ -z "$QT_HOME_5" ]; then
      QT_HOME_5=$(find ~/Qt*/5.15* -maxdepth 4 -type d -path '*/clang_64' | sort -V | tail -n 1)
    fi

  elif [[ "$OSTYPE" == "msys"* || "$OSTYPE" == "cygwin"* ]]; then
    QT_HOME_6=$(find c:/Qt*/6.5* -maxdepth 4 -type d -path '*/msvc2019_64' | sort -V | tail -n 1)
    QT_HOME_5=$(find c:/Qt*/5.15* -maxdepth 4 -type d -path '*/msvc2019_64' | sort -V | tail -n 1)
  fi

  # Could not find Qt installation
  if [ -z "$QT_HOME_6" ] && [ -z "$QT_HOME_5" ]; then
    echo "Could not find Qt installation. Please set QT_HOME to the correct path in your environment variables."
  else 
    if [ -n "$QT_HOME_6" ]; then
      QT_HOME="$QT_HOME_6"
      QT_VERSION="6"
      RV_DEPS_QT_LOCATION="RV_DEPS_QT6_LOCATION"
      RV_VFX_PLATFORM="CY2024"
    else
      QT_HOME="$QT_HOME_5"
      QT_VERSION="5"
      RV_DEPS_QT_LOCATION="RV_DEPS_QT5_LOCATION"
      RV_VFX_PLATFORM="CY2023"
    fi
    echo "Found Qt $QT_VERSION installation at $QT_HOME"
    echo "Note: If you have multiple version of Qt installed, the first one found will be used. The search prioritize Qt 6."
  fi
 
# Qt installation path already set
else 
  if [[ $QT_HOME == *"6.5"* ]]; then
    echo "Using Qt 6 installation already set at $QT_HOME"
    RV_DEPS_QT_LOCATION="RV_DEPS_QT6_LOCATION"
    RV_VFX_PLATFORM="CY2024"
  elif [[ $QT_HOME == *"5.15"* ]]; then
    echo "Using Qt 5 installation already set at $QT_HOME"
      RV_DEPS_QT_LOCATION="RV_DEPS_QT5_LOCATION"
      RV_VFX_PLATFORM="CY2023"
  else
    echo "Invalid Qt installation path. Please set QT_HOME to the correct path in your environment variables."
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

# Update all path variables based on current RV_PATH_SUFFIX
__rv_update_paths() {
  RV_BUILD_DIR="${RV_HOME}/_build${RV_PATH_SUFFIX}"
  RV_INST_DIR="${RV_HOME}/_install${RV_PATH_SUFFIX}"
  RV_APP_DIR="${RV_BUILD_DIR}/stage/app"
  if [[ "$OSTYPE" == "darwin"* ]]; then
    RV_APP_DIR="${RV_APP_DIR}/RV.app/Contents/MacOS"
  fi
}

# Deactivate virtual environment if active when sourcing
if [ -n "$VIRTUAL_ENV" ]; then
  deactivate
fi

# Capture clean prompt before any RV modifications (only if not already set)
if [ -z "$RV_CLEAN_PROMPT" ]; then
  RV_CLEAN_PROMPT="$PS1"
fi

# Single set of aliases using current build variables
# Note: Single quotes preserve variables for expansion at execution time
alias rvenv='__rv_env_shell'
alias rvsetup='rvenv && SETUPTOOLS_USE_DISTUTILS=${SETUPTOOLS_USE_DISTUTILS} python3 -m pip install --upgrade -r ${RV_HOME}/requirements.txt'
alias rvcfg='rvenv && cmake -B ${RV_BUILD_DIR} -G "${CMAKE_GENERATOR}" ${RV_TOOLCHAIN} ${CMAKE_WIN_ARCH} -DCMAKE_BUILD_TYPE=${RV_BUILD_TYPE} -D${RV_DEPS_QT_LOCATION}=${QT_HOME} -DRV_VFX_PLATFORM=${RV_VFX_PLATFORM} -DRV_DEPS_WIN_PERL_ROOT=${WIN_PERL}'
alias rvbuildt='rvenv && cmake --build ${RV_BUILD_DIR} --config ${RV_BUILD_TYPE} -v --parallel=${RV_BUILD_PARALLELISM} --target '
alias rvbuild='rvenv && rvbuildt main_executable'
alias rvtest='rvenv && ctest --test-dir ${RV_BUILD_DIR} --extra-verbose'
alias rvinst='rvenv && cmake --install ${RV_BUILD_DIR} --prefix ${RV_INST_DIR} --config ${RV_BUILD_TYPE}'
alias rvclean='__rv_clean_build'
alias rvappdir='cd ${RV_APP_DIR}'
alias rvhomedir='cd ${RV_HOME}'
alias rvhome='cd ${RV_HOME}'
alias rvmk='rvbuild'
alias rvbootstrap='rvsetup && rvcfg && rvbuild'

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
      export PS1="${build_prefix} (run rvcfg) $RV_CLEAN_PROMPT"
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

echo "To override any of them do unset [name]; export [name]=value; source $SCRIPT"
echo
echo
echo "Use 'rvrelease' (default) or 'rvdebug' to switch between build configurations."
echo "Call 'rvbootstrap' if its your first time building or after calling rvclean."
echo
echo "After rvbootstrap, you can do incremental builds using 'rvbuild' or 'rvmk'."
echo

# Initialize with appropriate build mode (after sourcing, if we were in debug, stay in debug)
if [ "$INIT_BUILD_TYPE" = "Debug" ]; then
  rvdebug
else
  rvrelease
fi
