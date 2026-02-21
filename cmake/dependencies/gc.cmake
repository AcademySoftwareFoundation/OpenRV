#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_GC" "${RV_DEPS_GC_VERSION}" "" "")

SET(_download_url
    "https://github.com/ivmai/bdwgc/archive/refs/tags/v${_version}.zip"
)
SET(_download_hash
    ${RV_DEPS_GC_DOWNLOAD_HASH}
)

SET(_gc_lib_name
    ${CMAKE_SHARED_LIBRARY_PREFIX}gc.1${CMAKE_SHARED_LIBRARY_SUFFIX}
)
SET(_cord_lib_name
    ${CMAKE_SHARED_LIBRARY_PREFIX}cord.1${CMAKE_SHARED_LIBRARY_SUFFIX}
)

IF(RV_TARGET_LINUX)
  IF(RV_TARGET_IS_RHEL9
     OR RV_TARGET_IS_RHEL8
  )
    SET(_gc_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}gc${CMAKE_SHARED_LIBRARY_SUFFIX}.1
    )
    SET(_cord_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}cord${CMAKE_SHARED_LIBRARY_SUFFIX}.1
    )
  ELSE() # Not RHEL
    SET(_gc_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}gc${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
    SET(_cord_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}cord${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
  ENDIF()
ENDIF()

IF(RV_TARGET_WINDOWS)
  SET(_gc_lib_name
      gc-lib.lib
  )
ENDIF()

SET(_gc_lib
    ${_lib_dir}/${_gc_lib_name}
)

LIST(APPEND _gc_byproducts ${_gc_lib})
LIST(APPEND _gc_stage_outputs ${RV_STAGE_LIB_DIR}/${_gc_lib_name})
IF(NOT RV_TARGET_WINDOWS)
  SET(_cord_lib
      ${_lib_dir}/${_cord_lib_name}
  )

  LIST(APPEND _gc_byproducts ${_cord_lib})
  LIST(APPEND _gc_stage_outputs ${RV_STAGE_LIB_DIR}/${_cord_lib_name})
ENDIF()

LIST(APPEND _configure_options "-Denable_parallel_mark=ON")
LIST(APPEND _configure_options "-Denable_cplusplus=ON")
LIST(APPEND _configure_options "-Denable_large_config=yes")
IF(RV_TARGET_WINDOWS)
  LIST(APPEND _configure_options "-DCMAKE_USE_WIN32_THREADS_INIT=1")
ELSE()
  LIST(APPEND _configure_options "-DCMAKE_USE_PTHREADS_INIT=1")
ENDIF()

EXTERNALPROJECT_ADD(
  ${_target}
  DOWNLOAD_NAME ${_target}_${_version}.zip
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${RV_DEPS_BASE_DIR}/${_target}/src
  INSTALL_DIR ${_install_dir}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
  BUILD_COMMAND ${_cmake_build_command}
  INSTALL_COMMAND ${_cmake_install_command}
  BUILD_IN_SOURCE TRUE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_gc_byproducts}
  USES_TERMINAL_BUILD TRUE
)
RV_ADD_IMPORTED_LIBRARY(
  NAME BDWGC::Gc
  TYPE STATIC
  LOCATION ${_gc_lib}
  SONAME ${_gc_lib_name}
  INCLUDE_DIRS ${_include_dir}
  DEPENDS ${_target}
  ADD_TO_DEPS_LIST
)

IF(NOT RV_TARGET_WINDOWS)
  RV_ADD_IMPORTED_LIBRARY(
    NAME BDWGC::Cord
    TYPE SHARED
    LOCATION ${_cord_lib}
    SONAME ${_cord_lib_name}
    INCLUDE_DIRS ${_include_dir}
    DEPENDS ${_target}
    ADD_TO_DEPS_LIST
  )
ENDIF()

RV_STAGE_DEPENDENCY_LIBS(TARGET ${_target} OUTPUTS ${_gc_stage_outputs})

