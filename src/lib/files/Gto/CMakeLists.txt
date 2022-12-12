#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(cxx_defaults)
INCLUDE(rv_lex)
INCLUDE(rv_sed)
INCLUDE(rv_yacc)

#
# GTO main
#
SET(_target
    "Gto"
)

SET(_yy_prefix
    "GTO"
)

#
# GTO Lexer
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
# GTO Parser
#
YACC_IT(
  YY_PREFIX
  ${_yy_prefix}
  INPUT_FILE
  ${CMAKE_CURRENT_SOURCE_DIR}/Parser.y
  OUTPUT_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/${_target}/generated
  OUTPUT_FILE
  Parser.cpp
)

SET(_sources
    Writer.cpp Reader.cpp RawData.cpp Utilities.cpp
    # The following 2 are generated above
    ${CMAKE_CURRENT_SOURCE_DIR}/${_target}/generated/FlexLexer.cpp ${CMAKE_CURRENT_SOURCE_DIR}/${_target}/generated/Parser.cpp
)

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
  PRIVATE ZLIB::ZLIB Imath::Imath TwkUtil
)

TARGET_COMPILE_OPTIONS(
  ${_target}
  PRIVATE -DGTO_DECLARE_YYWRAP
          -DGTO_FLEX_APPLE=${RV_FLEX_APPLE}
          -DGTO_FLEX_MINOR_VERSION=${RV_FLEX_MINOR_VERSION}
          -DGTO_HAVE_FULL_IOSTREAMS
          -DGTO_SUPPORT_HALF
          -DGTO_SUPPORT_ZIP
          -DUPDATED_ZLIB_VERNUM=0x1270
)

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
