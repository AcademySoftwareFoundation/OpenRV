#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

MESSAGE(STATUS "Adding \"CMAKE_MINIMUM_REQUIRED(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})\" to INSTALL")

INSTALL(CODE "CMAKE_MINIMUM_REQUIRED(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})")

GET_CMAKE_PROPERTY(_variableNames VARIABLES)
LIST(SORT _variableNames)
FOREACH(
  _variableName
  ${_variableNames}
)
  IF(_variableName MATCHES "^RV_")
    IF(${_variableName})
      MESSAGE(STATUS "Adding VARIABLE ${_variableName} to INSTALL")
      SET(_install_safe_value
          "${${_variableName}}"
      )
      IF(RV_TARGET_WINDOWS)
        # Convert backslashes to forward slashes so Windows paths like "D:\a\OpenRV\..." don't produce invalid escape sequences (\a, \O, etc.) in the generated
        # cmake_install.cmake.
        STRING(REPLACE "\\" "/" _install_safe_value "${_install_safe_value}")
      ENDIF()
      INSTALL(CODE "SET(${_variableName} ${_install_safe_value})")
    ENDIF()
  ENDIF()
ENDFOREACH()

MESSAGE(STATUS "Adding SCRIPT ${CMAKE_CURRENT_LIST_DIR}/pre_install.cmake to INSTALL")

IF(RV_TARGET_DARWIN)
  MESSAGE(STATUS "Adding SCRIPT ${CMAKE_CURRENT_LIST_DIR}/pre_install_darwin.cmake to INSTALL")
  INSTALL(SCRIPT ${CMAKE_CURRENT_LIST_DIR}/pre_install_darwin.cmake)
ELSEIF(RV_TARGET_LINUX)
  MESSAGE(STATUS "Adding SCRIPT ${CMAKE_CURRENT_LIST_DIR}/pre_install_linux.cmake to INSTALL")
  INSTALL(SCRIPT ${CMAKE_CURRENT_LIST_DIR}/pre_install_linux.cmake)
ELSEIF(RV_TARGET_WINDOWS)
  MESSAGE(STATUS "Adding SCRIPT ${CMAKE_CURRENT_LIST_DIR}/pre_install_windows.cmake to INSTALL")
  INSTALL(SCRIPT ${CMAKE_CURRENT_LIST_DIR}/pre_install_windows.cmake)
ENDIF()

INSTALL(SCRIPT ${CMAKE_CURRENT_LIST_DIR}/pre_install.cmake)

MESSAGE(STATUS "Adding SCRIPT ${CMAKE_CURRENT_LIST_DIR}/install_darwin.cmake to INSTALL")
INSTALL(SCRIPT ${CMAKE_CURRENT_LIST_DIR}/install.cmake)

MESSAGE(STATUS "Adding SCRIPT ${CMAKE_CURRENT_LIST_DIR}/post_install.cmake to INSTALL")
INSTALL(SCRIPT ${CMAKE_CURRENT_LIST_DIR}/post_install.cmake)
IF(RV_TARGET_DARWIN)
  MESSAGE(STATUS "Adding SCRIPT ${CMAKE_CURRENT_LIST_DIR}/post_install_darwin.cmake to INSTALL")
  INSTALL(SCRIPT ${CMAKE_CURRENT_LIST_DIR}/post_install_darwin.cmake)
ELSEIF(RV_TARGET_LINUX)
  MESSAGE(STATUS "Adding SCRIPT ${CMAKE_CURRENT_LIST_DIR}/post_install_linux.cmake to INSTALL")
  INSTALL(SCRIPT ${CMAKE_CURRENT_LIST_DIR}/post_install_linux.cmake)
ELSEIF(RV_TARGET_WINDOWS)
  MESSAGE(STATUS "Adding SCRIPT ${CMAKE_CURRENT_LIST_DIR}/post_install_windows.cmake to INSTALL")
  INSTALL(SCRIPT ${CMAKE_CURRENT_LIST_DIR}/post_install_windows.cmake)
ENDIF()
