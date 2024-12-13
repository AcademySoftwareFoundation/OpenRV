#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_GC" "8.2.2" "" "")

SET(_download_url
    "https://github.com/ivmai/bdwgc/archive/refs/tags/v${_version}.zip"
)
SET(_download_hash
    "2ca38d05e1026b3426cf6c24ca3a7787"
)

SET(_install_dir
    ${RV_DEPS_BASE_DIR}/${_target}/install
)

IF(RV_TARGET_LINUX)
  SET(_lib_dir
      ${_install_dir}/lib64
  )
ELSE()
  SET(_lib_dir
      ${_install_dir}/lib
  )
ENDIF()

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

SET(_include_dir
    ${_install_dir}/include
)
FILE(MAKE_DIRECTORY ${_include_dir})

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
ADD_LIBRARY(BDWGC::Gc STATIC IMPORTED GLOBAL)

ADD_DEPENDENCIES(BDWGC::Gc ${_target})
SET_PROPERTY(
  TARGET BDWGC::Gc
  PROPERTY IMPORTED_LOCATION ${_gc_lib}
)
SET_PROPERTY(
  TARGET BDWGC::Gc
  PROPERTY IMPORTED_SONAME ${_gc_lib_name}
)
TARGET_INCLUDE_DIRECTORIES(
  BDWGC::Gc
  INTERFACE ${_include_dir}
)
LIST(APPEND RV_DEPS_LIST BDWGC::Gc)

IF(NOT RV_TARGET_WINDOWS)
  ADD_LIBRARY(BDWGC::Cord SHARED IMPORTED GLOBAL)
  ADD_DEPENDENCIES(BDWGC::Cord ${_target})
  SET_PROPERTY(
    TARGET BDWGC::Cord
    PROPERTY IMPORTED_LOCATION ${_cord_lib}
  )
  SET_PROPERTY(
    TARGET BDWGC::Cord
    PROPERTY IMPORTED_SONAME ${_cord_lib_name}
  )
  TARGET_INCLUDE_DIRECTORIES(
    BDWGC::Cord
    INTERFACE ${_include_dir}
  )
  LIST(APPEND RV_DEPS_LIST BDWGC::Cord)
ENDIF()

ADD_CUSTOM_COMMAND(
  COMMENT "Installing ${_target}'s libs into ${RV_STAGE_LIB_DIR}"
  OUTPUT ${_gc_stage_outputs}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${RV_STAGE_LIB_DIR}
  DEPENDS ${_target}
)
ADD_CUSTOM_TARGET(
  ${_target}-stage-target ALL
  DEPENDS ${_gc_stage_outputs}
)

ADD_DEPENDENCIES(dependencies ${_target}-stage-target)

SET(RV_DEPS_GC_VERSION
    ${_version}
    CACHE INTERNAL ""
)
