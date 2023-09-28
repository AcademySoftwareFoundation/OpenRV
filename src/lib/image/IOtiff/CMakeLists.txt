#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "IOtiff"
)

SET(_sources
    IOtiff.cpp
)

ADD_LIBRARY(
  ${_target} STATIC
  ${_sources}
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC TwkFB
  PRIVATE TwkUtil Tiff::Tiff stl_ext
)

ADD_LINK_OPTIONS("-Wl,-E")

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
