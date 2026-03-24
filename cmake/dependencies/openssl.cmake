#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

RV_CREATE_STANDARD_DEPS_VARIABLES("RV_DEPS_OPENSSL" "${RV_DEPS_OPENSSL_VERSION}" "" "")

IF(RV_TARGET_IS_RHEL8
   AND RV_VFX_PLATFORM STREQUAL CY2023
)
  # VFX2023 on Rocky Linux 8: use system OpenSSL 1.1.1
  FIND_PACKAGE(OpenSSL 1.1.1 REQUIRED)

  SET(RV_DEPS_OPENSSL_VERSION
      ${OPENSSL_VERSION}
      CACHE INTERNAL "" FORCE
  )

  SET(_include_dir
      ${OPENSSL_INCLUDE_DIR}
  )
  GET_FILENAME_COMPONENT(_lib_dir "${OPENSSL_SSL_LIBRARY}" DIRECTORY)

ELSE()
  # All other platforms: find-first dispatcher. Tries CONFIG, then MODULE (FindOpenSSL.cmake), then pkg-config. On systems where OpenSSL is not on the default
  # search path (e.g., keg-only on macOS, custom install on Linux/Windows), hint with CMAKE_PREFIX_PATH, OPENSSL_ROOT_DIR, or PKG_CONFIG_PATH.
  RV_FIND_DEPENDENCY(
    TARGET
    ${_target}
    PACKAGE
    OpenSSL
    VERSION
    ${_version}
    PKG_CONFIG_NAME
    openssl
    ALLOW_MODULE
    DEPS_LIST_TARGETS
    OpenSSL::Crypto
    OpenSSL::SSL
  )

  SET(_download_url
      "https://github.com/openssl/openssl/releases/download/openssl-${_version}/openssl-${_version}.tar.gz"
  )
  IF(${_version} STREQUAL "1.1.1u")
    STRING(REPLACE "." "_" _version_underscored ${_version})
    SET(_download_url
        "https://github.com/openssl/openssl/releases/download/OpenSSL_${_version_underscored}/openssl-${_version}.tar.gz"
    )
  ENDIF()
  SET(_download_hash
      ${RV_DEPS_OPENSSL_HASH}
  )

  # Shared naming logic (used by both build and found paths)
  SET(_dot_version
      ${RV_DEPS_OPENSSL_VERSION_DOT}
  )
  SET(_underscore_version
      ${RV_DEPS_OPENSSL_VERSION_UNDERSCORE}
  )

  IF(RV_TARGET_LINUX)
    SET(_crypto_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}crypto${CMAKE_SHARED_LIBRARY_SUFFIX}${_dot_version}
    )
    SET(_ssl_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}ssl${CMAKE_SHARED_LIBRARY_SUFFIX}${_dot_version}
    )
  ELSEIF(RV_TARGET_WINDOWS)
    SET(_crypto_lib_name
        libcrypto-${_underscore_version}-x64${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
    SET(_ssl_lib_name
        libssl-${_underscore_version}-x64${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
  ELSE()
    SET(_crypto_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}crypto${_dot_version}${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
    SET(_ssl_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}ssl${_dot_version}${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
  ENDIF()

  SET(_crypto_lib
      ${_lib_dir}/${_crypto_lib_name}
  )
  SET(_ssl_lib
      ${_lib_dir}/${_ssl_lib_name}
  )

  IF(RV_TARGET_WINDOWS)
    SET(_implibpath_crypto
        ${_lib_dir}/${CMAKE_IMPORT_LIBRARY_PREFIX}crypto${CMAKE_IMPORT_LIBRARY_SUFFIX}
    )
    SET(_implibpath_ssl
        ${_lib_dir}/${CMAKE_IMPORT_LIBRARY_PREFIX}ssl${CMAKE_IMPORT_LIBRARY_SUFFIX}
    )
  ENDIF()

  IF(NOT ${_target}_FOUND)
    INCLUDE(${CMAKE_CURRENT_LIST_DIR}/build/openssl.cmake)

    IF(RV_TARGET_WINDOWS)
      ADD_CUSTOM_COMMAND(
        COMMENT "Staging ${_target} libs into ${RV_STAGE_LIB_DIR} and ${RV_STAGE_BIN_DIR}"
        OUTPUT ${RV_STAGE_BIN_DIR}/${_crypto_lib_name} ${RV_STAGE_BIN_DIR}/${_ssl_lib_name}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${RV_STAGE_LIB_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${_bin_dir}/${_crypto_lib_name} ${RV_STAGE_BIN_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${_bin_dir}/${_ssl_lib_name} ${RV_STAGE_BIN_DIR}
        DEPENDS ${_target}
      )
      ADD_CUSTOM_TARGET(
        ${_target}-stage-target ALL
        DEPENDS ${RV_STAGE_BIN_DIR}/${_crypto_lib_name} ${RV_STAGE_BIN_DIR}/${_ssl_lib_name}
      )
      ADD_DEPENDENCIES(dependencies ${_target}-stage-target)
    ELSE()
      SET(_openssl_stage_lib_dir
          ${RV_STAGE_LIB_DIR}
      )
      IF(RV_TARGET_LINUX)
        SET(_openssl_stage_lib_dir
            ${_openssl_stage_lib_dir}/OpenSSL
        )
      ENDIF()

      RV_STAGE_DEPENDENCY_LIBS(
        TARGET
        ${_target}
        STAGE_LIB_DIR
        ${_openssl_stage_lib_dir}
        OUTPUTS
        ${_openssl_stage_lib_dir}/${_crypto_lib_name}
        ${_openssl_stage_lib_dir}/${_ssl_lib_name}
      )
    ENDIF()
  ELSE()
    FOREACH(
      _ssl_target
      OpenSSL::Crypto OpenSSL::SSL
    )
      IF(NOT TARGET ${_ssl_target})
        STRING(
          REGEX
          REPLACE ".*::" "" _ssl_short ${_ssl_target}
        )
        STRING(TOLOWER ${_ssl_short} _ssl_lower)
        RV_ADD_IMPORTED_LIBRARY(
          NAME
          ${_ssl_target}
          TYPE
          SHARED
          LOCATION
          ${_lib_dir}/lib${_ssl_lower}${CMAKE_SHARED_LIBRARY_SUFFIX}
          INCLUDE_DIRS
          ${_include_dir}
          DEPENDS
          ${_target}
        )
      ENDIF()
    ENDFOREACH()

    SET(_openssl_stage_lib_dir
        ${RV_STAGE_LIB_DIR}
    )
    IF(RV_TARGET_LINUX)
      SET(_openssl_stage_lib_dir
          ${_openssl_stage_lib_dir}/OpenSSL
      )
    ENDIF()

    RV_STAGE_DEPENDENCY_LIBS(
      TARGET
      ${_target}
      STAGE_LIB_DIR
      ${_openssl_stage_lib_dir}
      TARGET_LIBS
      OpenSSL::Crypto
      OpenSSL::SSL
    )
  ENDIF()

  SET(RV_DEPS_OPENSSL_INSTALL_DIR
      ${_install_dir}
  )

  SET(RV_DEPS_OPENSSL_VERSION
      ${_version}
      CACHE INTERNAL "" FORCE
  )

  # FFmpeg customization to make it use this version of openssl
  SET_PROPERTY(
    GLOBAL APPEND
    PROPERTY "RV_FFMPEG_DEPENDS" RV_DEPS_OPENSSL
  )
ENDIF()

SET_PROPERTY(
  GLOBAL APPEND
  PROPERTY "RV_FFMPEG_EXTRA_C_OPTIONS" "--extra-cflags=-I${_include_dir}"
)
IF(RV_TARGET_WINDOWS)
  SET_PROPERTY(
    GLOBAL APPEND
    PROPERTY "RV_FFMPEG_EXTRA_LIBPATH_OPTIONS" "--extra-ldflags=-LIBPATH:${_lib_dir}"
  )
ELSE()
  SET_PROPERTY(
    GLOBAL APPEND
    PROPERTY "RV_FFMPEG_EXTRA_LIBPATH_OPTIONS" "--extra-ldflags=-L${_lib_dir}"
  )
ENDIF()
SET_PROPERTY(
  GLOBAL APPEND
  PROPERTY "RV_FFMPEG_EXTERNAL_LIBS" "--enable-openssl"
)
# For found packages, expose the pkgconfig directory so FFmpeg's configure can detect OpenSSL via pkg-config. This is critical on Windows where FFmpeg's
# fallback `-lssl -lcrypto` translates to `ssl.lib`/`crypto.lib` (MSVC literal name), but vcpkg/OpenSSL 3.x ships `libssl.lib`/`libcrypto.lib`. The .pc files
# contain the correct `-llibssl -llibcrypto` flags. On Unix this is also useful when the found package is in a non-standard prefix.
IF(${_target}_FOUND
   AND EXISTS "${_lib_dir}/pkgconfig"
)
  SET_PROPERTY(
    GLOBAL APPEND
    PROPERTY "RV_DEPS_PKG_CONFIG_PATH" "${_lib_dir}/pkgconfig"
  )
ENDIF()
