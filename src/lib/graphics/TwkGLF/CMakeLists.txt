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

FIND_PACKAGE(
  ${RV_QT_PACKAGE_NAME}
  COMPONENTS Core Gui OpenGL Widgets
  REQUIRED
)

# Create shaders and add them to sources
QUOTE_FILES(_cooked_shaders INPUT_LIST ${_shaders} SUFFIX "glsl")
LIST(APPEND _sources ${_cooked_shaders})

# Embed stamp brush shaders from the OpenRV-annotation submodule. This ensures that we share shaders with all other apps using the same version.
SET(_ap_shader_dir
    "../../geometry/twkpaint-src/assets/shaders"
)
QUOTE_FILE(_ap_compat INPUT_FILENAME ${_ap_shader_dir}/compat_gl21.glsl CPP_SYMBOL ap_compat_gl21)
QUOTE_FILE(_ap_stamp_vert INPUT_FILENAME ${_ap_shader_dir}/stamp_vert.glsl CPP_SYMBOL ap_stamp_vert)
QUOTE_FILE(_ap_stamp_frag INPUT_FILENAME ${_ap_shader_dir}/stamp_frag.glsl CPP_SYMBOL ap_stamp_frag)
QUOTE_FILE(_ap_soft_stamp INPUT_FILENAME ${_ap_shader_dir}/soft_stamp_frag.glsl CPP_SYMBOL ap_soft_stamp_frag)
LIST(APPEND _sources ${_ap_compat} ${_ap_stamp_vert} ${_ap_stamp_frag} ${_ap_soft_stamp})

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

TARGET_LINK_LIBRARIES(
  ${_target}
  TwkApp
  TwkFB
  TwkGLText
  TwkUtil
  TwkAudio
  Qt::Core
  Qt::Gui
  Qt::Widgets
  Qt::OpenGL
)

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
