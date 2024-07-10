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

# MacOS
elif [[ "$OSTYPE" == "darwin"* ]]; then
  CMAKE_GENERATOR="${CMAKE_GENERATOR:-Ninja}"

# Windows
elif [[ "$OSTYPE" == "msys"* ]]; then
  CMAKE_GENERATOR="${CMAKE_GENERATOR:-Visual Studio 17 2022}"
  WIN_PERL="${WIN_PERL:-c:/Strawberry/perl/bin}"
  CMAKE_WIN_ARCH="${CMAKE_WIN_ARCH:--A x64}"
  SETUPTOOLS_USE_DISTUTILS=stdlib

else
  echo "OS does not seem to be linux, darwin or msys. Exiting."
  exit 1
fi

# Searching for a Qt installation path if it has not already been set
if [ -z "$QT_HOME" ]; then
  echo "Searching for Qt installation..."

  if [[ "$OSTYPE" == "linux"* ]]; then
    QT_HOME=$(find ~/Qt/5.15* -type d -maxdepth 4 -path '*/gcc_64' | head -n 1)
  elif [[ "$OSTYPE" == "darwin"* ]]; then
    QT_HOME=$(find ~/Qt/5.15* -type d -maxdepth 4 -path '*/macos' | head -n 1)

    # If no macos installation found, try clang_64
    if [ -z "$QT_HOME" ]; then
      QT_HOME=$(find ~/Qt/5.15* -type d -maxdepth 4 -path '*/clang_64' | head -n 1)
    fi
    
  elif [[ "$OSTYPE" == "msys"* ]]; then
    QT_HOME=$(find c:/Qt/5.15* -type d -maxdepth 4 -path '*/msvc2019_64' | head -n 1)
  fi

  # Could not find Qt installation
  if [ -z "$QT_HOME" ]; then
    echo "Could not find Qt installation. Please set QT_HOME to the correct path in your environment variables."
  else 
    echo "Found Qt installation at $QT_HOME"
  fi

# Qt installation path already set
else 
  echo "Using Qt installation already set at $QT_HOME"
fi

# VARIABLES
RV_HOME="${RV_HOME:-$SCRIPT_HOME}"
RV_BUILD="${RV_BUILD:-${RV_HOME}/_build}"
RV_INST="${RV_INST:-${RV_HOME}/_install}"
RV_BUILD_PARALLELISM="${RV_BUILD_PARALLELISM:-$(python3 -c 'import os; print(os.cpu_count())')}"

# ALIASES: Basic commands

alias rvsetup="SETUPTOOLS_USE_DISTUTILS=${SETUPTOOLS_USE_DISTUTILS} python3 -m pip install --user --upgrade -r ${RV_HOME}/requirements.txt"
alias rvcfg="cmake -B ${RV_BUILD} -G \"${CMAKE_GENERATOR}\" ${CMAKE_WIN_ARCH} -DCMAKE_BUILD_TYPE=Release -DRV_DEPS_QT5_LOCATION=${QT_HOME} -DRV_DEPS_WIN_PERL_ROOT=${WIN_PERL}"
alias rvcfgd="cmake -B ${RV_BUILD} -G \"${CMAKE_GENERATOR}\" ${CMAKE_WIN_ARCH} -DCMAKE_BUILD_TYPE=Debug -DRV_DEPS_QT5_LOCATION=${QT_HOME} -DRV_DEPS_WIN_PERL_ROOT=${WIN_PERL}"
alias rvbuildt="cmake --build ${RV_BUILD} --config Release -v --parallel=${RV_BUILD_PARALLELISM} --target "
alias rvbuildtd="cmake --build ${RV_BUILD} --config Debug -v --parallel=${RV_BUILD_PARALLELISM} --target "
alias rvbuild="rvbuildt main_executable"
alias rvbuildd="rvbuildtd main_executable"
alias rvtest="ctest --test-dir ${RV_BUILD} --extra=verbose"
alias rvinst="cmake --install ${RV_BUILD} --prefix ${RV_INST} --config Release"
alias rvinstd="cmake --install ${RV_BUILD} --prefix ${RV_INST} --config Debug"
alias rvclean="rm -rf ${RV_BUILD}"

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
if [[ "$OSTYPE" == "msys"* ]]; then echo "WIN_PERL is $WIN_PERL"; fi

echo "To override any of them do unset [name]; export [name]=value; source $SCRIPT"
echo
echo "If this is your first time building RV try rvbootstrap (release) or rvbootstrapd (debug)"
echo "To build quickly after bootstraping try rvmk (release) or rvmkd (debug)"
