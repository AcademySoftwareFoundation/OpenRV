#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

SET(_target
    "RV_DEPS_OPENSSL"
)

IF(RV_TARGET_IS_RHEL8)
  FIND_PACKAGE(OpenSSL 1.1.1 REQUIRED)

  SET(RV_DEPS_OPENSSL_VERSION
      ${OPENSSL_VERSION}
      CACHE INTERNAL "" FORCE
  )

  SET(_include_dir
      ${OPENSSL_INCLUDE_DIR}
  )
  GET_FILENAME_COMPONENT(_lib_dir "${OPENSSL_SSL_LIBRARY}" DIRECTORY)

  MESSAGE(STATUS "_include_dir ${_include_dir}")
  MESSAGE(STATUS "_lib_dir ${_lib_dir}")

ELSE()
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
  string(REPLACE "." "_" _version_underscored ${_version})

  IF(RV_TARGET_WINDOWS
     AND (NOT RV_DEPS_WIN_PERL_ROOT
          OR RV_DEPS_WIN_PERL_ROOT STREQUAL "")
  )
    MESSAGE(
      FATAL_ERROR
        "Unable to build without a RV_DEPS_WIN_PERL_ROOT. OpenSSL requires a Windows native perl interpreter to build (it recommends https://strawberryperl.com/). Example -DRV_DEPS_WIN_PERL_ROOT=c:/Strawberry/perl/bin"
    )
  ENDIF()

  SET(RV_DEPS_OPENSSL_INSTALL_DIR
      ${RV_DEPS_BASE_DIR}/${_target}/install
  )
  SET(_include_dir
      ${RV_DEPS_OPENSSL_INSTALL_DIR}/include
  )
  SET(_source_dir
      ${RV_DEPS_BASE_DIR}/${_target}/src
  )
  SET(_build_dir
      ${RV_DEPS_BASE_DIR}/${_target}/build
  )
  SET(_lib_dir
      ${RV_DEPS_OPENSSL_INSTALL_DIR}/lib
  )
  SET(_bin_dir
      ${RV_DEPS_OPENSSL_INSTALL_DIR}/bin
  )

  SET(_download_url
      "https://github.com/openssl/openssl/releases/download/OpenSSL_${_version_underscored}/openssl-${_version}.tar.gz"
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
  LIST(APPEND _make_command ${RV_DEPS_OPENSSL_INSTALL_DIR})
  IF(RV_TARGET_WINDOWS)
    LIST(APPEND _make_command "--perlroot")
    LIST(APPEND _make_command ${RV_DEPS_WIN_PERL_ROOT})
  ENDIF()

  IF(APPLE)
    # This is needed because if Rosetta is used to compile for x86_64 from ARM64,
    # openssl build system detects it as "linux-x86_64" and it causes issues.

    IF(RV_TARGET_APPLE_X86_64)
      SET(__openssl_arch__ x86_64)
    ELSEIF(RV_TARGET_APPLE_ARM64)
      SET(__openssl_arch__ arm64)
    ENDIF()

    LIST(APPEND _make_command --arch=-${__openssl_arch__})
  ENDIF()
  
  # On most POSIX platforms, shared libraries are named `libcrypto.so.1.1`
  # and `libssl.so.1.1`.

  # On Windows build with MSVC or using MingW, shared libraries are named
  # `libcrypto-1_1.dll` and `libssl-1_1.dll` for 32-bit Windows,
  # `libcrypto-1_1-x64.dll` and `libssl-1_1-x64.dll` for 64-bit x86_64 Windows,
  # and `libcrypto-1_1-ia64.dll` and `libssl-1_1-ia64.dll` for IA64 Windows.
  # With MSVC, the import libraries are named `libcrypto.lib` and `libssl.lib`,
  # while with MingW, they are named `libcrypto.dll.a` and `libssl.dll.a`.

  # Ref: https://github.com/openssl/openssl/blob/398011848468c7e8e481b295f7904afc30934217/INSTALL.md?plain=1#L1847-L1858

  IF(RV_TARGET_LINUX)
    SET(_crypto_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}crypto${CMAKE_SHARED_LIBRARY_SUFFIX}.1.1
    )

    SET(_ssl_lib_name
        ${CMAKE_SHARED_LIBRARY_PREFIX}ssl${CMAKE_SHARED_LIBRARY_SUFFIX}.1.1
    )
  ELSEIF(RV_TARGET_WINDOWS)
    # As stated in the openssl documentation, the names are libcrypto-1_1-x64 and libssl-1_1-x64
    # when OpenSSL is build with MSVC.
    SET(_crypto_lib_name
      libcrypto-1_1-x64${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
    SET(_ssl_lib_name
      libssl-1_1-x64${CMAKE_SHARED_LIBRARY_SUFFIX}
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

  EXTERNALPROJECT_ADD(
    ${_target}
    DOWNLOAD_NAME ${_target}_${_version}.zip
    DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_DIR ${_source_dir}
    INSTALL_DIR ${RV_DEPS_OPENSSL_INSTALL_DIR}
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
  IF(RV_TARGET_WINDOWS)
    SET_PROPERTY(
      TARGET OpenSSL::Crypto
      PROPERTY IMPORTED_IMPLIB ${_implibpath_crypto}
    )
  ENDIF()
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
  IF(RV_TARGET_WINDOWS)
    SET_PROPERTY(
      TARGET OpenSSL::SSL
      PROPERTY IMPORTED_IMPLIB ${_implibpath_ssl}
    )
  ENDIF()
  TARGET_INCLUDE_DIRECTORIES(
    OpenSSL::SSL
    INTERFACE ${_include_dir}
  )
  LIST(APPEND RV_DEPS_LIST OpenSSL::SSL)

  SET(_openssl_stage_lib_dir
      ${RV_STAGE_LIB_DIR}
  )

  IF(RV_TARGET_WINDOWS)
    ADD_CUSTOM_COMMAND(
      TARGET ${_target}
      POST_BUILD
      COMMENT "Renaming the openssl import libs to the name FFmpeg is expecting"
      COMMAND ${CMAKE_COMMAND} -E copy ${RV_DEPS_OPENSSL_INSTALL_DIR}/lib/libssl.lib ${_lib_dir}/ssl.lib
      COMMAND ${CMAKE_COMMAND} -E copy ${RV_DEPS_OPENSSL_INSTALL_DIR}/lib/libcrypto.lib ${_lib_dir}/crypto.lib
    )
    ADD_CUSTOM_COMMAND(
      COMMENT "Installing ${_target}'s libs and bin into ${RV_STAGE_LIB_DIR} and ${RV_STAGE_BIN_DIR}"
      OUTPUT ${RV_STAGE_LIB_DIR}/${_crypto_lib_name} ${RV_STAGE_LIB_DIR}/${_ssl_lib_name}
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${RV_STAGE_LIB_DIR}
      COMMAND ${CMAKE_COMMAND} -E copy ${_bin_dir}/${_crypto_lib_name} ${RV_STAGE_BIN_DIR}
      COMMAND ${CMAKE_COMMAND} -E copy ${_bin_dir}/${_ssl_lib_name} ${RV_STAGE_BIN_DIR}
      DEPENDS ${_target}
    )
    ADD_CUSTOM_TARGET(
      ${_target}-stage-target ALL
      DEPENDS ${RV_STAGE_LIB_DIR}/${_crypto_lib_name} ${RV_STAGE_LIB_DIR}/${_ssl_lib_name}
    )
  ELSE()

    # Because RHEL8 has the same version of openssl library as we use but is not compatible with our library, we will copy openssl into its own seperate lib
    # directory and conditionally add it to the LD_LIBRARY_PATH if the version we build does not match the system version. This will allow RHEL8 to use its own
    # system version
    #
    IF(RV_TARGET_LINUX)
      SET(_openssl_stage_lib_dir
          ${_openssl_stage_lib_dir}/OpenSSL
      )
    ENDIF()

    ADD_CUSTOM_COMMAND(
      COMMENT "Installing ${_target}'s libs into ${_openssl_stage_lib_dir}"
      OUTPUT ${_openssl_stage_lib_dir}/${_crypto_lib_name} ${_openssl_stage_lib_dir}/${_ssl_lib_name}
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${_openssl_stage_lib_dir}
      DEPENDS ${_target}
    )
    ADD_CUSTOM_TARGET(
      ${_target}-stage-target ALL
      DEPENDS ${_openssl_stage_lib_dir}/${_crypto_lib_name} ${_openssl_stage_lib_dir}/${_ssl_lib_name}
    )
  ENDIF()

  ADD_DEPENDENCIES(dependencies ${_target}-stage-target)

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