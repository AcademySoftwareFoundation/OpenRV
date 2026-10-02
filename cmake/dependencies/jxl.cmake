#
# SPDX-License-Identifier: Apache-2.0
#

#
# Official sources: https://github.com/libjxl/libjxl
#
# Build instructions: https://github.com/libjxl/libjxl/blob/main/BUILDING.md
#
# libjxl is built from source. Rather than resolving highway, brotli, and skcms as separate RV dependencies, this module lets libjxl build its own pinned
# third_party submodules (see build/jxl.cmake).
#

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_JXL" "${RV_DEPS_JXL_VERSION}" "make" "")

# libjxl ships CMake CONFIG files (JxlConfig.cmake) that create jxl::jxl and jxl::jxl_threads. Fall back to pkg-config (libjxl) when CONFIG is unavailable.
RV_FIND_DEPENDENCY(
  TARGET
  ${_target}
  PACKAGE
  Jxl
  VERSION
  ${_version}
  PKG_CONFIG_NAME
  libjxl
  DEPS_LIST_TARGETS
  jxl::jxl
  jxl::jxl_threads
)

# jxl library naming (shared across the build and found paths).
IF(RV_TARGET_WINDOWS)
  RV_MAKE_STANDARD_LIB_NAME("jxl" "${RV_DEPS_JXL_VERSION}" "SHARED" "")
  SET(_libname
      "jxl.lib"
  )
  SET(_implibpath
      ${_lib_dir}/${_libname}
  )
ELSE()
  RV_MAKE_STANDARD_LIB_NAME("jxl" "${RV_DEPS_JXL_VERSION}" "SHARED" "")
ENDIF()

# jxl_threads library naming.
IF(RV_TARGET_WINDOWS)
  SET(_threads_libname
      "jxl_threads.lib"
  )
  SET(_threads_implibpath
      ${_lib_dir}/${_threads_libname}
  )
  SET(_threads_libpath
      ${_bin_dir}/jxl_threads${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ELSE()
  SET(_threads_libname
      "${CMAKE_SHARED_LIBRARY_PREFIX}jxl_threads${CMAKE_SHARED_LIBRARY_SUFFIX}"
  )
  SET(_threads_libpath
      ${_lib_dir}/${_threads_libname}
  )
ENDIF()

IF(NOT ${_target}_FOUND)
  INCLUDE(${CMAKE_CURRENT_LIST_DIR}/build/jxl.cmake)

  # A single staging call copies the whole install lib dir, so libjxl_cms and the bundled brotli runtime libraries (libbrotli*) that libjxl links against are
  # staged alongside libjxl automatically. highway and skcms are compiled statically into libjxl, so they produce no separate shared libraries.
  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} LIBNAME ${_libname})

  IF(NOT RV_TARGET_WINDOWS)
    RV_ADD_IMPORTED_LIBRARY(
      NAME
      jxl::jxl
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
    RV_ADD_IMPORTED_LIBRARY(
      NAME
      jxl::jxl_threads
      TYPE
      SHARED
      LOCATION
      ${_threads_libpath}
      SONAME
      ${_threads_libname}
      INCLUDE_DIRS
      ${_include_dir}
      DEPENDS
      ${_target}
      ADD_TO_DEPS_LIST
    )
  ELSE()
    RV_ADD_IMPORTED_LIBRARY(
      NAME
      jxl::jxl
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
    RV_ADD_IMPORTED_LIBRARY(
      NAME
      jxl::jxl_threads
      TYPE
      SHARED
      LOCATION
      ${_threads_libpath}
      IMPLIB
      ${_threads_implibpath}
      INCLUDE_DIRS
      ${_include_dir}
      DEPENDS
      ${_target}
      ADD_TO_DEPS_LIST
    )
  ENDIF()
ELSE()
  # A pre-built libjxl was found (CONFIG or pkg-config). Create the jxl::* targets from the resolved install when CONFIG did not already provide them.
  IF(NOT TARGET jxl::jxl)
    SET(_jxl_found_lib
        "${_lib_dir}/${CMAKE_SHARED_LIBRARY_PREFIX}jxl${CMAKE_SHARED_LIBRARY_SUFFIX}"
    )
    RV_ADD_IMPORTED_LIBRARY(
      NAME
      jxl::jxl
      TYPE
      SHARED
      LOCATION
      ${_jxl_found_lib}
      INCLUDE_DIRS
      ${_include_dir}
      DEPENDS
      ${_target}
    )
    LIST(APPEND RV_DEPS_LIST jxl::jxl)
    RV_RESOLVE_DARWIN_INSTALL_NAME(jxl::jxl)
  ENDIF()

  IF(NOT TARGET jxl::jxl_threads)
    SET(_jxl_threads_found_lib
        "${_lib_dir}/${CMAKE_SHARED_LIBRARY_PREFIX}jxl_threads${CMAKE_SHARED_LIBRARY_SUFFIX}"
    )
    RV_ADD_IMPORTED_LIBRARY(
      NAME
      jxl::jxl_threads
      TYPE
      SHARED
      LOCATION
      ${_jxl_threads_found_lib}
      INCLUDE_DIRS
      ${_include_dir}
      DEPENDS
      ${_target}
    )
    LIST(APPEND RV_DEPS_LIST jxl::jxl_threads)
    RV_RESOLVE_DARWIN_INSTALL_NAME(jxl::jxl_threads)
  ENDIF()

  RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} TARGET_LIBS jxl::jxl jxl::jxl_threads)
ENDIF()
