#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

FIND_PACKAGE(OpenSSL REQUIRED)
IF(TARGET OpenSSL::Crypto)
  SET_PROPERTY(
    TARGET OpenSSL::Crypto
    PROPERTY IMPORTED_GLOBAL TRUE
  )
ENDIF()
IF(TARGET OpenSSL::SSL)
  SET_PROPERTY(
    TARGET OpenSSL::SSL
    PROPERTY IMPORTED_GLOBAL TRUE
  )
ENDIF()
