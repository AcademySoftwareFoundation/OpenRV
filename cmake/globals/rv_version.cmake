#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
SET(RV_VERSION_YEAR
    "2023"
    CACHE STRING "RV's year of release."
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
