#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "MovieSideCar"
)

SET(_sources
    MovieSideCarIO.cpp MovieSideCar.cpp SideCar.cpp Common.cpp
)

ADD_LIBRARY(
  ${_target} STATIC
  ${_sources}
)

FIND_PACKAGE(
  Qt5
  COMPONENTS Core
  REQUIRED
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC Qt5::Core TwkMovie Boost::thread
  PRIVATE IOgto
          TwkExc
          TwkFB
          TwkMath
          TwkUtil
          Boost::date_time
          Boost::filesystem
          Boost::program_options
          resample
          stl_ext
)

IF(RV_TARGET_DARWIN)
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-Wno-switch"
  )
ENDIF()

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
