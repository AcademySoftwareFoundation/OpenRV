#
# Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
        "PyMediaLibrary"
)

SET(_sources
        PyMediaLibrary.cpp
        PyRootNode.cpp
        PyMediaNode.cpp
        PyNode.cpp
)

ADD_LIBRARY(
        ${_target} STATIC
        ${_sources}
)

TARGET_LINK_LIBRARIES(
        ${_target}
        PUBLIC RvApp TwkMediaLibrary Python::Python
        PRIVATE TwkPython TwkUtil
)

TARGET_INCLUDE_DIRECTORIES(
        ${_target}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
