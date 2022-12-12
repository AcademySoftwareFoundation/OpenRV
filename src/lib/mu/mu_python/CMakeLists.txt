#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "mu_python"
)

SET(_sources
    init.cpp
)

ADD_LIBRARY(
  ${_target} SHARED
  ${_sources}
)

SET_TARGET_PROPERTIES(
  ${_target}
  PROPERTIES OUTPUT_NAME "python"
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PRIVATE Mu MuPy MuLang
)

RV_STAGE(TYPE "MU_PLUGIN" TARGET ${_target})
