#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "TwkGLFFBO"
)

LIST(APPEND _sources FBOVideoDevice.cpp)

ADD_LIBRARY(
  ${_target} STATIC
  ${_sources}
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# Need that with Qt6 since the code for this target does not support unicode correctly.
IF(RV_TARGET_WINDOWS
   AND RV_VFX_PLATFORM STRGREATER_EQUAL CY2024
)
  ADD_DEFINITIONS(-UUNICODE -U_UNICODE)
ENDIF()

TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC TwkGLF
  PRIVATE TwkExc
)

IF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE glew
  )
ELSE()
  FIND_PACKAGE(OpenGL REQUIRED)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE OpenGL::GL
  )

  IF(RV_TARGET_LINUX)
    TARGET_LINK_LIBRARIES(
      ${_target}
      PRIVATE OpenGL::GLX X11
    )
  ENDIF()
ENDIF()

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
