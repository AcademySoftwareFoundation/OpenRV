#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)
INCLUDE(rv_quote_file)

SET(_target
    "RvCommon"
)

SET(CMAKE_AUTOUIC
    ON
)
SET(CMAKE_AUTOMOC
    ON
)
SET(CMAKE_AUTORCC
    ON
)

#
# This find_package statement is also required to find QT's build tools like 'uic' required just below. Keep this line before 'qt5_wrap_ui'
FIND_PACKAGE(
  ${RV_QT_PACKAGE_NAME}
  COMPONENTS Core Gui OpenGL Widgets ${QT6_QOPENGLWIDGET_COMPONENT}
  REQUIRED
)

SET(_sources
    QTGLVideoDevice.cpp
    QTTranslator.cpp
    InitGL.cpp
    QTUtils.cpp
    FileTypeTraits.cpp
    MediaFileTypes.cpp
    DesktopVideoDevice.cpp
    DesktopVideoModule.cpp
    MuUICommands.cpp
    PyUICommands.cpp
    RvFileDialog.cpp
    RvConsoleWindow.cpp
    RvPreferences.cpp
    RvDocument.cpp
    RvConsoleApplication.cpp
    RvApplication.cpp
    DiagnosticsView.cpp
    GLView.cpp
    TwkQTAction.cpp
    MediaDirModel.cpp
    RvNetworkDialog.cpp
    PermDelegate.cpp
    RvWebManager.cpp
    RvTopViewToolBar.cpp
    RvBottomViewToolBar.cpp
    RvSourceEditor.cpp
    GLSLSyntaxHighlighter.cpp
    RvProfileManager.cpp
    StreamConnection.cpp
    RvJavaScriptObject.cpp
)

# IF(RV_TARGET_DARWIN) LIST(APPEND _sources CGDesktopVideoDevice.cpp CGDesktopVideoDeviceArm.cpp DisplayLink.cpp) ENDIF()

IF(RV_TARGET_DARWIN)
  LIST(APPEND _sources DisplayLink.cpp)
ENDIF()

IF(RV_TARGET_DARWIN)
  QUOTE_FILE(_filename INPUT_FILENAME "rv_mac_dark.qss" CPP_SYMBOL "rv_mac_dark")
  LIST(APPEND _sources ${_filename})
ENDIF()

IF(RV_TARGET_LINUX
   OR RV_TARGET_WINDOWS
)
  QUOTE_FILE(_filename INPUT_FILENAME "rv_linux_dark.qss" CPP_SYMBOL "rv_linux_dark")
  LIST(APPEND _sources ${_filename})
ENDIF()

FILE(GLOB _ui_sources ui/*.ui)

IF(RV_VFX_PLATFORM STRGREATER_EQUAL CY2024)
  FILE(GLOB _ui_sources_extra ui/RvPreferences/Qt6/RvPreferences.ui)
ELSEIF(RV_VFX_PLATFORM STREQUAL CY2023)
  FILE(GLOB _ui_sources_extra ui/RvPreferences/Qt5/RvPreferences.ui)
ENDIF()

LIST(APPEND _ui_sources ${_ui_sources_extra})

IF(RV_VFX_PLATFORM STRGREATER_EQUAL CY2024)
  # Use Qt6 connections syntax because it generates the wrong QObject::connect for RvConsoleWindow.ui. TODO_QT: Could remove the connections from the UI XML and
  # add them to RvConsoleWindow.cpp
  SET(ui_options
      --connections string
  )
ENDIF()

FOREACH(
  ui_source
  ${_ui_sources}
)
  GET_FILENAME_COMPONENT(outfile ${ui_source} NAME_WE)
  GET_FILENAME_COMPONENT(infile ${ui_source} ABSOLUTE)
  SET(outfile
      ${CMAKE_CURRENT_SOURCE_DIR}/${_target}/generated/ui_${outfile}.h
  )

  IF(RV_VFX_PLATFORM STREQUAL CY2023)
    ADD_CUSTOM_COMMAND(
      OUTPUT ${outfile}
      COMMAND ${Qt5Widgets_UIC_EXECUTABLE} ARGS ${ui_options} -o ${outfile} ${infile}
      MAIN_DEPENDENCY ${infile}
      VERBATIM
    )
  ELSEIF(RV_VFX_PLATFORM STRGREATER_EQUAL CY2024)
    ADD_CUSTOM_COMMAND(
      OUTPUT ${outfile}
      COMMAND ${QT_UIC_EXECUTABLE} ARGS ${ui_options} -o ${outfile} ${infile}
      MAIN_DEPENDENCY ${infile}
      VERBATIM
    )
  ENDIF()

  SET_SOURCE_FILES_PROPERTIES(
    ${infile}
    PROPERTIES SKIP_AUTOMOC ON
               SKIP_AUTOUIC ON
               SKIP_AUTORCC ON
  )
  SET_SOURCE_FILES_PROPERTIES(
    ${outfile}
    PROPERTIES SKIP_AUTOMOC ON
               SKIP_AUTOUIC ON
               SKIP_AUTORCC ON
  )

  LIST(APPEND _sources ${outfile})
ENDFOREACH()

FILE(
  GLOB _files_to_moc
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  RvCommon/*.h
)
IF(NOT RV_TARGET_DARWIN)
  LIST(REMOVE_ITEM _files_to_moc "RvCommon/DisplayLink.h" "RvCommon/CGDesktopVideoDevice.h")
ENDIF()

FOREACH(
  file_to_moc
  ${_files_to_moc}
)
  GET_FILENAME_COMPONENT(outfile ${file_to_moc} NAME_WE)
  GET_FILENAME_COMPONENT(infile ${file_to_moc} ABSOLUTE)
  SET(outfile
      ${CMAKE_CURRENT_SOURCE_DIR}/${_target}/generated/moc_${outfile}.cpp
  )

  IF(RV_VFX_PLATFORM STREQUAL CY2023)
    ADD_CUSTOM_COMMAND(
      OUTPUT ${outfile}
      COMMAND ${Qt5Core_MOC_EXECUTABLE} -DRV_VFX_CY2023 -I ${CMAKE_CURRENT_SOURCE_DIR} -o ${outfile} ${infile}
      MAIN_DEPENDENCY ${infile}
      VERBATIM
    )
  ELSEIF(RV_VFX_PLATFORM STRGREATER_EQUAL CY2024)
    ADD_CUSTOM_COMMAND(
      OUTPUT ${outfile}
      COMMAND ${QT_MOC_EXECUTABLE} -DRV_VFX_${RV_VFX_PLATFORM} -I ${CMAKE_CURRENT_SOURCE_DIR} -o ${outfile} ${infile}
      MAIN_DEPENDENCY ${infile}
      VERBATIM
    )
  ENDIF()

  SET_SOURCE_FILES_PROPERTIES(
    ${infile}
    PROPERTIES SKIP_AUTOMOC ON
               SKIP_AUTOUIC ON
               SKIP_AUTORCC ON
  )
  SET_SOURCE_FILES_PROPERTIES(
    ${outfile}
    PROPERTIES SKIP_AUTOMOC ON
               SKIP_AUTOUIC ON
               SKIP_AUTORCC ON
  )

  LIST(APPEND _sources ${outfile})
ENDFOREACH()

#
# Generate about_rv.cpp with build information
#
SET(_about_rv_cpp
    ${CMAKE_CURRENT_BINARY_DIR}/about_rv.cpp
)

# Get compiler name
IF(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  SET(_compiler_name
      "GCC ${CMAKE_CXX_COMPILER_VERSION}"
  )
ELSEIF(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  IF(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    SET(_compiler_name
        "Clang-cl ${CMAKE_CXX_COMPILER_VERSION}"
    )
  ELSE()
    SET(_compiler_name
        "Clang ${CMAKE_CXX_COMPILER_VERSION}"
    )
  ENDIF()
ELSEIF(MSVC)
  SET(_compiler_name
      "MSVC ${CMAKE_CXX_COMPILER_VERSION}"
  )
ELSE()
  SET(_compiler_name
      "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}"
  )
ENDIF()

# Get CXX flags
STRING(REPLACE ";" " " _cxx_flags_str "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}}")

# Get VFX platform
IF(RV_VFX_PLATFORM)
  SET(_vfx_platform_str
      ${RV_VFX_PLATFORM}
  )
ELSE()
  SET(_vfx_platform_str
      "N/A"
  )
ENDIF()

# Get build root
GET_FILENAME_COMPONENT(_build_root ${CMAKE_BINARY_DIR} NAME)

# Get platform and arch
IF(RV_TARGET_DARWIN)
  SET(_platform_name
      "macOS"
  )
ELSEIF(RV_TARGET_LINUX)
  SET(_platform_name
      "Linux"
  )
ELSEIF(RV_TARGET_WINDOWS)
  SET(_platform_name
      "Windows"
  )
ELSE()
  SET(_platform_name
      ${CMAKE_SYSTEM_NAME}
  )
ENDIF()

SET(_arch_name
    ${CMAKE_SYSTEM_PROCESSOR}
)

# Gather dependency versions
IF(RV_VFX_PLATFORM STRGREATER_EQUAL CY2024)
  SET(_qt_version
      "${RV_DEPS_QT6_VERSION}"
  )
ELSE()
  SET(_qt_version
      "${RV_DEPS_QT5_VERSION}"
  )
ENDIF()

# Create a JSON-like string with versions (using semicolon as separator for CMake list)
SET(_versions_list
    "Qt:${_qt_version}"
    "CMake:${CMAKE_VERSION}"
    "Python:${RV_DEPS_PYTHON3_VERSION}"
    "PySide:${RV_DEPS_PYSIDE_VERSION}"
    "Boost:${RV_DEPS_BOOST_VERSION}"
    "Imath:${RV_DEPS_IMATH_VERSION}"
    "OpenEXR:${RV_DEPS_OPENEXR_VERSION}"
    "OpenImageIO:${RV_DEPS_OIIO_VERSION}"
    "OpenColorIO:${RV_DEPS_OCIO_VERSION}"
    "FFmpeg:${RV_DEPS_FFMPEG_VERSION}"
    "OpenSSL:${RV_DEPS_OPENSSL_VERSION}"
    "GLEW:${RV_DEPS_GLEW_VERSION}"
    "spdlog:${RV_DEPS_SPDLOG_VERSION}"
    "yaml-cpp:${RV_DEPS_YAML_CPP_VERSION}"
    "zlib:${RV_DEPS_ZLIB_VERSION}"
    "dav1d:${RV_DEPS_DAV1D_VERSION}"
    "jpegturbo:${RV_DEPS_JPEGTURBO_VERSION}"
    "png:${RV_DEPS_PNG_VERSION}"
    "tiff:${RV_DEPS_TIFF_VERSION}"
    "webp:${RV_DEPS_WEBP_VERSION}"
    "openjpeg:${RV_DEPS_OPENJPEG_VERSION}"
    "openjph:${RV_DEPS_OPENJPH_VERSION}"
    "raw:${RV_DEPS_RAW_VERSION}"
    "gc:${RV_DEPS_GC_VERSION}"
    "aja:${RV_DEPS_AJA_VERSION}"
    "bmd:${RV_DEPS_BMD_VERSION}"
    "imgui:${RV_DEPS_IMGUI_VERSION}"
    "numpy:${RV_DEPS_NUMPY_VERSION}"
)

# Add commercial RV-specific SDK versions if they exist
IF(DEFINED RV_DEPS_PRORES_VERSION
   AND RV_DEPS_PRORES_VERSION
)
  LIST(APPEND _versions_list "prores:${RV_DEPS_PRORES_VERSION}")
ENDIF()
IF(DEFINED RV_DEPS_R3DSDK_VERSION
   AND RV_DEPS_R3DSDK_VERSION
)
  LIST(APPEND _versions_list "r3dsdk:${RV_DEPS_R3DSDK_VERSION}")
ENDIF()
IF(DEFINED RV_DEPS_ARRIRAW_VERSION
   AND RV_DEPS_ARRIRAW_VERSION
)
  LIST(APPEND _versions_list "arriraw:${RV_DEPS_ARRIRAW_VERSION}")
ENDIF()
IF(DEFINED RV_DEPS_X264_VERSION
   AND RV_DEPS_X264_VERSION
)
  LIST(APPEND _versions_list "x264:${RV_DEPS_X264_VERSION}")
ENDIF()

STRING(REPLACE ";" "," _versions_str "${_versions_list}")

ADD_CUSTOM_COMMAND(
  OUTPUT ${_about_rv_cpp}
  COMMAND
    ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_about_rv.py ${_about_rv_cpp} "${_compiler_name}" "${CMAKE_BUILD_TYPE}" "${_cxx_flags_str}"
    "${_vfx_platform_str}" "${_build_root}" "${_platform_name}" "${_arch_name}" "${RV_UI_APPLICATION_NAME}" "${_versions_str}" "${RV_GIT_COMMIT_SHORT_HASH}"
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/generate_about_rv.py
  COMMENT "Generating about_rv.cpp with build information"
  VERBATIM
)

LIST(APPEND _sources ${_about_rv_cpp})

ADD_LIBRARY(
  ${_target} STATIC
  ${_sources} qrc/RvCommon.qrc
)

# Need that with Qt6 since the code for this target does not support unicode correctly.
IF(RV_TARGET_WINDOWS
   AND RV_VFX_PLATFORM STRGREATER_EQUAL CY2024
)
  QT_DISABLE_UNICODE_DEFINES(${_target})
ENDIF()

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC IPCore
         Mu
         MuLang
         MuTwkApp
         RvApp
         RvPackage
         TwkApp
         TwkGLF
         TwkMath
         PyMediaLibrary
         TwkQtChat
         TwkUtil
         Boost::thread
         Qt::Core
         Qt::Gui
         Qt::Widgets
         Qt::OpenGL
         ${QT6_QOPENGLWIDGET_TARGET}
  PRIVATE Python::Python
          IPBaseNodes
          ${RV_QT_MU_TARGET}
          MuPy
          PyTwkApp
          QTAudioRenderer
          TwkAudio
          TwkContainer
          TwkDeploy
          TwkMovie
          TwkQtBase
          TwkQtCoreUtil
          TwkPython
          arg
          stl_ext
          yaml
          minizip
          IOcin
          IOdpx
          IOexr
          IOjpeg
          IOtarga
          IOtiff
          spdlog::spdlog
          ImGuiPythonBridge
          imgui::imgui
)

IF(RV_TARGET_DARWIN)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PUBLIC TwkGLFCoreGraphics
    PRIVATE "-framework CoreVideo"
  )
ENDIF()

# https://doc.qt.io/qt-6/extras-changes-qt6.html
#
# The Qt X11 Extras module was removed in Qt 6.

# For QX11Info::connection(), use QNativeInterface::QX11Application::connection() instead. For QX11Info::display(), use
# QNativeInterface::QX11Application::display() instead. For QX11Info::isPlatformX11(), check QNativeInterface::QX11Application existence instead.
#
# Clients that still rely on the functionality in QX11Info class can include the private header <QtGui/private/qtx11extras_p.h> as a stopgap solution. To enable
# private headers use QT += gui-private with qmake, or add a project dependency to Qt::GuiPrivate with CMake.

IF(RV_TARGET_LINUX)
  IF(RV_VFX_PLATFORM STREQUAL CY2023)
    SET(_rvcommon_qt_components
        X11Extras
    )
    SET(_rvcommon_link_libraries
        Qt::X11Extras
    )
  ELSEIF(RV_VFX_PLATFORM STRGREATER_EQUAL CY2024)
    SET(_rvcommon_qt_components
        Gui
    )
    SET(_rvcommon_link_libraries
        Qt::GuiPrivate
    )
  ENDIF()

  FIND_PACKAGE(
    ${RV_QT_PACKAGE_NAME}
    COMPONENTS ${_rvcommon_qt_components}
    REQUIRED
  )
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE X11 ${_rvcommon_link_libraries}
  )
ENDIF()

IF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE lcms
  )
ENDIF()

IF(RV_TARGET_LINUX
   OR RV_TARGET_WINDOWS
)
  SET(_rv_icon_path_suffix_
      "/../resources/RV.ico"
  )
ELSEIF(RV_TARGET_DARWIN)
  SET(_rv_icon_path_suffix_
      "/../Resources/RV.ico"
  )
ENDIF()

SET(RV_ICON_PATH_SUFFIX
    ${_rv_icon_path_suffix_}
    CACHE STRING "RV's icon path suffix"
)

TARGET_COMPILE_OPTIONS(
  ${_target}
  PRIVATE -DARCH_STRING=\"${CMAKE_HOST_SYSTEM_PROCESSOR}\"
          -DBUILD_TYPE_STRING=\"${CMAKE_BUILD_TYPE}\"
          -DCOMPILER_STRING=\"${CMAKE_CXX_COMPILER_ID}\"
          -DCXXFLAGS_STRING=\"${CXXFLAGS}\"
          -DGIT_HEAD=\"${RV_GIT_COMMIT_SHORT_HASH}\"
          -DPLATFORM_STRING=\"${RV_TARGET_STRING}\"
          -DPLATFORM_VERSION=\"${CMAKE_SYSTEM_VERSION}\"
          -DRELEASE_DESCRIPTION=\"${RV_RELEASE_DESCRIPTION}\"
          -DINTERNAL_APPLICATION_NAME=\"${RV_INTERNAL_APPLICATION_NAME}\"
          -DINTERNAL_ORGANIZATION_NAME=\"${RV_INTERNAL_ORGANIZATION_NAME}\"
          -DINTERNAL_ORGANIZATION_DOMAIN=\"${RV_INTERNAL_ORGANIZATION_DOMAIN}\"
          -DUI_APPLICATION_NAME=\"${RV_UI_APPLICATION_NAME}\"
          -DCOPYRIGHT_TEXT=\"${RV_COPYRIGHT_TEXT}\"
          -DRV_ICON_PATH_SUFFIX=\"${RV_ICON_PATH_SUFFIX}\"
)

GET_PROPERTY(
  RV_LAZY_BUILD_THIRD_PARTY_CUSTOMIZATION GLOBAL
  PROPERTY "RV_LAZY_BUILD_THIRD_PARTY_CUSTOMIZATION"
)
IF(RV_LAZY_BUILD_THIRD_PARTY_CUSTOMIZATION)
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-DRV_LAZY_BUILD_THIRD_PARTY_CUSTOMIZATION=1"
  )
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE ${RV_LAZY_BUILD_THIRD_PARTY_CUSTOMIZATION}
  )
ENDIF()

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})

CONFIGURE_FILE(qrc/images/RV_icon.png ${RV_STAGE_RESOURCES_DIR}/RV.ico COPYONLY)
