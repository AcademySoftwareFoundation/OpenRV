#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "utf8"
)

ADD_LIBRARY(${_target} INTERFACE IMPORTED GLOBAL)
SET_TARGET_PROPERTIES(
  ${_target}
  PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}"
)
