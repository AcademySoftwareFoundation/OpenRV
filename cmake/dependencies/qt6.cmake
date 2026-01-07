#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

SET(RV_DEPS_QT_LOCATION
    ""
    CACHE STRING "Path to pre-compiled Qt6"
)
SET(_target
    RV_DEPS_QT
)
IF(NOT RV_DEPS_QT_LOCATION
   OR RV_DEPS_QT_LOCATION STREQUAL ""
)
  MESSAGE(
    FATAL_ERROR
      "Unable to build without a RV_DEPS_QT_LOCATION. It is required to provide a working Qt 5.15 root path to build. Example: cmake .. -DRV_DEPS_QT_LOCATION=/Users/rv/Qt/5.15.11/clang_64"
  )
ENDIF()

# TODO : I think that the resources folder changed location
SET(RV_DEPS_QT_RESOURCES_FOLDER
    "${RV_DEPS_QT_LOCATION}/resources"
    CACHE STRING "Path to the Qt resources files folder"
)

# TODO : I think that the translations folder changed location
SET(RV_DEPS_QT_TRANSLATIONS_FOLDER
    "${RV_DEPS_QT_LOCATION}/translations"
    CACHE STRING "Path to the Qt translations files folder"
)

FILE(GLOB QT6_CMAKE_DIRS ${RV_DEPS_QT_LOCATION}/lib/cmake/*)
FOREACH(
  QT6_CMAKE_DIR
  ${QT6_CMAKE_DIRS}
)
  IF(IS_DIRECTORY ${QT6_CMAKE_DIR})
    GET_FILENAME_COMPONENT(QT_COMPONENT_NAME ${QT6_CMAKE_DIR} NAME)
    SET(CMAKE_PREFIX_PATH
        ${CMAKE_PREFIX_PATH} ${QT6_CMAKE_DIR}
    )
    SET(${QT_COMPONENT_NAME}_DIR
        ${QT6_CMAKE_DIR}
        CACHE INTERNAL "Path to ${QT_COMPONENT_NAME} CMake"
    )
  ENDIF()
ENDFOREACH()

# For newer versions of Qt on macOS, QtWebEngine may not be part of the base installation. This logic checks for its existence and, if missing for specific Qt
# versions, downloads and extracts it.
STRING(REGEX MATCH "([0-9]+\.[0-9]+\.[0-9]+)" _qt_version_from_path "${RV_DEPS_QT_LOCATION}")

# If we are on macOS, with Qt 6.8.3, and WebEngine is missing, download it.
IF(_qt_version_from_path VERSION_EQUAL "6.8.3"
   AND NOT EXISTS "${RV_DEPS_QT_LOCATION}/lib/cmake/Qt6WebEngineCore/Qt6WebEngineCoreConfig.cmake"
)
  MESSAGE(STATUS "QtWebEngine component for 6.8.3 not found. Attempting to download and install it.")

  # Find the 7z executable, which is required for extraction.
  FIND_PROGRAM(
    SEVEN_ZIP_EXECUTABLE
    NAMES 7z p7zip
  )
  IF(NOT SEVEN_ZIP_EXECUTABLE)
    MESSAGE(FATAL_ERROR "p7zip (or 7z) is required to extract the QtWebEngine module but was not found. Please install it (e.g., 'brew install p7zip').")
  ENDIF()

  IF(RV_TARGET_DARWIN)
    # Download the 7z archive provided by the user.
    SET(QT_WEBENGINE_URL
        "https://download.qt.io/online/qtsdkrepository/mac_x64/extensions/qtwebengine/683/clang_64/extensions.qtwebengine.683.clang_64/6.8.3-0-202503201424qtwebengine-MacOS-MacOS_14-Clang-MacOS-MacOS_14-X86_64-ARM64.7z"
    )
    SET(QT_WEBENGINE_ARCHIVE
        "${CMAKE_BINARY_DIR}/qtwebengine-6.8.3-macos.7z"
    )
  ELSEIF(RV_TARGET_LINUX)
    SET(QT_WEBENGINE_URL
        "https://download.qt.io/online/qtsdkrepository/linux_x64/extensions/qtwebengine/683/x86_64/extensions.qtwebengine.683.linux_gcc_64/6.8.3-0-202503201424qtwebengine-Linux-RHEL_8_10-GCC-Linux-RHEL_8_10-X86_64.7z"
    )
    SET(QT_WEBENGINE_ARCHIVE
        "${CMAKE_BINARY_DIR}/qtwebengine-6.8.3-linux.7z"
    )
  ELSEIF(RV_TARGET_WINDOWS)
    SET(QT_WEBENGINE_URL
        "https://download.qt.io/online/qtsdkrepository/windows_x86/extensions/qtwebengine/683/msvc2022_64/extensions.qtwebengine.683.win64_msvc2022_64/6.8.3-0-202503201424qtwebengine-Windows-Windows_11_23H2-MSVC2022-Windows-Windows_11_23H2-X86_64.7z"
    )
    SET(QT_WEBENGINE_ARCHIVE
        "${CMAKE_BINARY_DIR}/qtwebengine-6.8.3-windows.7z"
    )
  ELSE()
    MESSAGE(
      FATAL_ERROR
        "Failed to determine platform for downloading QtWebEngine. Modify qt6.cmake to add support for this platform.  Installers can be found at https://download.qt.io/online/qtsdkrepository/ under platform/extensions/qtwebengine/683/"
    )
  ENDIF()

  MESSAGE(STATUS "Downloading QtWebEngine from ${QT_WEBENGINE_URL}")
  FILE(
    DOWNLOAD ${QT_WEBENGINE_URL} ${QT_WEBENGINE_ARCHIVE}
    SHOW_PROGRESS
  )

  # Extract the archive into a temporary directory to handle its internal structure safely.
  SET(QT_WEBENGINE_TEMP_DIR
      "${CMAKE_BINARY_DIR}/qtwebengine_temp_extract"
  )
  IF(EXISTS "${QT_WEBENGINE_TEMP_DIR}")
    FILE(REMOVE_RECURSE "${QT_WEBENGINE_TEMP_DIR}")
  ENDIF()
  FILE(MAKE_DIRECTORY "${QT_WEBENGINE_TEMP_DIR}")

  MESSAGE(STATUS "Extracting QtWebEngine to temporary directory: ${QT_WEBENGINE_TEMP_DIR}")
  EXECUTE_PROCESS(
    COMMAND ${SEVEN_ZIP_EXECUTABLE} x ${QT_WEBENGINE_ARCHIVE} -o${QT_WEBENGINE_TEMP_DIR}
    RESULT_VARIABLE extract_result
    OUTPUT_QUIET ERROR_QUIET
  )

  IF(NOT extract_result EQUAL 0)
    MESSAGE(FATAL_ERROR "Failed to extract QtWebEngine archive to temp directory. Result: ${extract_result}.")
  ENDIF()

  # The archive contains Qt component directories (lib, qml, etc.) directly. Copy the entire contents of the temporary directory into the Qt installation,
  # merging the folders.
  MESSAGE(STATUS "Copying extracted files from ${QT_WEBENGINE_TEMP_DIR} to ${RV_DEPS_QT_LOCATION}")
  FILE(
    COPY "${QT_WEBENGINE_TEMP_DIR}/"
    DESTINATION "${RV_DEPS_QT_LOCATION}"
  )

  # Clean up the temporary directory and downloaded archive
  FILE(REMOVE_RECURSE "${QT_WEBENGINE_TEMP_DIR}")
  FILE(REMOVE "${QT_WEBENGINE_ARCHIVE}")

  MESSAGE(STATUS "QtWebEngine for 6.8.3 installed successfully.")
ENDIF()

# Testing if everything is alright. In Qt6, QtWebEngine has been split into Qt6WebEngineCore and Qt6WebEngineWidgets.
FIND_PACKAGE(
  Qt6
  COMPONENTS Core WebEngineCore WebEngineWidgets
  REQUIRED
)

GET_TARGET_PROPERTY(MOC_EXECUTABLE Qt6::moc IMPORTED_LOCATION)
GET_TARGET_PROPERTY(UIC_EXECUTABLE Qt6::uic IMPORTED_LOCATION)

SET(QT_MOC_EXECUTABLE
    "${MOC_EXECUTABLE}"
    CACHE STRING "Qt MOC executable"
)
SET(QT_UIC_EXECUTABLE
    "${UIC_EXECUTABLE}"
    CACHE STRING "Qt UIC executable"
)

SET(_qt_copy_message
    "Copying Qt into ${RV_STAGE_ROOT_DIR}"
)
MESSAGE(STATUS "${_qt_copy_message}")

SET(RV_DEPS_QT_VERSION
    ${Qt6Core_VERSION}
    CACHE STRING "Qt Version String"
)

# Common to both Mac and Linux platforms
IF(RV_TARGET_DARWIN
   OR RV_TARGET_LINUX
)
  FILE(
    GLOB _qt_plugins_dirs
    RELATIVE ${RV_DEPS_QT_LOCATION}/plugins
    ${RV_DEPS_QT_LOCATION}/plugins/*
  )
  FOREACH(
    _qt_plugin_dir
    ${_qt_plugins_dirs}
  )
    FILE(
      COPY ${RV_DEPS_QT_LOCATION}/plugins/${_qt_plugin_dir}
      DESTINATION ${RV_STAGE_PLUGINS_QT_DIR}
    )
  ENDFOREACH()
ENDIF()

# Mac
IF(RV_TARGET_DARWIN)
  SET(_qt5_lib_dir
      ${RV_DEPS_QT_LOCATION}/lib
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
    IF(lib_to_copy MATCHES "framework"
       AND NOT
           (CMAKE_BUILD_TYPE STREQUAL "Release"
            AND lib_to_copy MATCHES "\\.dSYM$")
    )
      FILE(
        COPY ${_qt5_lib_dir}/${lib_to_copy}
        DESTINATION ${RV_STAGE_FRAMEWORKS_DIR}
      )
    ENDIF()
  ENDFOREACH()
ENDIF()

# Linux
IF(RV_TARGET_LINUX)
  SET(RV_DEPS_QT_LIB_DIR
      ${RV_DEPS_QT_LOCATION}/lib
  )
  FILE(
    GLOB libs_to_copy
    RELATIVE ${RV_DEPS_QT_LIB_DIR}
    CONFIGURE_DEPENDS ${RV_DEPS_QT_LIB_DIR}/*
  )
  FOREACH(
    lib_to_copy
    ${libs_to_copy}
  )
    FILE(
      COPY ${RV_DEPS_QT_LIB_DIR}/${lib_to_copy}
      DESTINATION ${RV_STAGE_LIB_DIR}
    )
  ENDFOREACH()

  MESSAGE(STATUS "Copying Qt libexec files ...")
  FILE(
    COPY "${RV_DEPS_QT_LOCATION}/libexec"
    DESTINATION "${RV_STAGE_ROOT_DIR}"
  )

  MESSAGE(STATUS "Copying Qt resources files ...")
  FILE(
    COPY "${RV_DEPS_QT_RESOURCES_FOLDER}"
    DESTINATION "${RV_STAGE_ROOT_DIR}"
  )

  MESSAGE(STATUS "Copying Qt translations files ...")
  FILE(
    COPY "${RV_DEPS_QT_TRANSLATIONS_FOLDER}"
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

    # Remove Qt6 executables that are not needed.
    FILE(
      GLOB _qt_executables
      RELATIVE ${SRC_DIR}
      ${SRC_DIR}/*.exe
    )

    # Filtering. Some executables are needed for RV to work: QtWebEngineProcess.exe
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
    RELATIVE ${RV_DEPS_QT_LOCATION}/plugins
    ${RV_DEPS_QT_LOCATION}/plugins/*
  )
  FOREACH(
    _qt_plugin_dir
    ${_qt_plugins_dirs}
  )
    COPY_ONLY_LIBS_MATCHING_BUILD_TYPE(${RV_DEPS_QT_LOCATION}/plugins/${_qt_plugin_dir} ${RV_STAGE_PLUGINS_QT_DIR}/${_qt_plugin_dir})
  ENDFOREACH()

  # Copy the Qt import libs
  SET(RV_DEPS_QT_LIB_DIR
      ${RV_DEPS_QT_LOCATION}/lib
  )
  COPY_ONLY_LIBS_MATCHING_BUILD_TYPE(${RV_DEPS_QT_LIB_DIR} ${RV_STAGE_LIB_DIR})

  # Copy the Qt dlls
  SET(RV_DEPS_QT_BIN_DIR
      ${RV_DEPS_QT_LOCATION}/bin
  )
  COPY_ONLY_LIBS_MATCHING_BUILD_TYPE(${RV_DEPS_QT_BIN_DIR} ${RV_STAGE_BIN_DIR})

  MESSAGE(STATUS "Copying Qt translations files ...")
  FILE(
    COPY "${RV_DEPS_QT_LOCATION}/translations"
    DESTINATION "${RV_STAGE_ROOT_DIR}"
  )

  MESSAGE(STATUS "Copying Qt resources files ...")
  FILE(
    COPY "${RV_DEPS_QT_LOCATION}/resources"
    DESTINATION "${RV_STAGE_ROOT_DIR}"
  )
ENDIF()

# Qt5: OpenGLWidgets is in component Widgets Qt6: OpenGLWidgets is in component OpenGLWidgets
SET(QT6_QOPENGLWIDGET_COMPONENT
    "OpenGLWidgets"
    CACHE STRING "Qt QOpenGLWidget component name"
)
SET(QT6_QOPENGLWIDGET_TARGET
    "Qt6::OpenGLWidgets"
    CACHE STRING "Qt QOpenGLWidget target name"
)

MESSAGE(STATUS "${_qt_copy_message} -- DONE")
