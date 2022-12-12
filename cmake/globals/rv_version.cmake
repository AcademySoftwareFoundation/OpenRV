#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

SET(RV_VERSION_YEAR
    "2023"
    CACHE STRING "RV's year of release."
)
SET(RV_MAJOR_VERSION
    "1"
    CACHE STRING "RV's version major"
)
SET(RV_MINOR_VERSION
    "0"
    CACHE STRING "RV's version minor"
)
SET(RV_REVISION_NUMBER
    "0"
    CACHE STRING "RV's revision number"
)

SET(RV_VERSION_STRING
    "${RV_MAJOR_VERSION}.${RV_MINOR_VERSION}.${RV_REVISION_NUMBER}"
)

IF(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  SET(RV_RELEASE_DESCRIPTION
      "Debug"
  )
ELSE()
  # as checked 'src/lib/app/RvCommon/RvApplication.cpp'
  SET(RV_RELEASE_DESCRIPTION
      "RELEASE"
  )
ENDIF()

SET(RV_INTERNAL_APPLICATION_NAME
    "OpenRV"
    CACHE STRING "RV's internal application name"
)
