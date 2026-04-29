#
# Modified for the Visto project. Copyright (C) 2026  Makai Systems. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# Official sources: https://github.com/aous72/OpenJPH
#
# Build instructions: https://github.com/aous72/OpenJPH/blob/master/docs/compiling.md
#

IF(RV_USE_SYSTEM_DEPS)
  FIND_PACKAGE(PkgConfig REQUIRED)
  PKG_CHECK_MODULES(OPENJPH REQUIRED openjph)
  IF(TARGET PkgConfig::OPENJPH)
    SET_PROPERTY(
      TARGET PkgConfig::OPENJPH
      PROPERTY IMPORTED_GLOBAL TRUE
    )
  ENDIF()
  IF(NOT TARGET OpenJph::OpenJph)
    ADD_LIBRARY(OpenJph::OpenJph INTERFACE IMPORTED GLOBAL)
    TARGET_LINK_LIBRARIES(
      OpenJph::OpenJph
      INTERFACE ${OPENJPH_LIBRARIES}
    )
    TARGET_INCLUDE_DIRECTORIES(
      OpenJph::OpenJph
      INTERFACE ${OPENJPH_INCLUDE_DIRS}
    )
  ENDIF()
  RETURN()
ENDIF()

# version 2+ requires changes to IOjp2 project
RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_OPENJPH" "${RV_DEPS_OPENJPH_VERSION}" "make" "")
IF(RV_TARGET_LINUX)
  # Overriding _lib_dir created in 'RV_CREATE_STANDARD_DEPS_VARIABLES' since this CMake-based project isn't using lib64
  SET(_lib_dir
      ${_install_dir}/lib
  )
ENDIF()
RV_SHOW_STANDARD_DEPS_VARIABLES()

SET(_download_url
    "https://github.com/aous72/OpenJPH/archive/refs/tags/${_version}.tar.gz"
)
SET(_download_hash
    ${RV_DEPS_OPENJPH_DOWNLOAD_HASH}
)

IF(RV_TARGET_WINDOWS)
  RV_MAKE_STANDARD_LIB_NAME("openjph.${_version_major}.${_version_minor}" "${RV_DEPS_OPENJPH_VERSION}" "SHARED" "")
  SET(_libname
      "openjph.${_version_major}.${_version_minor}.lib"
  )
  SET(_implibpath
      ${_lib_dir}/${_libname}
  )
ELSE()
  RV_MAKE_STANDARD_LIB_NAME("openjph" "${RV_DEPS_OPENJPH_VERSION}" "SHARED" "")
ENDIF()
# The '_configure_options' list gets reset and initialized in 'RV_CREATE_STANDARD_DEPS_VARIABLES'

# Do not build the executables (Openjph calls them "codec executables"). BUILD_THIRDPARTY options is valid only if BUILD_CODEC=ON. PNG, TIFF and ZLIB are not
# needed anymore because they are used for the executables only.
LIST(APPEND _configure_options "-DBUILD_CODEC=OFF")

EXTERNALPROJECT_ADD(
  ${_target}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  DOWNLOAD_NAME ${_target}_${_version}.tar.gz
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${_source_dir}
  BINARY_DIR ${_build_dir}
  INSTALL_DIR ${_install_dir}
  # DEPENDS ZLIB::ZLIB TIFF::TIFF PNG::PNG
  CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
  BUILD_COMMAND ${_cmake_build_command}
  INSTALL_COMMAND ${_cmake_install_command}
  BUILD_IN_SOURCE FALSE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_byproducts}
  USES_TERMINAL_BUILD TRUE
)

RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} LIBNAME ${_libname})

SET(_openjph_include_dir
    "${_include_dir}"
)

IF(NOT RV_TARGET_WINDOWS)
  RV_ADD_IMPORTED_LIBRARY(
    NAME
    OpenJph::OpenJph
    TYPE
    SHARED
    LOCATION
    ${_libpath}
    SONAME
    ${_libname}
    INCLUDE_DIRS
    ${_openjph_include_dir}
    DEPENDS
    ${_target}
    ADD_TO_DEPS_LIST
  )
ELSE()
  RV_ADD_IMPORTED_LIBRARY(
    NAME
    OpenJph::OpenJph
    TYPE
    SHARED
    LOCATION
    ${_libpath}
    IMPLIB
    ${_implibpath}
    INCLUDE_DIRS
    ${_openjph_include_dir}
    DEPENDS
    ${_target}
    ADD_TO_DEPS_LIST
  )
ENDIF()
