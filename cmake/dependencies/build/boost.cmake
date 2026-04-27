#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# Build Boost from source via ExternalProject_Add. Included by cmake/dependencies/boost.cmake when no installed package is found.
#
# Expects these variables from the caller (set by RV_CREATE_STANDARD_DEPS_VARIABLES): _target, _version, _install_dir, _lib_dir, _include_dir, _source_dir And
# these dep-specific variables from the caller: _major_minor_version, _boost_libs, _boost_byproducts, _boost_${lib}_lib, _boost_${lib}_lib_name,
# _boost_${lib}_implib (Windows)

STRING(REPLACE "." "_" _version_with_underscore ${_version})
SET(_download_url
    "https://archives.boost.io/release/${_version}/source/boost_${_version_with_underscore}.tar.gz"
)
SET(_download_hash
    ${RV_DEPS_BOOST_DOWNLOAD_HASH}
)

LIST(APPEND _boost_b2_options "-s")
LIST(APPEND _boost_b2_options "NO_LZMA=1")

IF(RV_VERBOSE_INVOCATION)
  LIST(APPEND _boost_b2_options "-d+2")
ELSE()
  LIST(APPEND _boost_b2_options "-d+0")
ENDIF()

IF(RV_TARGET_DARWIN)
  SET(_toolset
      "clang"
  )
ELSEIF(RV_TARGET_LINUX)
  SET(_toolset
      "gcc"
  )
ELSEIF(RV_TARGET_WINDOWS)
  SET(_toolset
      "msvc-14.3"
  )
ELSE()
  MESSAGE(FATAL_ERROR "Unsupported (yet) target for Boost")
ENDIF()

IF(RV_TARGET_WINDOWS)
  SET(_bootstrap_command
      ./bootstrap.bat
  )
ELSE()
  SET(_bootstrap_command
      ./bootstrap.sh
  )
ENDIF()

IF(RV_TARGET_WINDOWS)
  SET(_boost_python_bin
      ${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install/python.exe
  )
ELSE()
  SET(_boost_python_bin
      ${RV_DEPS_BASE_DIR}/RV_DEPS_PYTHON3/install/bin/python
  )
ENDIF()

STRING(TOLOWER ${CMAKE_BUILD_TYPE} _boost_variant)

LIST(
  TRANSFORM _boost_libs
  PREPEND "--with-"
          OUTPUT_VARIABLE _boost_with_list
)

SET(__boost_arch__
    x86
)
IF(APPLE)
  IF(RV_TARGET_APPLE_ARM64)
    SET(__boost_arch__
        arm
    )
  ENDIF()
ENDIF()

EXTERNALPROJECT_ADD(
  ${_target}
  DEPENDS Python::Python
  DOWNLOAD_NAME ${_target}_${_version}.tar.gz
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${RV_DEPS_BASE_DIR}/${_target}/src
  INSTALL_DIR ${_install_dir}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  CONFIGURE_COMMAND ${_bootstrap_command} --with-toolset=${_toolset} --with-python=${_boost_python_bin}
  BUILD_COMMAND
    # Ref.: https://www.boost.org/doc/libs/1_70_0/tools/build/doc/html/index.html#bbv2.builtin.features.cflags Ref.:
    # https://www.boost.org/doc/libs/1_76_0/tools/build/doc/html/index.html#bbv2.builtin.features.cflags
    ./b2 -a -q toolset=${_toolset} cxxstd=${RV_CPP_STANDARD} variant=${_boost_variant} link=shared threading=multi architecture=${__boost_arch__}
    address-model=64 ${_boost_with_list} ${_boost_b2_options} -j${_cpu_count} install --prefix=${_install_dir}
  INSTALL_COMMAND echo "Boost was both built and installed in the build stage"
  BUILD_IN_SOURCE TRUE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_boost_byproducts}
  USES_TERMINAL_BUILD TRUE
)

IF(RV_TARGET_WINDOWS)
  SET(_include_dir
      ${_install_dir}/include/boost-${_major_minor_version}
  )
ELSE()
  SET(_include_dir
      ${_install_dir}/include
  )
ENDIF()

FOREACH(
  _boost_lib
  ${_boost_libs}
)
  RV_ADD_IMPORTED_LIBRARY(
    NAME
    Boost::${_boost_lib}
    TYPE
    SHARED
    LOCATION
    ${_boost_${_boost_lib}_lib}
    SONAME
    ${_boost_${_boost_lib}_lib_name}
    IMPLIB
    ${_boost_${_boost_lib}_implib}
    INCLUDE_DIRS
    ${_include_dir}
    DEPENDS
    ${_target}
    ADD_TO_DEPS_LIST
  )
ENDFOREACH()

ADD_LIBRARY(Boost::headers INTERFACE IMPORTED GLOBAL)
SET_TARGET_PROPERTIES(
  Boost::headers
  PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${_include_dir}"
)
