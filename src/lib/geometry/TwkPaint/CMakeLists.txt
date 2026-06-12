#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "TwkPaint"
)

# Sources and headers come from the standalone twkpaint submodule.
SET(_twkpaint_src
    ${CMAKE_CURRENT_SOURCE_DIR}/../twkpaint-src/TwkPaint
)

SET(_sources
    ${_twkpaint_src}/Path.cpp ${_twkpaint_src}/Smoother.cpp ${_twkpaint_src}/StampPath.cpp
)

ADD_LIBRARY(
  ${_target} STATIC
  ${_sources}
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${_twkpaint_src}
)

TARGET_LINK_LIBRARIES(${_target} TwkMath)

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
