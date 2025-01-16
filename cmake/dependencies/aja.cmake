#
# Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_AJA" "17.1.0" "make" "")
RV_SHOW_STANDARD_DEPS_VARIABLES()

STRING(REPLACE "." "_" _version_with_underscore ${_version})

SET(_download_url
    "https://github.com/aja-video/libajantv2/archive/refs/tags/ntv2_${_version_with_underscore}.zip"
)

SET(_download_hash
    "b9d189f77e18dbdff7c39a339b1a5dd4"
)

IF(RV_TARGET_WINDOWS)
  RV_MAKE_STANDARD_LIB_NAME(ajantv2_vs143_MT "" "SHARED" "d")
ELSE()
  RV_MAKE_STANDARD_LIB_NAME(ajantv2 "" "SHARED" "d")
ENDIF()

SET(_aja_ntv2_include_dir
    ${_include_dir}/libajantv2/ajantv2/includes
)
SET(_aja_include_dir
    ${_include_dir}/libajantv2
)

IF(RHEL_VERBOSE)
SET(_mbedtls_lib_dir
    ${_build_dir}/ajantv2/mbedtls-install/lib64
)
ELSE()
SET(_mbedtls_lib_dir
    ${_build_dir}/ajantv2/mbedtls-install/lib
)
ENDIF()

SET(_mbedtls_lib ${_mbedtls_lib_dir}/${CMAKE_STATIC_LIBRARY_PREFIX}mbedtls${CMAKE_STATIC_LIBRARY_SUFFIX})
SET(_mbedx509_lib ${_mbedtls_lib_dir}/${CMAKE_STATIC_LIBRARY_PREFIX}mbedx509${CMAKE_STATIC_LIBRARY_SUFFIX})
SET(_mbedcrypto_lib ${_mbedtls_lib_dir}/${CMAKE_STATIC_LIBRARY_PREFIX}mbedcrypto${CMAKE_STATIC_LIBRARY_SUFFIX})

LIST(APPEND _byproducts ${_mbedtls_lib} ${_mbedx509_lib} ${_mbedcrypto_lib})

# There is an issue with the recent AJA SDK : the OS specific header files are no longer copied to _aja_ntv2_include_dir Adding custom paths here to work around
# this issue
IF(RV_TARGET_LINUX)
  SET(_aja_ntv2_os_specific_include_dir
      ${_include_dir}/libajantv2/ajantv2/src/lin
  )
ELSEIF(RV_TARGET_DARWIN)
  SET(_aja_ntv2_os_specific_include_dir
      ${_include_dir}/libajantv2/ajantv2/src/mac
  )
ELSEIF(RV_TARGET_WINDOWS)
  SET(_aja_ntv2_os_specific_include_dir
      ${_include_dir}/libajantv2/ajantv2/src/win
  )
ENDIF()

LIST(APPEND
  _configure_options
  "-DAJANTV2_DISABLE_DEMOS=ON"
  "-DAJANTV2_DISABLE_TOOLS=ON"
  "-DAJANTV2_DISABLE_TESTS=ON"
  "-DAJANTV2_BUILD_SHARED=ON"
)

# In Debug, the MSVC runtime library needs to be set to MultiThreadedDebug. Otherwise, it will be set to MultiThreaded.
IF(RV_TARGET_WINDOWS AND CMAKE_BUILD_TYPE MATCHES "^Debug$")
  LIST(APPEND
  _configure_options
  "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebug"
  )
ENDIF()

EXTERNALPROJECT_ADD(
  ${_target}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  DOWNLOAD_NAME ${_target}_${_version}.zip
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${_source_dir}
  BINARY_DIR ${_build_dir}
  INSTALL_DIR ${_install_dir}
  CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
  BUILD_COMMAND ${_cmake_build_command}
  INSTALL_COMMAND ${_cmake_install_command} && ${CMAKE_COMMAND} -E copy_directory ${_mbedtls_lib_dir} ${_lib_dir}
  BUILD_IN_SOURCE FALSE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_byproducts}
  USES_TERMINAL_BUILD TRUE
)

RV_COPY_LIB_BIN_FOLDERS()

ADD_LIBRARY(aja::ntv2 SHARED IMPORTED GLOBAL)
ADD_DEPENDENCIES(aja::ntv2 ${_target})
SET_PROPERTY(
  TARGET aja::ntv2
  PROPERTY IMPORTED_LOCATION ${_libpath}
)
SET_PROPERTY(
  TARGET aja::ntv2
  PROPERTY IMPORTED_SONAME ${_libname}
)
IF(RV_TARGET_WINDOWS)
  SET_PROPERTY(
    TARGET aja::ntv2
    PROPERTY IMPORTED_IMPLIB ${_implibpath}
  )
ENDIF()

FILE(MAKE_DIRECTORY ${_aja_include_dir} ${_aja_ntv2_include_dir} ${_aja_ntv2_os_specific_include_dir})
TARGET_INCLUDE_DIRECTORIES(
  aja::ntv2
  INTERFACE ${_aja_include_dir} ${_aja_ntv2_include_dir} ${_aja_ntv2_os_specific_include_dir}
)

TARGET_LINK_LIBRARIES(
  aja::ntv2 INTERFACE
  ${_mbedtls_lib} ${_mbedx509_lib} ${_mbedcrypto_lib}
)

IF(RV_TARGET_DARWIN)
  LIST(APPEND _aja_compile_options "-DAJAMac=1")
  LIST(APPEND _aja_compile_options "-DAJA_MAC=1")
ELSEIF(RV_TARGET_LINUX)
  LIST(APPEND _aja_compile_options "-DAJALinux=1")
  LIST(APPEND _aja_compile_options "-DAJA_LINUX=1")
ELSEIF(RV_TARGET_WINDOWS)
  LIST(APPEND _aja_compile_options "-DMSWindows=1")
  LIST(APPEND _aja_compile_options "-DAJA_WINDOWS=1")
ENDIF()
SET(RV_DEPS_AJA_COMPILE_OPTIONS
    ${_aja_compile_options}
    CACHE INTERNAL "" FORCE
)

LIST(APPEND RV_DEPS_LIST aja::ntv2)

ADD_DEPENDENCIES(dependencies ${_target}-stage-target)

SET(RV_DEPS_AJA_VERSION
    ${_version}
    CACHE INTERNAL "" FORCE
)
