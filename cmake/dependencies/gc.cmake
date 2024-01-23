#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

SET(_target
    "RV_DEPS_GC"
)

SET(_version
    "8.2.2"
)

SET(_download_url
    "https://github.com/ivmai/bdwgc/archive/refs/tags/v${_version}.zip"
)
SET(_download_hash
    "2ca38d05e1026b3426cf6c24ca3a7787"
)

SET(_install_dir
    ${RV_DEPS_BASE_DIR}/${_target}/install
)

SET(_make_command
    make
)
SET(_autogen_command
    sh ./autogen.sh
)
SET(_configure_command
    sh ./configure
)

IF(${RV_OSX_EMULATION})
  SET(_darwin_x86_64
      "arch" "${RV_OSX_EMULATION_ARCH}"
  )

  SET(_make_command
      ${_darwin_x86_64} ${_make_command}
  )
  SET(_autogen_command
      ${_darwin_x86_64} ${_autogen_command}
  )
  SET(_configure_command
      ${_darwin_x86_64} ${_configure_command}
  )
ENDIF()
IF(RV_TARGET_WINDOWS)
  # MSYS2/CMake defaults to Ninja
  SET(_make_command
      ninja
  )
ENDIF()

SET(_lib_dir
    ${_install_dir}/lib
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

SET(_include_dir
    ${_install_dir}/include
)
FILE(MAKE_DIRECTORY ${_include_dir})

IF(RV_TARGET_WINDOWS)
  SET(_cmake_configure_command
      ${CMAKE_COMMAND}
  )
  LIST(APPEND _cmake_configure_command "-DCMAKE_INSTALL_PREFIX=${_install_dir}")
  LIST(APPEND _cmake_configure_command "-DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}")
  LIST(APPEND _cmake_configure_command "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
  LIST(APPEND _cmake_configure_command "-Denable_parallel_mark=ON")
  LIST(APPEND _cmake_configure_command "-Denable_cplusplus=ON")
  LIST(APPEND _cmake_configure_command "-DCMAKE_USE_WIN32_THREADS_INIT=1")
  LIST(APPEND _cmake_configure_command "${RV_DEPS_BASE_DIR}/${_target}/src")
  EXTERNALPROJECT_ADD(
    ${_target}
    DOWNLOAD_NAME ${_target}_${_version}.zip
    DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_DIR ${RV_DEPS_BASE_DIR}/${_target}/src
    INSTALL_DIR ${_install_dir}
    URL ${_download_url}
    URL_MD5 ${_download_hash}
    CONFIGURE_COMMAND ${_cmake_configure_command}
    BUILD_COMMAND ${_make_command} -j${_cpu_count}
    INSTALL_COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source
                    "${RV_DEPS_BASE_DIR}/${_target}/src/include" --destination "${_include_dir}/gc"
    COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source
            "${RV_DEPS_BASE_DIR}/${_target}/src/${_gc_lib_name}" --destination "${_gc_lib}/${_gc_lib_name}"
    BUILD_IN_SOURCE TRUE
    BUILD_ALWAYS FALSE
    BUILD_BYPRODUCTS ${_gc_byproducts}
    USES_TERMINAL_BUILD TRUE
  )
  ADD_LIBRARY(BDWGC::Gc STATIC IMPORTED GLOBAL)
ELSE()
  SET(_configure_args
      "--enable-cplusplus"
  )
  LIST(APPEND _configure_args "--prefix=${_install_dir}")

  EXTERNALPROJECT_ADD(
    ${_target}
    DOWNLOAD_NAME ${_target}_${_version}.zip
    DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_DIR ${RV_DEPS_BASE_DIR}/${_target}/src
    INSTALL_DIR ${_install_dir}
    URL ${_download_url}
    URL_MD5 ${_download_hash}
    CONFIGURE_COMMAND ${_autogen_command} && ${_configure_command} ${_configure_args}
    BUILD_COMMAND ${_make_command} -j${_cpu_count}
    INSTALL_COMMAND ${_make_command} install
    COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_install_dir}" --destination
            "${CMAKE_BINARY_DIR}"
    COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_install_dir}/lib" --destination
            "${RV_STAGE_LIB_DIR}"
    BUILD_IN_SOURCE TRUE
    BUILD_ALWAYS FALSE
    BUILD_BYPRODUCTS ${_gc_byproducts}
    USES_TERMINAL_BUILD TRUE
  )
  ADD_LIBRARY(BDWGC::Gc SHARED IMPORTED GLOBAL)
ENDIF()

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
  COMMAND python3 "${OPENRV_ROOT}/src/build/copy_third_party.py" --build-root "${CMAKE_BINARY_DIR}" --source "${_lib_dir}" --destination "${RV_STAGE_LIB_DIR}"
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
