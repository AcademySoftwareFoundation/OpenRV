#
# Copyright (C) 2025  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

CMAKE_MINIMUM_REQUIRED(VERSION 3.27)
PROJECT(pyimgui)

SET(_target
    "pyimgui"
)

FIND_PACKAGE(
  Python
  COMPONENTS Interpreter Development
  REQUIRED
)

FIND_PACKAGE(nanobind CONFIG REQUIRED)

SET(IMGUI_NB_SOURCES
    imgui_pywrappers/imgui_pywrappers.cpp bindings/pybind_imgui_module.cpp
)

SET(IMGUI_NB_HEADERS
    imgui_pywrappers/imgui_pywrappers.h
)

# Overwrite NB_SUFFIX to control the name of the output library.
IF(RV_TARGET_WINDOWS)
  SET(NB_SUFFIX ".pyd")
ELSE()
  SET(NB_SUFFIX "${CMAKE_SHARED_MODULE_SUFFIX}")
ENDIF()

NANOBIND_ADD_MODULE(pyimgui ${IMGUI_NB_SOURCES})

# Set the correct suffix for Windows Python modules
IF(RV_TARGET_WINDOWS)
  SET_TARGET_PROPERTIES(pyimgui PROPERTIES SUFFIX ".pyd")
ENDIF()

TARGET_LINK_LIBRARIES(
  pyimgui
  PUBLIC ${imgui_LIBRARY}
)

TARGET_COMPILE_DEFINITIONS(
  pyimgui
  PUBLIC IMGUI_BUNDLE_PYTHON_API IMGUI_DISABLE_DEMO_WINDOWS
)

TARGET_INCLUDE_DIRECTORIES(
  pyimgui
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui_pywrappers ${CMAKE_CURRENT_SOURCE_DIR}/bindings ${imgui_INCLUDE_DIRS}
)

SET_TARGET_PROPERTIES(
  pyimgui
  PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

INSTALL(
  TARGETS pyimgui
  LIBRARY DESTINATION lib
)
