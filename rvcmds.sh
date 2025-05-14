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
rvenv_shell() {
  local activate_path=".venv/bin/activate"

  # Using msys2/cygwin as a way to detect if the script is running on Windows.
  if [[ "$OSTYPE" == "msys"* || "$OSTYPE" == "cygwin"* ]]; then
    activate_path=".venv/Scripts/activate"
  fi

  if [ -d ".venv" ]; then
    source "$activate_path"
  else
    python3 -m venv .venv
    source "$activate_path"
  fi
}

# VARIABLES
RV_HOME="${RV_HOME:-$SCRIPT_HOME}"
RV_BUILD="${RV_BUILD:-${RV_HOME}/_build}"
RV_BUILD_DEBUG="${RV_BUILD_DEBUG:-${RV_HOME}/_build_debug}"
RV_INST="${RV_INST:-${RV_HOME}/_install}"
RV_INST_DEBUG="${RV_INST_DEBUG:-${RV_HOME}/_install_debug}"
RV_BUILD_PARALLELISM="${RV_BUILD_PARALLELISM:-$(python3 -c 'import os; print(os.cpu_count())')}"

# ALIASES: Basic commands
alias rvenv="rvenv_shell"
alias rvsetup="rvenv && SETUPTOOLS_USE_DISTUTILS=${SETUPTOOLS_USE_DISTUTILS} python3 -m pip install --upgrade -r ${RV_HOME}/requirements.txt"
alias rvcfg="rvenv && cmake -B ${RV_BUILD} -G \"${CMAKE_GENERATOR}\" ${RV_TOOLCHAIN} ${CMAKE_WIN_ARCH} -DCMAKE_BUILD_TYPE=Release -D${RV_DEPS_QT_LOCATION}=${QT_HOME} -DRV_VFX_PLATFORM=${RV_VFX_PLATFORM} -DRV_DEPS_WIN_PERL_ROOT=${WIN_PERL}"
alias rvcfgd="rvenv && cmake -B ${RV_BUILD_DEBUG} -G \"${CMAKE_GENERATOR}\" ${RV_TOOLCHAIN} ${CMAKE_WIN_ARCH} -DCMAKE_BUILD_TYPE=Debug -D${RV_DEPS_QT_LOCATION}=${QT_HOME} -DRV_VFX_PLATFORM=${RV_VFX_PLATFORM} -DRV_DEPS_WIN_PERL_ROOT=${WIN_PERL}"
alias rvbuildt="rvenv && cmake --build ${RV_BUILD} --config Release -v --parallel=${RV_BUILD_PARALLELISM} --target "
alias rvbuildtd="rvenv && cmake --build ${RV_BUILD_DEBUG} --config Debug -v --parallel=${RV_BUILD_PARALLELISM} --target "
alias rvbuild="rvenv && rvbuildt main_executable"
alias rvbuildd="rvenv && rvbuildtd main_executable"
alias rvtest="rvenv && ctest --test-dir ${RV_BUILD} --extra-verbose"
alias rvtestd="rvenv && ctest --test-dir ${RV_BUILD_DEBUG} --extra-verbose"
alias rvinst="rvenv && cmake --install ${RV_BUILD} --prefix ${RV_INST} --config Release"
alias rvinstd="rvenv && cmake --install ${RV_BUILD_DEBUG} --prefix ${RV_INST_DEBUG} --config Debug"
alias rvclean="rm -rf ${RV_BUILD} && rm -rf .venv"
alias rvcleand="rm -rf ${RV_BUILD_DEBUG} && rm -rf .venv"

# ALIASES: Config and Build

alias rvmk="rvcfg && rvbuild"
alias rvmkd="rvcfgd && rvbuildd"

# ALIASES: Setup, Config and Build

alias rvbootstrap="rvsetup && rvmk"
alias rvbootstrapd="rvsetup && rvmkd"

echo "Please ensure you have installed any required dependencies from doc/build_system/config_[os]"
echo
echo "CMake parameters:"

echo "RV_BUILD_PARALLELISM is $RV_BUILD_PARALLELISM"
echo "RV_HOME is $RV_HOME"
echo "RV_BUILD is $RV_BUILD"
echo "RV_INST is $RV_INST"
echo "CMAKE_GENERATOR is $CMAKE_GENERATOR"
echo "QT_HOME is $QT_HOME"
if [[ "$OSTYPE" == "msys"* || "$OSTYPE" == "cygwin"* ]]; then echo "WIN_PERL is $WIN_PERL"; fi

echo "To override any of them do unset [name]; export [name]=value; source $SCRIPT"
echo
echo "If this is your first time building RV try rvbootstrap (release) or rvbootstrapd (debug)"
echo "To build quickly after bootstraping try rvmk (release) or rvmkd (debug)"
