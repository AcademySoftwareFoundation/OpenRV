#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

SET(RV_DEPS_QT5_LOCATION
    ""
    CACHE STRING "Path to pre-compiled Qt5"
)
SET(_target
    RV_DEPS_QT5
)
IF(NOT RV_DEPS_QT5_LOCATION
   OR RV_DEPS_QT5_LOCATION STREQUAL ""
)
  MESSAGE(
    FATAL_ERROR
      "Unable to build without a RV_DEPS_QT5_LOCATION. It is required to provide a working Qt 5.15 root path to build. Example: cmake .. -DRV_DEPS_QT5_LOCATION=/Users/rv/Qt/5.15.11/clang_64"
  )
ENDIF()

SET(RV_DEPS_QT5_RESOURCES_FOLDER
    "${RV_DEPS_QT5_LOCATION}/resources"
    CACHE STRING "Path to the Qt resources files folder"
)
SET(RV_DEPS_QT5_TRANSLATIONS_FOLDER
    "${RV_DEPS_QT5_LOCATION}/translations"
    CACHE STRING "Path to the Qt translations files folder"
)

FILE(GLOB QT5_CMAKE_DIRS ${RV_DEPS_QT5_LOCATION}/lib/cmake/*)
FOREACH(
  QT5_CMAKE_DIR
  ${QT5_CMAKE_DIRS}
)
  IF(IS_DIRECTORY ${QT5_CMAKE_DIR})
    GET_FILENAME_COMPONENT(QT_COMPONENT_NAME ${QT5_CMAKE_DIR} NAME)
    SET(CMAKE_PREFIX_PATH
        ${CMAKE_PREFIX_PATH} ${QT5_CMAKE_DIR}
    )
    SET(${QT_COMPONENT_NAME}_DIR
        ${QT5_CMAKE_DIR}
        CACHE INTERNAL "Path to ${QT_COMPONENT_NAME} CMake"
    )
  ENDIF()
ENDFOREACH()

# Testing if everything is alright.
FIND_PACKAGE(
  Qt5
  COMPONENTS Core WebEngineCore Widgets
  REQUIRED
)

SET(_qt_copy_message
    "Copying Qt into ${RV_STAGE_ROOT_DIR}"
)
MESSAGE(STATUS "${_qt_copy_message}")

SET(RV_DEPS_QT5_VERSION
    ${Qt5Core_VERSION_STRING}
    CACHE STRING "Qt Version String"
)

# Common to both Mac and Linux platforms
IF(RV_TARGET_DARWIN
   OR RV_TARGET_LINUX
)
  FILE(
    GLOB _qt_plugins_dirs
    RELATIVE ${RV_DEPS_QT5_LOCATION}/plugins
    ${RV_DEPS_QT5_LOCATION}/plugins/*
  )
  FOREACH(
    _qt_plugin_dir
    ${_qt_plugins_dirs}
  )
    FILE(
      COPY ${RV_DEPS_QT5_LOCATION}/plugins/${_qt_plugin_dir}
      DESTINATION ${RV_STAGE_PLUGINS_QT_DIR}
    )
  ENDFOREACH()
ENDIF()

# Mac
IF(RV_TARGET_DARWIN)
  SET(_qt5_lib_dir
      ${RV_DEPS_QT5_LOCATION}/lib
  )
  FILE(
    GLOB libs_to_copy
    RELATIVE ${_qt5_lib_dir}
    ${_qt5_lib_dir}/*
  )
  FOREACH(
    lib_to_copy
    ${libs_to_copy}
  )
    IF(lib_to_copy MATCHES "framework(.dSYM)?")
      FILE(
        COPY ${_qt5_lib_dir}/${lib_to_copy}
        DESTINATION ${RV_STAGE_FRAMEWORKS_DIR}
      )
    ENDIF()
  ENDFOREACH()
ENDIF()

# Linux
IF(RV_TARGET_LINUX)
  SET(RV_DEPS_QT5_LIB_DIR
      ${RV_DEPS_QT5_LOCATION}/lib
  )
  FILE(
    GLOB libs_to_copy
    RELATIVE ${RV_DEPS_QT5_LIB_DIR}
    CONFIGURE_DEPENDS ${RV_DEPS_QT5_LIB_DIR}/*
  )
  FOREACH(
    lib_to_copy
    ${libs_to_copy}
  )
    FILE(
      COPY ${RV_DEPS_QT5_LIB_DIR}/${lib_to_copy}
      DESTINATION ${RV_STAGE_LIB_DIR}
    )
  ENDFOREACH()

  MESSAGE(STATUS "Copying Qt libexec files ...")
  FILE(
    COPY "${RV_DEPS_QT5_LOCATION}/libexec"
    DESTINATION "${RV_STAGE_ROOT_DIR}"
  )

  MESSAGE(STATUS "Copying Qt resources files ...")
  FILE(
    COPY "${RV_DEPS_QT5_RESOURCES_FOLDER}"
    DESTINATION "${RV_STAGE_ROOT_DIR}"
  )

  MESSAGE(STATUS "Copying Qt translations files ...")
  FILE(
    COPY "${RV_DEPS_QT5_TRANSLATIONS_FOLDER}"
    DESTINATION "${RV_STAGE_ROOT_DIR}"
  )
ENDIF()

# Windows

IF(RV_TARGET_WINDOWS)
  # Note: On windows, the Qt distribution has both the debug and release versions of its libs, dlls, and plugins into the same directories. This will prevent RV
  # from working correctly. We need to only copy the ones matching the CMAKE_BUILD_TYPE
  FUNCTION(COPY_ONLY_LIBS_MATCHING_BUILD_TYPE SRC_DIR DST_DIR)

    IF(NOT EXISTS (${DST_DIR}))
      FILE(MAKE_DIRECTORY ${DST_DIR})
    ENDIF()

    # First make a list of problematic libs: the ones ending up with d (which means they end with dd in debug)
    FILE(
      GLOB _qt_dd_libs
      RELATIVE ${SRC_DIR}
      ${SRC_DIR}/*dd.*
    )

    FILE(
      GLOB _qt_debug_libs
      RELATIVE ${SRC_DIR}
      ${SRC_DIR}/*d.*
    )

    # If we have dd libs then we need to remove the single d ones from the _qt_debug_libs
    FOREACH(
      _qt_dd_lib
      ${_qt_dd_libs}
    )
      # Determine the name of the associated single d lib so we can remove it from the _qt_debug_libs
      STRING(REPLACE "dd." "d." _d_lib ${_qt_dd_lib})
      LIST(REMOVE_ITEM _qt_debug_libs ${_d_lib})
    ENDFOREACH()

    # Remove Qt5 executables that are not needed.
    FILE(
      GLOB _qt_executables
      RELATIVE ${SRC_DIR}
      ${SRC_DIR}/*.exe
    )

    # Filtering. Some executables are needed for RV to work:
    # QtWebEngineProcess.exe
    FOREACH(
      _qt_executable 
      ${_qt_executables}
    )
      IF("${_qt_executable}" STREQUAL "QtWebEngineProcess.exe")
          LIST(REMOVE_ITEM _qt_executables "${_qt_executable}")
      ENDIF()
    ENDFOREACH()

    IF(CMAKE_BUILD_TYPE MATCHES "^Debug$")
      SET(_qt_libs_to_copy
          ${_qt_debug_libs}
      )
    ELSE()
      FILE(
        GLOB _qt_libs_to_copy
        RELATIVE ${SRC_DIR}
        ${SRC_DIR}/*
      )
      LIST(REMOVE_ITEM _qt_libs_to_copy ${_qt_debug_libs})
      LIST(REMOVE_ITEM _qt_libs_to_copy ${_qt_executables})
    ENDIF()

    FOREACH(
      _qt_lib
      ${_qt_libs_to_copy}
    )
      IF(NOT IS_DIRECTORY "${SRC_DIR}/${_qt_lib}")
        FILE(COPY_FILE ${SRC_DIR}/${_qt_lib} ${DST_DIR}/${_qt_lib} ONLY_IF_DIFFERENT)
      ENDIF()
    ENDFOREACH()
  ENDFUNCTION()

  # Copy the Qt plugins
  FILE(
    GLOB _qt_plugins_dirs
    RELATIVE ${RV_DEPS_QT5_LOCATION}/plugins
    ${RV_DEPS_QT5_LOCATION}/plugins/*
  )
  FOREACH(
    _qt_plugin_dir
    ${_qt_plugins_dirs}
  )
    COPY_ONLY_LIBS_MATCHING_BUILD_TYPE(${RV_DEPS_QT5_LOCATION}/plugins/${_qt_plugin_dir} ${RV_STAGE_PLUGINS_QT_DIR}/${_qt_plugin_dir})
  ENDFOREACH()

  # Copy the Qt import libs
  SET(RV_DEPS_QT5_LIB_DIR
      ${RV_DEPS_QT5_LOCATION}/lib
  )
  COPY_ONLY_LIBS_MATCHING_BUILD_TYPE(${RV_DEPS_QT5_LIB_DIR} ${RV_STAGE_LIB_DIR})

  # Copy the Qt dlls
  SET(RV_DEPS_QT5_BIN_DIR
      ${RV_DEPS_QT5_LOCATION}/bin
  )
  COPY_ONLY_LIBS_MATCHING_BUILD_TYPE(${RV_DEPS_QT5_BIN_DIR} ${RV_STAGE_BIN_DIR})

  MESSAGE(STATUS "Copying Qt translations files ...")
  FILE(
    COPY "${RV_DEPS_QT5_LOCATION}/translations"
    DESTINATION "${RV_STAGE_ROOT_DIR}"
  )

  MESSAGE(STATUS "Copying Qt resources files ...")
  FILE(
    COPY "${RV_DEPS_QT5_LOCATION}/resources"
    DESTINATION "${RV_STAGE_ROOT_DIR}"
  )
ENDIF()

MESSAGE(STATUS "${_qt_copy_message} -- DONE")
