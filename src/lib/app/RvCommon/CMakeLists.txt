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
  Qt5
  COMPONENTS Core Gui OpenGL
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
    QTDesktopVideoDevice.cpp
    MuUICommands.cpp
    PyUICommands.cpp
    RvFileDialog.cpp
    RvConsoleWindow.cpp
    RvPreferences.cpp
    RvDocument.cpp
    RvConsoleApplication.cpp
    RvApplication.cpp
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

IF(RV_TARGET_DARWIN)
  LIST(APPEND _sources CGDesktopVideoDevice.cpp DisplayLink.cpp)
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

FOREACH(
  ui_source
  ${_ui_sources}
)
  GET_FILENAME_COMPONENT(outfile ${ui_source} NAME_WE)
  GET_FILENAME_COMPONENT(infile ${ui_source} ABSOLUTE)
  SET(outfile
      ${CMAKE_CURRENT_SOURCE_DIR}/${_target}/generated/ui_${outfile}.h
  )
  ADD_CUSTOM_COMMAND(
    OUTPUT ${outfile}
    COMMAND ${Qt5Widgets_UIC_EXECUTABLE} ARGS ${ui_options} -o ${outfile} ${infile}
    MAIN_DEPENDENCY ${infile}
    VERBATIM
  )
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
  ADD_CUSTOM_COMMAND(
    OUTPUT ${outfile}
    COMMAND ${Qt5Core_MOC_EXECUTABLE} -I ${CMAKE_CURRENT_SOURCE_DIR} -o ${outfile} ${infile}
    MAIN_DEPENDENCY ${infile}
    VERBATIM
  )
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

ADD_LIBRARY(
  ${_target} STATIC
  ${_sources} qrc/RvCommon.qrc
)

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
         Qt5::Core
         Qt5::Gui
         Qt5::Widgets
         Qt5::OpenGL
  PRIVATE Python::Python
          IPBaseNodes
          MuQt5
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
)

IF(RV_TARGET_DARWIN)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PUBLIC TwkGLFCoreGraphics
    PRIVATE "-framework CoreVideo"
  )
ENDIF()

IF(RV_TARGET_LINUX)
  FIND_PACKAGE(
    Qt5
    COMPONENTS X11Extras
    REQUIRED
  )
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE X11 Qt5::X11Extras
  )
ENDIF()

IF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE lcms
  )
ENDIF()

IF(RV_TARGET_LINUX OR RV_TARGET_WINDOWS)
  SET(_rv_icon_path_suffix_ "/../resources/RV.ico")
ELSEIF(RV_TARGET_DARWIN)
  SET(_rv_icon_path_suffix_ "/../Resources/RV.ico")
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
