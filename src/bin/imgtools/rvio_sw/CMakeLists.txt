#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

SET(_target
    "rvio_sw"
)

LIST(APPEND _sources UICommands.cpp main.cpp utf8Main.cpp 
  #glfix.c
)

ADD_EXECUTABLE(
  ${_target}
  ${_sources}
)

FIND_PACKAGE(
  Qt5
  COMPONENTS Core Gui Widgets
  REQUIRED
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
)

# INCLUDE_DIRECTORIES(/usr/local/Cellar/mesa/22.3.6_1/include)
# LINK_DIRECTORIES(/usr/local/Cellar/mesa/22.3.6_1/lib)

# FILE(GLOB _mesa_libraries "/usr/local/Cellar/mesa/22.3.6_1/lib/*.dylib")
# message("LIBRARIES = ${_mesa_libraries}")

# TARGET_LINK_LIBRARIES(
#     ${_target} 
#     PRIVATE ${_mesa_libraries}
# )




# LIBS	    = RvApp TwkQtBase OCIONodes ICCNodes CTFNodes IPBaseNodes IPCore ShotgunMediaLibrary TwkMediaLibrary \
#               TwkApp IPMu MuTwkApp PyTwkApp TwkContainer TwkDeploy \
#               MovieRV MovieMuDraw MovieFB MovieProcedural \
#               MuImage MuEncoding MuMathLinear MuPy \
#               MuGLText MuGL MuGLU MuSystem MuIO MuAutoDoc MuLang Mu \
#               TwkPaint TwkGLText FTGL TwkGLFMesa TwkGLF LUT CDL \
#               IOproxy MovieProxy \
#               TwkMovie TwkAudio TwkFBAux TwkFB TwkMath TwkUtil TwkExc WFObj \
#               cv cxcore ZFile arg tiff jpeg png16 stl_ext \
#               OpenEXR Imath Iex IlmThread TwkLM resample Gto \
#               OpenColorIO yaml_cpp tinyxml \
#               freetype z exif lcms gc m boost_filesystem boost_system boost_regex boost_date_time boost_thread \
#               TwkQtUtil RvPackage yaml minizip

TARGET_LINK_LIBRARIES(
  ${_target}
  PRIVATE Mu
          MuLang
          MuTwkApp
          IOproxy
          MovieProxy
          OpenEXR::OpenEXR
          PyTwkApp
          RvApp
          IPCore
          IPBaseNodes
          MovieRV_FBO
          MovieFB
          MovieMuDraw
          MovieProcedural
          TwkAudio
          TwkMovie
          TwkCMS
          TwkMath
          TwkDeploy
          TwkExc
          TwkFB
          TwkUtil
          TwkQtBase
          arg
          stl_ext
          TwkMediaLibrary
          Qt5::Core
          TwkGLFFBO

        #   OCIONodes
        #   ICCNodes
)

IF(RV_TARGET_LINUX)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE Qt5::Gui Qt5::Widgets GLEW::GLEW BDWGC::Gc QTBundle
  )

TARGET_COMPILE_OPTIONS(
  ${_target}
  PRIVATE "-DRVIO_HW=1" "-DGIT_HEAD=\"${RV_GIT_COMMIT_SHORT_HASH}\"" "-DRELEASE_DESCRIPTION=\"${RV_RELEASE_DESCRIPTION}\""
)

RV_STAGE(TYPE "EXECUTABLE_WITH_PLUGINS" TARGET ${_target})
