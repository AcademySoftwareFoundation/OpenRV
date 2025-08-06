#
# Copyright (C) 2025  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

CMAKE_MINIMUM_REQUIRED(VERSION 3.27)
PROJECT(pyimplot)

SET(_target
    "pyimplot"
)

FIND_PACKAGE(
  Python
  COMPONENTS Interpreter Development
  REQUIRED
)

FIND_PACKAGE(nanobind CONFIG REQUIRED)

SET(IMPLOT_NB_SOURCES
    bindings/pybind_implot_module.cpp
)

# Overwrite NB_SUFFIX to control the name of the output library.
IF(RV_TARGET_WINDOWS)
  SET(NB_SUFFIX ".pyd")
ELSE()
  SET(NB_SUFFIX "${CMAKE_SHARED_MODULE_SUFFIX}")
ENDIF()

NANOBIND_ADD_MODULE(pyimplot ${IMPLOT_NB_SOURCES})

# Set the correct suffix for Windows Python modules
IF(RV_TARGET_WINDOWS)
  SET_TARGET_PROPERTIES(pyimplot PROPERTIES SUFFIX ".pyd")
ENDIF()

TARGET_LINK_LIBRARIES(
  pyimplot
  PUBLIC ${imgui_LIBRARY}
)

TARGET_COMPILE_DEFINITIONS(
  pyimplot
  PUBLIC IMGUI_BUNDLE_PYTHON_API
)

TARGET_COMPILE_DEFINITIONS(
  pyimplot
  PUBLIC IMGUI_BUNDLE_WITH_IMPLOT
)

TARGET_INCLUDE_DIRECTORIES(
  pyimplot
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/bindings ${imgui_INCLUDE_DIRS}
)

SET_TARGET_PROPERTIES(
  pyimplot
  PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

INSTALL(
  TARGETS pyimplot
  LIBRARY DESTINATION lib
)
