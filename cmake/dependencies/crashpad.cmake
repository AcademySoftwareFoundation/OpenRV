#
# Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(ProcessorCount) # require CMake 3.15+
PROCESSORCOUNT(_cpu_count)

# Crashpad - https://github.com/getsentry/crashpad The commit pin (RV_DEPS_CRASHPAD_GIT_TAG) lives in cmake/defaults/CYCOMMON.cmake alongside the Breakpad pin,
# per the version-pinning contract (C7).
RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_CRASHPAD" "${RV_DEPS_CRASHPAD_GIT_TAG}" "" "")

# getsentry/crashpad is a fork of Google's Crashpad with native CMake support. mini_chromium is a git submodule (third_party/mini_chromium), so we must use
# GIT_REPOSITORY + GIT_SUBMODULES rather than a zip archive download. Note: Using commit hash since the repo doesn't have versioned releases.

# Include directories: - crashpad headers are at the source root (e.g. <client/crashpad_client.h>) - mini_chromium submodule checks out into
# third_party/mini_chromium/; within that repo, headers live under a nested mini_chromium/ subdirectory
SET(_crashpad_include_dir
    ${_source_dir}
)
SET(_mini_chromium_include_dir
    ${_source_dir}/third_party/mini_chromium/mini_chromium
)

# Library names
SET(_crashpad_client_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}crashpad_client${CMAKE_STATIC_LIBRARY_SUFFIX}
)
SET(_crashpad_util_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}crashpad_util${CMAKE_STATIC_LIBRARY_SUFFIX}
)
# getsentry/crashpad names the mini_chromium library with an underscore
SET(_minichromium_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}mini_chromium${CMAKE_STATIC_LIBRARY_SUFFIX}
)
# getsentry/crashpad splits mpack into its own library (not present in older forks)
SET(_crashpad_mpack_lib_name
    ${CMAKE_STATIC_LIBRARY_PREFIX}crashpad_mpack${CMAKE_STATIC_LIBRARY_SUFFIX}
)

# Library paths in install directory
SET(_crashpad_client_lib
    ${_lib_dir}/${_crashpad_client_lib_name}
)
SET(_crashpad_util_lib
    ${_lib_dir}/${_crashpad_util_lib_name}
)
SET(_minichromium_lib
    ${_lib_dir}/${_minichromium_lib_name}
)
SET(_crashpad_mpack_lib
    ${_lib_dir}/${_crashpad_mpack_lib_name}
)

# Handler executable
IF(RV_TARGET_WINDOWS)
  SET(_crashpad_handler_name
      crashpad_handler.exe
  )
ELSE()
  SET(_crashpad_handler_name
      crashpad_handler
  )
ENDIF()

SET(_crashpad_handler
    ${_bin_dir}/${_crashpad_handler_name}
)

# Configure options (CMAKE_INSTALL_PREFIX, CMAKE_BUILD_TYPE, CMAKE_OSX_ARCHITECTURES, -S and -B are already appended by RV_CREATE_STANDARD_DEPS_VARIABLES)
LIST(APPEND _configure_options "-DCMAKE_POSITION_INDEPENDENT_CODE=ON")
LIST(APPEND _configure_options "-DCRASHPAD_BUILD_TESTS=OFF")

# Disable deprecation warnings on macOS — mini_chromium uses deprecated Security APIs
IF(RV_TARGET_DARWIN)
  LIST(APPEND _configure_options "-DCMAKE_CXX_FLAGS=-Wno-error=deprecated-declarations -Wno-deprecated-declarations")
  LIST(APPEND _configure_options "-DCMAKE_C_FLAGS=-Wno-error=deprecated-declarations -Wno-deprecated-declarations")
ENDIF()

# crashpad's CMake selects zlib via the CRASHPAD_ZLIB_SYSTEM option. On non-MSVC it defaults ON (find_package(ZLIB)); on MSVC it defaults OFF and builds the
# bundled third_party/zlib, which requires a nested zlib git submodule we don't fetch -- hence the Windows "No SOURCES given to target: crashpad_zlib" failure.
# Force CRASHPAD_ZLIB_SYSTEM=ON everywhere and point it at RV's already-built zlib (ZLIB::ZLIB) so there is a single zlib across the build and no extra
# submodule checkout is needed.
LIST(APPEND _configure_options "-DCRASHPAD_ZLIB_SYSTEM=ON")
RV_RESOLVE_IMPORTED_LINKER_FILE(ZLIB::ZLIB _crashpad_zlib_library)
GET_TARGET_PROPERTY(_crashpad_zlib_include_dir ZLIB::ZLIB INTERFACE_INCLUDE_DIRECTORIES)
LIST(APPEND _configure_options "-DZLIB_ROOT=${RV_DEPS_ZLIB_ROOT_DIR}")
LIST(APPEND _configure_options "-DZLIB_LIBRARY=${_crashpad_zlib_library}")
LIST(APPEND _configure_options "-DZLIB_INCLUDE_DIR=${_crashpad_zlib_include_dir}")

# On Linux, crashpad defaults to libcurl for HTTP uploads. RV stores dumps locally only, so a patch replaces the libcurl transport with a stub (no
# libcurl/libldap/OpenSSL).
SET(_crashpad_patch_command
    ${CMAKE_COMMAND} -E true
)
IF(RV_TARGET_LINUX)
  SET(_crashpad_patch_command
      sh -c
      "${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/patch/crashpad_http_transport_disabled.cc util/net/http_transport_disabled.cc && patch -p1 -N < ${CMAKE_CURRENT_LIST_DIR}/patch/crashpad_linux_disable_http_transport.patch"
  )
ENDIF()

SET(_crashpad_depends
    ZLIB::ZLIB
)

# third_party/lss (Linux Syscall Support) is only used on Linux.
IF(RV_TARGET_LINUX)
  SET(_crashpad_submodules
      "third_party/mini_chromium" "third_party/lss"
  )
ELSE()
  SET(_crashpad_submodules
      "third_party/mini_chromium"
  )
ENDIF()

EXTERNALPROJECT_ADD(
  ${_target}
  GIT_REPOSITORY "https://github.com/getsentry/crashpad.git"
  GIT_TAG ${_version}
  GIT_SUBMODULES ${_crashpad_submodules}
  GIT_SHALLOW FALSE
  GIT_PROGRESS TRUE
  UPDATE_DISCONNECTED TRUE
  SOURCE_DIR ${_source_dir}
  BINARY_DIR ${_build_dir}
  INSTALL_DIR ${_install_dir}
  PATCH_COMMAND ${_crashpad_patch_command}
  DEPENDS ${_crashpad_depends}
  CONFIGURE_COMMAND ${CMAKE_COMMAND} ${_configure_options}
  BUILD_COMMAND ${_cmake_build_command}
  INSTALL_COMMAND ${_cmake_install_command}
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_crashpad_client_lib} ${_crashpad_util_lib} ${_minichromium_lib} ${_crashpad_mpack_lib} ${_crashpad_handler}
  USES_TERMINAL_BUILD TRUE
)

# crashpad_client depends on crashpad_mpack (split out by getsentry/crashpad); create it first so consumers of crashpad::client pull it in automatically via the
# INTERFACE_LINK_LIBRARIES set below.
RV_ADD_IMPORTED_LIBRARY(
  NAME
  crashpad::mpack
  TYPE
  STATIC
  LOCATION
  ${_crashpad_mpack_lib}
  DEPENDS
  ${_target}
)

# Create imported target for crashpad::client
RV_ADD_IMPORTED_LIBRARY(
  NAME
  crashpad::client
  TYPE
  STATIC
  LOCATION
  ${_crashpad_client_lib}
  SONAME
  ${_crashpad_client_lib_name}
  INCLUDE_DIRS
  ${_crashpad_include_dir}
  ${_mini_chromium_include_dir}
  DEPENDS
  ${_target}
  ADD_TO_DEPS_LIST
)
# RV_ADD_IMPORTED_LIBRARY does not cover INTERFACE_LINK_LIBRARIES; declare the crashpad::mpack dependency here.
SET_PROPERTY(
  TARGET crashpad::client
  PROPERTY INTERFACE_LINK_LIBRARIES crashpad::mpack
)

# Create imported target for crashpad::util
RV_ADD_IMPORTED_LIBRARY(
  NAME
  crashpad::util
  TYPE
  STATIC
  LOCATION
  ${_crashpad_util_lib}
  SONAME
  ${_crashpad_util_lib_name}
  INCLUDE_DIRS
  ${_crashpad_include_dir}
  ${_mini_chromium_include_dir}
  DEPENDS
  ${_target}
  ADD_TO_DEPS_LIST
)

# Create imported target for crashpad::minichromium (base library)
RV_ADD_IMPORTED_LIBRARY(
  NAME
  crashpad::minichromium
  TYPE
  STATIC
  LOCATION
  ${_minichromium_lib}
  SONAME
  ${_minichromium_lib_name}
  INCLUDE_DIRS
  ${_mini_chromium_include_dir}
  DEPENDS
  ${_target}
  ADD_TO_DEPS_LIST
)

# Stage libraries and crashpad_handler executable
IF(RV_TARGET_WINDOWS)
  ADD_CUSTOM_COMMAND(
    COMMENT
      "Staging ${_target}'s libs and handler into ${RV_STAGE_LIB_DIR} and ${RV_STAGE_BIN_DIR}, then converting crashpad_handler.exe to Windows GUI application"
    OUTPUT ${RV_STAGE_LIB_DIR}/${_crashpad_client_lib_name} ${RV_STAGE_BIN_DIR}/${_crashpad_handler_name}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${RV_STAGE_LIB_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_bin_dir} ${RV_STAGE_BIN_DIR}
    COMMAND editbin /SUBSYSTEM:WINDOWS ${RV_STAGE_BIN_DIR}/${_crashpad_handler_name}
    DEPENDS ${_target}
  )
  ADD_CUSTOM_TARGET(
    ${_target}-stage-target ALL
    DEPENDS ${RV_STAGE_LIB_DIR}/${_crashpad_client_lib_name}
    DEPENDS ${RV_STAGE_BIN_DIR}/${_crashpad_handler_name}
  )
ELSE()
  SET(_crashpad_stage_post_commands
      ""
  )
  IF(RV_TARGET_DARWIN)
    SET(_crashpad_stage_post_commands
        COMMAND ${CMAKE_INSTALL_NAME_TOOL} -add_rpath @executable_path/../lib ${RV_STAGE_BIN_DIR}/${_crashpad_handler_name} COMMAND ${CMAKE_INSTALL_NAME_TOOL}
        -change @rpath/libz.1.dylib @rpath/libz.dylib ${RV_STAGE_BIN_DIR}/${_crashpad_handler_name} COMMAND codesign --force --sign -
        ${RV_STAGE_BIN_DIR}/${_crashpad_handler_name}
    )
  ELSEIF(RV_TARGET_LINUX)
    SET(_crashpad_stage_post_commands
        COMMAND patchelf --set-rpath \$$ORIGIN/../lib ${RV_STAGE_BIN_DIR}/${_crashpad_handler_name}
    )
  ENDIF()
  ADD_CUSTOM_COMMAND(
    COMMENT "Staging ${_target}'s libs and handler into ${RV_STAGE_LIB_DIR} and ${RV_STAGE_BIN_DIR}"
    OUTPUT ${RV_STAGE_LIB_DIR}/${_crashpad_client_lib_name} ${RV_STAGE_BIN_DIR}/${_crashpad_handler_name}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${RV_STAGE_LIB_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_bin_dir} ${RV_STAGE_BIN_DIR} ${_crashpad_stage_post_commands}
    DEPENDS ${_target}
  )
  ADD_CUSTOM_TARGET(
    ${_target}-stage-target ALL
    DEPENDS ${RV_STAGE_LIB_DIR}/${_crashpad_client_lib_name}
    DEPENDS ${RV_STAGE_BIN_DIR}/${_crashpad_handler_name}
  )
ENDIF()

ADD_DEPENDENCIES(dependencies ${_target}-stage-target)

SET(RV_DEPS_CRASHPAD_VERSION
    ${_version}
    CACHE INTERNAL "" FORCE
)
