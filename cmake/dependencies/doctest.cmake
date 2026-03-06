#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

SET(_target
    "RV_DEPS_DOCTEST"
)

SET(_version
    ${RV_DEPS_DOCTEST_VERSION}
)

SET(_download_url
    "https://github.com/doctest/doctest/archive/refs/tags/v${_version}.tar.gz"
)
SET(_download_hash
    ${RV_DEPS_DOCTEST_DOWNLOAD_HASH}
)

SET(_install_dir
    ${RV_DEPS_BASE_DIR}/${_target}/install
)
SET(_include_dir
    ${_install_dir}/include
)

# This is an include-only archive, we don't want to build anything
EXTERNALPROJECT_ADD(
  ${_target}
  PREFIX ${RV_DEPS_BASE_DIR}/${_target}
  DOWNLOAD_NAME ${_target}_${_version}.tar.gz
  DOWNLOAD_DIR ${RV_DEPS_DOWNLOAD_DIR}
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  URL ${_download_url}
  URL_MD5 ${_download_hash}
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  BUILD_ALWAYS FALSE
  TIMEOUT 10
  LOG_DOWNLOAD ON
)

ADD_LIBRARY(doctest::doctest INTERFACE IMPORTED GLOBAL)
ADD_DEPENDENCIES(doctest::doctest ${_target})
TARGET_INCLUDE_DIRECTORIES(
  doctest::doctest
  INTERFACE "${RV_DEPS_BASE_DIR}/${_target}/src/${_target}"
)
