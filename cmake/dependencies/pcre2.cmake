#
# Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_PCRE2" "pcre2-10.43" "make" "")
RV_SHOW_STANDARD_DEPS_VARIABLES()

SET(_download_url
    "https://github.com/PCRE2Project/pcre2/archive/refs/tags/${_version}.zip"
)

SET(_download_hash
    "e4c3f2a24eb5c15bec8360e50b3f0137"
)

SET(_install_dir
    ${RV_DEPS_BASE_DIR}/${_target}/install
)

IF(CMAKE_BUILD_TYPE MATCHES "^Debug$")
    SET(_pcre2_debug_postfix_ "d")
ENDIF()

# PCRE is not used for Linux and MacOS (Boost regex is used) in the current code.
IF(RV_TARGET_WINDOWS)
    SET(_pcre2_libname
        pcre2-8${_pcre2_debug_postfix_}${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
    SET(_pcre2_libname_posix
        pcre2-posix${_pcre2_debug_postfix_}${CMAKE_SHARED_LIBRARY_SUFFIX}
    )

    SET(_pcre2_implibname
        pcre2-8${_pcre2_debug_postfix_}.lib
    )
    SET(_pcre2_implibname_posix
        pcre2-posix${_pcre2_debug_postfix_}.lib
    )

    SET(_pcre2_libpath
        ${_bin_dir}/${_pcre2_libname}
    )
    SET(_pcre2_libpath_posix
        ${_bin_dir}/${_pcre2_libname_posix}
    )

    SET(_pcre2_implibpath
        ${_lib_dir}/${_pcre2_implibname}
    )
    SET(_pcre2_implibpath_posix
        ${_lib_dir}/${_pcre2_implibname_posix}
    )
ENDIF()

SET(_pcre2_include_dir
    ${_install_dir}/include
)

LIST(APPEND _pcre2_cmake_configure_args "-G ${CMAKE_GENERATOR}")
LIST(APPEND _pcre2_cmake_configure_args "-S ${_source_dir}")
LIST(APPEND _pcre2_cmake_configure_args "-B ${_build_dir}")

LIST(APPEND _pcre2_cmake_configure_args "-DBUILD_SHARED_LIBS=ON")
LIST(APPEND _pcre2_cmake_configure_args "-DBUILD_STATIC_LIBS=OFF")
LIST(APPEND _pcre2_cmake_configure_args "-DPCRE2_BUILD_PCRE2GREP=OFF")
LIST(APPEND _pcre2_cmake_configure_args "-DPCRE2_SUPPORT_LIBZ=OFF")
LIST(APPEND _pcre2_cmake_configure_args "-DPCRE2_SUPPORT_LIBBZ2=OFF")
LIST(APPEND _pcre2_cmake_configure_args "-DPCRE2_BUILD_TESTS=OFF")

LIST(APPEND _pcre2_cmake_configure_args "-DPCRE2_STATIC_RUNTIME=OFF")

IF(CMAKE_BUILD_TYPE MATCHES "^Debug$")
    LIST(APPEND _pcre2_cmake_configure_args "-DPCRE2_DEBUG=ON")
ENDIF()

# Build PCRE2 using 8 bits options (same as PCRE "1").
LIST(APPEND _pcre2_cmake_configure_args "-DPCRE2_BUILD_PCRE2_8=ON")
LIST(APPEND _pcre2_cmake_configure_args "-DPCRE2_BUILD_PCRE2_16=OFF")
LIST(APPEND _pcre2_cmake_configure_args "-DPCRE2_BUILD_PCRE2_32=OFF")

LIST(APPEND _pcre2_cmake_build_args "--build" "${_build_dir}" "--config" "${CMAKE_BUILD_TYPE}")

LIST(APPEND
    _pcre2_cmake_install_args
    "--install"
    "${_build_dir}"
    "--prefix"
    "${_install_dir}"
    "--config"
    "${CMAKE_BUILD_TYPE}"
)

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
    DEPENDS ZLIB::ZLIB
    CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_pcre2_cmake_configure_args}
    BUILD_COMMAND ${CMAKE_COMMAND} ${_pcre2_cmake_build_args}
    INSTALL_COMMAND ${CMAKE_COMMAND} ${_pcre2_cmake_install_args}
    BUILD_IN_SOURCE FALSE
    BUILD_ALWAYS FALSE
    BUILD_BYPRODUCTS ${_pcre2_libname} ${_pcre2_libname_posix} ${_pcre2_implibname} ${_pcre2_implibname_posix}
    USES_TERMINAL_BUILD TRUE
)

# PCRE is not used for Linux and MacOS (Boost regex is used) in the current code.
ADD_CUSTOM_COMMAND(
    TARGET ${_target}
    POST_BUILD
    COMMENT "Installing ${_target}'s shared library into ${RV_STAGE_BIN_DIR}"
    # Copy library files manually since there are tools that are not needed in the bin folder.
    COMMAND ${CMAKE_COMMAND} -E copy ${_pcre2_libpath} ${_pcre2_libpath_posix} -t ${RV_STAGE_BIN_DIR}
)

ADD_CUSTOM_TARGET(
    ${_target}-stage-target ALL
    DEPENDS ${RV_STAGE_BIN_DIR}/${_pcre2_libname} ${RV_STAGE_BIN_DIR}/${_pcre2_libname_posix}
)

ADD_DEPENDENCIES(dependencies ${_target}-stage-target)

ADD_LIBRARY(pcre2-8 SHARED IMPORTED GLOBAL)
ADD_LIBRARY(pcre2-posix SHARED IMPORTED GLOBAL)

ADD_DEPENDENCIES(pcre2-8 ${_target})
ADD_DEPENDENCIES(pcre2-posix ${_target})

# Setup includes
SET(_pcre2_include_dir
    ${_install_dir}/include
)
FILE(MAKE_DIRECTORY ${_pcre2_include_dir})

# Setup pcre2 8-bits target
SET_TARGET_PROPERTIES(
    pcre2-8 PROPERTIES
    IMPORTED_LOCATION ${_pcre2_libpath}
    IMPORTED_IMPLIB ${_pcre2_implibpath}
)
TARGET_INCLUDE_DIRECTORIES(
  pcre2-8
  INTERFACE ${_pcre2_include_dir}
)
target_compile_definitions(pcre2-8 INTERFACE PCRE2_CODE_UNIT_WIDTH=8)

# Setup pcre2-posix target
SET_TARGET_PROPERTIES(
    pcre2-posix PROPERTIES
    IMPORTED_LOCATION ${_pcre2_libpath_posix}
    IMPORTED_IMPLIB ${_pcre2_implibpath_posix}
)
TARGET_INCLUDE_DIRECTORIES(
  pcre2-posix
  INTERFACE ${_pcre2_include_dir}
)

LIST(APPEND RV_DEPS_LIST pcre2-8 pcre2-posix)

SET(RV_DEPS_PCRE2_VERSION
    ${_version}
    CACHE INTERNAL "" FORCE
)