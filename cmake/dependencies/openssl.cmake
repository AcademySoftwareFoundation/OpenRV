#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

SET(RV_DEPS_WIN_PERL_ROOT
    ""
    CACHE STRING "Path to Windows perl root"
)

SET(_target
    "RV_DEPS_OPENSSL"
)

SET(_version
    "1.1.1u"
)

IF(RV_TARGET_WINDOWS
   AND (NOT RV_DEPS_WIN_PERL_ROOT
        OR RV_DEPS_WIN_PERL_ROOT STREQUAL "")
)
  MESSAGE(
    FATAL_ERROR
      "Unable to build without a RV_DEPS_WIN_PERL_ROOT. OpenSSL requires a Windows native perl interpreter to build (it recommends https://strawberryperl.com/). Example -DRV_DEPS_WIN_PERL_ROOT=c:/Strawberry/perl/bin"
  )
ENDIF()

SET(_install_dir
    ${RV_DEPS_BASE_DIR}/${_target}/install
)
SET(_source_dir
    ${RV_DEPS_BASE_DIR}/${_target}/src
)
SET(_build_dir
    ${RV_DEPS_BASE_DIR}/${_target}/build
)
SET(RV_DEPS_OPENSSL_LIB_DIR
    ${_install_dir}/lib
)

SET(_download_url
    "https://www.openssl.org/source/openssl-${_version}.tar.gz"
)
SET(_download_hash
    "72f7ba7395f0f0652783ba1089aa0dcc"
)

SET(_make_command_script
    "${PROJECT_SOURCE_DIR}/src/build/make_openssl.py"
)
SET(_make_command
    python3 "${_make_command_script}"
)

LIST(APPEND _make_command "--source-dir")
LIST(APPEND _make_command ${_source_dir})
LIST(APPEND _make_command "--output-dir")
LIST(APPEND _make_command ${_install_dir})
IF(RV_TARGET_WINDOWS)
  LIST(APPEND _make_command "--perlroot")
  LIST(APPEND _make_command ${RV_DEPS_WIN_PERL_ROOT})
ENDIF()

IF(${RV_OSX_EMULATION})
  LIST(APPEND _make_command --arch=${RV_OSX_EMULATION_ARCH})
ENDIF()

IF(RV_TARGET_IS_RHEL9
   OR RV_TARGET_IS_RHEL8
)
  SET(_crypto_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}crypto${CMAKE_SHARED_LIBRARY_SUFFIX}.1.1
  )

  SET(_ssl_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}ssl${CMAKE_SHARED_LIBRARY_SUFFIX}.1.1
  )
ELSE()
  SET(_crypto_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}crypto.1.1${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
  SET(_ssl_lib_name
      ${CMAKE_SHARED_LIBRARY_PREFIX}ssl.1.1${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
ENDIF()

SET(_crypto_lib
    ${RV_DEPS_OPENSSL_LIB_DIR}/${_crypto_lib_name}
)

SET(_ssl_lib
    ${RV_DEPS_OPENSSL_LIB_DIR}/${_ssl_lib_name}
)

# IF(RV_TARGET_IS_RHEL9 OR RV_TARGET_IS_RHEL8) SET(_crypto_lib_name ${CMAKE_SHARED_LIBRARY_PREFIX}crypto${CMAKE_SHARED_LIBRARY_SUFFIX}.1.1 ) SET(_crypto_lib
# ${RV_DEPS_OPENSSL_LIB_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}crypto${CMAKE_SHARED_LIBRARY_SUFFIX}.1.1 ) SET(_ssl_lib_name
# ${CMAKE_SHARED_LIBRARY_PREFIX}ssl${CMAKE_SHARED_LIBRARY_SUFFIX}.1.1 ) SET(_ssl_lib
# ${RV_DEPS_OPENSSL_LIB_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}ssl${CMAKE_SHARED_LIBRARY_SUFFIX}.1.1 ) ENDIF()

EXTERNALPROJECT_ADD(
  ${_target}
  DOWNLOAD_NAME ${_target}_${_version}.zip
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_DIR ${_source_dir}
  INSTALL_DIR ${_install_dir}
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  CONFIGURE_COMMAND ${_make_command} --configure
  BUILD_COMMAND ${_make_command} --build
  INSTALL_COMMAND ${_make_command} --install
  BUILD_IN_SOURCE TRUE
  BUILD_ALWAYS FALSE
  BUILD_BYPRODUCTS ${_crypto_lib} ${_ssl_lib}
  USES_TERMINAL_BUILD TRUE
)

SET(_include_dir
    ${_install_dir}/include
)
FILE(MAKE_DIRECTORY ${_include_dir})

ADD_LIBRARY(OpenSSL::Crypto SHARED IMPORTED GLOBAL)
ADD_DEPENDENCIES(OpenSSL::Crypto ${_target})
SET_PROPERTY(
  TARGET OpenSSL::Crypto
  PROPERTY IMPORTED_LOCATION ${_crypto_lib}
)
SET_PROPERTY(
  TARGET OpenSSL::Crypto
  PROPERTY IMPORTED_SONAME ${_crypto_lib_name}
)
TARGET_INCLUDE_DIRECTORIES(
  OpenSSL::Crypto
  INTERFACE ${_include_dir}
)
LIST(APPEND RV_DEPS_LIST OpenSSL::Crypto)

ADD_LIBRARY(OpenSSL::SSL SHARED IMPORTED GLOBAL)
ADD_DEPENDENCIES(OpenSSL::SSL ${_target})
SET_PROPERTY(
  TARGET OpenSSL::SSL
  PROPERTY IMPORTED_LOCATION ${_ssl_lib}
)
SET_PROPERTY(
  TARGET OpenSSL::SSL
  PROPERTY IMPORTED_SONAME ${_ssl_lib_name}
)
TARGET_INCLUDE_DIRECTORIES(
  OpenSSL::SSL
  INTERFACE ${_include_dir}
)
LIST(APPEND RV_DEPS_LIST OpenSSL::SSL)

IF(RV_TARGET_WINDOWS)
  ADD_CUSTOM_COMMAND(
    TARGET ${_target}
    POST_BUILD
    COMMENT "Installing ${_target}'s libs and bin into ${RV_STAGE_LIB_DIR} and ${RV_STAGE_BIN_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_install_dir}/lib ${RV_STAGE_LIB_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${_install_dir}/bin ${RV_STAGE_BIN_DIR}
  )
  ADD_CUSTOM_TARGET(
    ${_target}-stage-target ALL
    DEPENDS ${RV_STAGE_BIN_DIR}/${_crypto_lib_name} ${RV_STAGE_BIN_DIR}/${_ssl_lib_name}
  )
ELSEIF(RV_TARGET_IS_RHEL8)
  # Blank target on RHEL8 Linux to avoid copying RV's OpenSSL files.
  ADD_CUSTOM_TARGET(
    ${_target}-stage-target ALL
  )
ELSE()
  ADD_CUSTOM_COMMAND(
    COMMENT "Installing ${_target}'s libs into ${RV_STAGE_LIB_DIR}"
    OUTPUT ${RV_STAGE_LIB_DIR}/${_crypto_lib_name} ${RV_STAGE_LIB_DIR}/${_ssl_lib_name}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${RV_DEPS_OPENSSL_LIB_DIR} ${RV_STAGE_LIB_DIR}
    DEPENDS ${_target}
  )
  ADD_CUSTOM_TARGET(
    ${_target}-stage-target ALL
    DEPENDS ${RV_STAGE_LIB_DIR}/${_crypto_lib_name} ${RV_STAGE_LIB_DIR}/${_ssl_lib_name}
  )
ENDIF()

ADD_DEPENDENCIES(dependencies ${_target}-stage-target)

SET(RV_DEPS_OPENSSL_VERSION
    ${_version}
    CACHE INTERNAL "" FORCE
)
