#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# LibRaw official Web page: https://www.libraw.org/about LibRaw official sources:  https://www.libraw.org/data/LibRaw-0.21.1.tar.gz LibRaw build from sources:
# https://www.libraw.org/docs/Install-LibRaw-eng.html
#

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_RAW" "${RV_DEPS_RAW_VERSION}" "make" "../src/configure")
IF(RV_TARGET_LINUX)
  # Overriding _lib_dir created in 'RV_CREATE_STANDARD_DEPS_VARIABLES' since this autotools-based project isn't using lib64
  SET(_lib_dir
      ${_install_dir}/lib
  )
ENDIF()

# Force build from source: pre-built libraw packages may link against a shared lcms2 DLL that isn't bundled.  Building from source uses OpenRV's own LCMS
# (lcms.dll). We need to remove lcms from pub-sub and use lcms2 in RV.
SET(RV_DEPS_RAW_FORCE_BUILD
    ON
    CACHE BOOL "libraw must be built from source (requires OpenRV's own LCMS build)" FORCE
)

# LibRaw uses autotools (not CMake), so no CONFIG files. Fall back to pkg-config. List both targets naming because vcpkg creates libraw::raw and Conan creates
# libraw::libraw. DEPS_LIST_TARGETS works regardless of which package manager provides it.
RV_FIND_DEPENDENCY(
  TARGET
  ${_target}
  PACKAGE
  LibRaw
  VERSION
  ${_version}
  PKG_CONFIG_NAME
  libraw
  DEPS_LIST_TARGETS
  libraw::raw
  libraw::libraw
)

SET(_download_url
    "https://github.com/LibRaw/LibRaw/archive/refs/tags/${_version}.tar.gz"
)
SET(_download_hash
    "${RV_DEPS_RAW_DOWNLOAD_HASH}"
)

# LIBRAW_SHLIB_CURRENT value in libraw_version.h https://github.com/LibRaw/LibRaw/blob/master/libraw/libraw_version.h
SET(_libraw_lib_version
    ${RV_DEPS_RAW_VERSION_LIB}
)
IF(NOT RV_TARGET_WINDOWS)
  RV_MAKE_STANDARD_LIB_NAME("raw" "${_libraw_lib_version}" "SHARED" "")
ELSE()
  RV_MAKE_STANDARD_LIB_NAME("libraw" "${_libraw_lib_version}" "SHARED" "")
ENDIF()

IF(NOT ${_target}_FOUND)
  INCLUDE(${CMAKE_CURRENT_LIST_DIR}/build/raw.cmake)

  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} LIBNAME ${_libname})

  RV_ADD_IMPORTED_LIBRARY(
    NAME
    libraw::raw
    TYPE
    SHARED
    LOCATION
    ${_libpath}
    SONAME
    ${_libname}
    IMPLIB
    ${_implibpath}
    INCLUDE_DIRS
    ${_include_dir}
    DEPENDS
    ${_target}
    ADD_TO_DEPS_LIST
  )
ELSE()
  # libraw's .pc file sets Cflags to "-I.../include/libraw -I.../include". pkg-config picks up the first (subdirectory) but IOraw.cpp uses #include
  # <libraw/libraw.h> which needs the parent. Override _include_dir to the install root's include dir.
  GET_FILENAME_COMPONENT(_include_dir "${_install_dir}/include" ABSOLUTE)

  IF(NOT TARGET libraw::raw)
    IF(TARGET libraw::libraw)
      # Conan CMakeDeps creates libraw::libraw (namespaced). Create an alias so downstream code using the upstream/pkg-config name works.
      ADD_LIBRARY(libraw::raw INTERFACE IMPORTED GLOBAL)
      TARGET_LINK_LIBRARIES(
        libraw::raw
        INTERFACE libraw::libraw
      )
    ELSE()
      # pkg-config found path: create the imported target. pkg-config creates PkgConfig::libraw, not libraw::raw. Resolve the actual library file from
      # pkg-config (don't use _libname which has the hardcoded build-from-source version).
      SET(_raw_found_lib
          ""
      )
      IF(${_target}_PC_LINK_LIBRARIES)
        LIST(GET ${_target}_PC_LINK_LIBRARIES 0 _raw_found_lib)
      ENDIF()
      IF(NOT _raw_found_lib
         OR NOT EXISTS "${_raw_found_lib}"
      )
        # Fallback: find the library in _lib_dir
        FILE(GLOB _raw_found_libs "${_lib_dir}/${CMAKE_SHARED_LIBRARY_PREFIX}raw${CMAKE_SHARED_LIBRARY_SUFFIX}"
             "${_lib_dir}/${CMAKE_SHARED_LIBRARY_PREFIX}raw.*${CMAKE_SHARED_LIBRARY_SUFFIX}*"
        )
        IF(_raw_found_libs)
          LIST(GET _raw_found_libs 0 _raw_found_lib)
        ENDIF()
      ENDIF()

      RV_ADD_IMPORTED_LIBRARY(
        NAME
        libraw::raw
        TYPE
        SHARED
        LOCATION
        ${_raw_found_lib}
        INCLUDE_DIRS
        ${_include_dir}
        DEPENDS
        ${_target}
      )
    ENDIF()
  ENDIF()

  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} TARGET_LIBS libraw::raw)
ENDIF()
