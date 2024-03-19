#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)

INCLUDE(rv_lex)
INCLUDE(rv_yacc)
INCLUDE(rv_sed)

SET(_target
    "MuLang"
)
SET(_yy_prefix
    "MUYY"
)

#
# Mu Lexer
#
LEX_IT(
  YY_PREFIX
  ${_yy_prefix}
  INPUT_FILE
  ${CMAKE_CURRENT_SOURCE_DIR}/Lexer.l
  OUTPUT_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/${_target}/generated
  OUTPUT_FILE
  FlexLexer.cpp
  INPUT_SED_FILE
  ${CMAKE_CURRENT_SOURCE_DIR}/flexfilter.sed
)

#
# Mu Parser
#
# Tune bison: -t adds debug support, MUYY is the prefix, YYFILTER adds stack variable initialization to make MSV happy (its not a bug, the Windows runtime
# environment is incorrectly throwing on an initialized memory usage)
#
YACC_IT(
  YY_PREFIX
  ${_yy_prefix}
  INPUT_FILE
  ${CMAKE_CURRENT_SOURCE_DIR}/Grammar.y
  OUTPUT_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/${_target}/generated
  OUTPUT_FILE
  Grammar.cpp
  INPUT_SED_FILE
  ${CMAKE_CURRENT_SOURCE_DIR}/yaccfilter.sed
)

FILE(GLOB_RECURSE _sources *.c*)

ADD_LIBRARY(
  ${_target} STATIC
  ${_sources}
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC Imath::Imath
  PRIVATE Mu stl_ext
)

TARGET_COMPILE_OPTIONS(
  ${_target}
  PRIVATE -DARCH=${ARCH} "-DCOMPILER=${CMAKE_CXX_COMPILER_ID}" -DPLATFORM=${PLATFORM} -DMU_FLEX_MINOR_VERSION=${RV_FLEX_MINOR_VERSION}
          -DMU_FLEX_APPLE=${RV_FLEX_APPLE}
)

IF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PUBLIC pcre2-8 pcre2-posix
  )
ENDIF()

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
