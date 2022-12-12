#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)
INCLUDE(rv_quote_file)

SET(_target
    "TwkGLF"
)

SET(_sources
    GLVideoDevice.cpp
    GL.cpp
    GLFBO.cpp
    GLVBO.cpp
    GLFence.cpp
    GLPixelBufferObject.cpp
    GLPixelBufferObjectPool.cpp
    GLSyncObject.cpp
    GLProgram.cpp
    BasicGLProgram.cpp
    GLPipeline.cpp
    GLState.cpp
    GLRenderPrimitives.cpp
)

SET(_shaders
    CloneFrag
    EraseFrag
    ReplaceVertex
    TexRectFrag
    CloneVertex
    EraseVertex
    ScaleFrag
    SoftScaleFrag
    TextureFrag
    DefaultFrag
    OldReplaceFrag
    ScaleVertex
    TextureVertex
    DefaultVertex
    SoftDirectionPaintFrag
    DirectionPaintFrag
    SoftOldReplaceFrag
    DirectionPaintVertex
    ReplaceFrag
    SoftReplaceFrag
    SoftCloneFrag
    SoftEraseFrag
    StereoScanlineFrag
    StereoCheckerFrag
    CrosshatchBGFrag
    CheckerboardBGFrag
    ReplaceColoredVertex
    PaintColoredFrag
)

FIND_PACKAGE(OpenGL REQUIRED)

# Create shaders and add them to sources
QUOTE_FILES(_cooked_shaders INPUT_LIST ${_shaders} SUFFIX "glsl")
LIST(APPEND _sources ${_cooked_shaders})

ADD_LIBRARY(
  ${_target} STATIC
  ${_sources}
)

IF(RV_TARGET_LINUX)
  TARGET_LINK_LIBRARIES(
    ${_target}
    # OpenGL:GL vs. GL Usage of GL does find the 'glGenFencesNV' whereas 'OpenGL:GL' does not :-(
    GL GLEW::GLEW
  )
ENDIF()

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

TARGET_LINK_LIBRARIES(${_target} TwkApp TwkFB TwkGLText TwkUtil TwkAudio)

IF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(${_target} glew)
ENDIF()

IF(RV_TARGET_LINUX)
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-DVM_NO_GL"
  )
ENDIF()

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
