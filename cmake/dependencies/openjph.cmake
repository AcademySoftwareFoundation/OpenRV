#
# SPDX-License-Identifier: Apache-2.0
#

#
# Official sources: https://github.com/aous72/OpenJPH
#
# Build instructions: https://github.com/aous72/OpenJPH/blob/master/docs/compiling.md
#

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_OPENJPH" "${RV_DEPS_OPENJPH_VERSION}" "make" "")
IF(RV_TARGET_LINUX)
  # Overriding _lib_dir created in 'RV_CREATE_STANDARD_DEPS_VARIABLES' since this CMake-based project isn't using lib64
  SET(_lib_dir
      ${_install_dir}/lib
  )
ENDIF()

# OpenJPH ships CMake CONFIG files (openjph-config.cmake). CONFIG creates target `openjph` (not namespaced). Fall back to pkg-config.
RV_FIND_DEPENDENCY(
  TARGET
  ${_target}
  PACKAGE
  openjph
  VERSION
  ${_version}
  PKG_CONFIG_NAME
  openjph
  DEPS_LIST_TARGETS
  openjph
)

SET(_download_url
    "https://github.com/aous72/OpenJPH/archive/refs/tags/${_version}.tar.gz"
)
SET(_download_hash
    ${RV_DEPS_OPENJPH_DOWNLOAD_HASH}
)

# Shared naming logic (used by both build and found paths)
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

IF(NOT ${_target}_FOUND)
  INCLUDE(${CMAKE_CURRENT_LIST_DIR}/build/openjph.cmake)

  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} LIBNAME ${_libname})

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
      ${_include_dir}
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
      ${_include_dir}
      DEPENDS
      ${_target}
      ADD_TO_DEPS_LIST
    )
  ENDIF()
ELSE()
  # CONFIG creates `openjph` target; pkg-config does not. Create it if missing (e.g. found via pkg-config).
  IF(NOT TARGET openjph)
    # Resolve actual library. On Windows, DLLs are in _bin_dir and import libs in _lib_dir. On Unix, shared libs are in _lib_dir.
    SET(_openjph_found_lib
        ""
    )
    SET(_openjph_found_implib
        ""
    )
    IF(RV_TARGET_WINDOWS)
      FILE(GLOB _openjph_found_dlls "${_bin_dir}/openjph*${CMAKE_SHARED_LIBRARY_SUFFIX}")
      IF(_openjph_found_dlls)
        LIST(GET _openjph_found_dlls 0 _openjph_found_lib)
      ENDIF()
      FILE(GLOB _openjph_found_implibs "${_lib_dir}/openjph*.lib")
      IF(_openjph_found_implibs)
        LIST(GET _openjph_found_implibs 0 _openjph_found_implib)
      ENDIF()
    ELSE()
      FILE(GLOB _openjph_found_libs "${_lib_dir}/${CMAKE_SHARED_LIBRARY_PREFIX}openjph${CMAKE_SHARED_LIBRARY_SUFFIX}"
           "${_lib_dir}/${CMAKE_SHARED_LIBRARY_PREFIX}openjph.*${CMAKE_SHARED_LIBRARY_SUFFIX}*"
      )
      IF(_openjph_found_libs)
        LIST(GET _openjph_found_libs 0 _openjph_found_lib)
      ENDIF()
    ENDIF()
    IF(NOT _openjph_found_lib)
      SET(_openjph_found_lib
          "${_lib_dir}/${CMAKE_SHARED_LIBRARY_PREFIX}openjph${CMAKE_SHARED_LIBRARY_SUFFIX}"
      )
    ENDIF()
    RV_ADD_IMPORTED_LIBRARY(
      NAME
      openjph
      TYPE
      SHARED
      LOCATION
      ${_openjph_found_lib}
      IMPLIB
      ${_openjph_found_implib}
      INCLUDE_DIRS
      ${_include_dir}
      DEPENDS
      ${_target}
    )
    LIST(APPEND RV_DEPS_LIST openjph)
    RV_RESOLVE_DARWIN_INSTALL_NAME(openjph)
  ENDIF()

  # Create `OpenJph::OpenJph` as an INTERFACE wrapper for backward compatibility.
  IF(NOT TARGET OpenJph::OpenJph)
    ADD_LIBRARY(OpenJph::OpenJph INTERFACE IMPORTED GLOBAL)
    TARGET_LINK_LIBRARIES(
      OpenJph::OpenJph
      INTERFACE openjph
    )
  ENDIF()

  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} TARGET_LIBS openjph)
ENDIF()
