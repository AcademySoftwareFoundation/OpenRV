#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# Modified for the Visto project. Copyright (C) 2026  Makai Systems. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

FIND_PACKAGE(
  Python3 REQUIRED
  COMPONENTS Interpreter Development
)

IF(NOT TARGET Python::Python)
  ADD_LIBRARY(Python::Python INTERFACE IMPORTED GLOBAL)
  TARGET_LINK_LIBRARIES(
    Python::Python
    INTERFACE Python3::Python
  )
  TARGET_INCLUDE_DIRECTORIES(
    Python::Python
    INTERFACE ${Python3_INCLUDE_DIRS}
  )
ENDIF()

SET(RV_DEPS_PYTHON3_EXECUTABLE
    ${Python3_EXECUTABLE}
    CACHE INTERNAL "" FORCE
)

# Install requirements using uv if available
FIND_PROGRAM(UV_EXECUTABLE uv)
IF(UV_EXECUTABLE)
  MESSAGE(STATUS "Using uv for python dependency management")
  EXECUTE_PROCESS(
    COMMAND ${UV_EXECUTABLE} pip install -r ${PROJECT_SOURCE_DIR}/requirements.txt
    RESULT_VARIABLE uv_result
  )
ELSE()
  MESSAGE(STATUS "uv not found, using pip for python dependency management")
  EXECUTE_PROCESS(
    COMMAND ${Python3_EXECUTABLE} -m pip install -r ${PROJECT_SOURCE_DIR}/requirements.txt
    RESULT_VARIABLE pip_result
  )
ENDIF()
