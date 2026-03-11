#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDE(rv_sed)

# Find the lexer tool: prefer win_flex on Windows, fall back to flex
IF(RV_TARGET_WINDOWS)
  FIND_PROGRAM(_lex win_flex NO_CACHE)
  IF(NOT _lex)
    FIND_PROGRAM(_lex flex NO_CACHE REQUIRED)
  ENDIF()
ELSE()
  FIND_PROGRAM(_lex flex NO_CACHE REQUIRED)
ENDIF()

# Get flex version using CMake string operations (no bash needed)
EXECUTE_PROCESS(
  COMMAND ${_lex} --version
  OUTPUT_VARIABLE _flex_version_output
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
# Extract minor version: output is like "flex 2.6.4" or "win_flex 2.6.4"
STRING(REGEX MATCH "[0-9]+\\.([0-9]+)" _flex_version_match "${_flex_version_output}")
SET(_flex_minor_version "${CMAKE_MATCH_1}")

# Detect Apple flex (only relevant on macOS)
IF(APPLE)
  STRING(FIND "${_flex_version_output}" "Apple" _apple_pos)
  IF(_apple_pos GREATER -1)
    SET(_flex_apple 1)
  ELSE()
    SET(_flex_apple 0)
  ENDIF()
ELSE()
  SET(_flex_apple 0)
ENDIF()

SET(RV_FLEX_MINOR_VERSION
    ${_flex_minor_version}
    CACHE STRING "The Flex/Lex tool minor version used for building RV."
)
SET(RV_FLEX_APPLE
    ${_flex_apple}
    CACHE BOOL "Wether or not we're building with an Apple-compiled version of Flex/Lex tool."
)

#
# 01234567890123456789012345678901234567890123456789012345678901234567890123456789 ! lex_it : wraps a call to Flex/Lex utility.
#
# The functon is checking specified parameters allowing clearer error messages from the CMake context. The function also allows an input '.sed' file to be
# specified so the lexer output can be patched.
#
# All of this is happening at configure time and should produce an output file that should typically be added to the module's source files.
#
# \param:PARAM1 PARAM1 specify the fooness of the function \param:PARAM2 PARAM2 should always be 42 \group:GROUP1 GROUP1 is a list of project to foo
# \group:GROUP2 This group represent optional project to pass to bar
#
FUNCTION(lex_it)

  SET(flags)
  SET(args)
  SET(listArgs
      YY_PREFIX INPUT_FILE OUTPUT_FILE INPUT_SED_FILE FLAGS OUTPUT_DIR
  )
  SET(_result
      ""
  )

  CMAKE_PARSE_ARGUMENTS(arg "${flags}" "${args}" "${listArgs}" ${ARGN})

  MESSAGE(DEBUG "******************************************************")
  MESSAGE(DEBUG "lex_it params:")
  MESSAGE(DEBUG "--- YY_PREFIX                : '${arg_YY_PREFIX}'")
  MESSAGE(DEBUG "--- FLAGS                    : '${arg_FLAGS}'")
  MESSAGE(DEBUG "--- INPUT_FILE               : '${arg_INPUT_FILE}'")
  MESSAGE(DEBUG "--- OUTPUT_FILE              : '${arg_OUTPUT_FILE}'")
  MESSAGE(DEBUG "--- INPUT_SED_FILE           : '${arg_INPUT_SED_FILE}'")
  MESSAGE(DEBUG "--- CMAKE_CURRENT_BINARY_DIR : '${CMAKE_CURRENT_BINARY_DIR}'")
  MESSAGE(DEBUG "--- UNPARSED_ARGUMENTS       : '${arg_UNPARSED_ARGUMENTS}'")
  MESSAGE(DEBUG "--- KEYWORDS_MISSING_VALUES  : '${arg_KEYWORDS_MISSING_VALUES}'")

  IF(NOT arg_YY_PREFIX)
    MESSAGE(FATAL ERROR "The 'YY_PREFIX' parameter was not specified.")
  ENDIF()

  IF(NOT arg_INPUT_FILE)
    MESSAGE(FATAL ERROR "The 'INPUT_FILE' parameter was not specified.")
  ENDIF()

  IF(NOT arg_OUTPUT_FILE)
    MESSAGE(FATAL ERROR "The 'OUTPUT_FILE' parameter was not specified.")
  ENDIF()

  GET_FILENAME_COMPONENT(input_file_name ${arg_INPUT_FILE} NAME)
  SET(input_timestamp_file
      ${CMAKE_CURRENT_BINARY_DIR}/${input_file_name}.timestamp
  )
  FILE(TIMESTAMP ${arg_INPUT_FILE} _input_file_timestamp)
  IF(EXISTS ${arg_OUTPUT_DIR}/${arg_OUTPUT_FILE}
     AND EXISTS ${input_timestamp_file}
  )
    FILE(READ ${input_timestamp_file} _saved_input_file_timestamp)
    IF(_input_file_timestamp STREQUAL _saved_input_file_timestamp)
      RETURN()
    ENDIF()
  ENDIF()
  FILE(
    WRITE ${input_timestamp_file}
    ${_input_file_timestamp}
  )

  IF(NOT arg_FLAGS)
    MESSAGE(STATUS "Lexer flags not specified, using RV defaults.")
    SET(arg_FLAGS
        --debug --prefix=${arg_YY_PREFIX}
    )
  ENDIF()

  IF(RV_VERBOSE_INVOCATION)
    LIST(APPEND arg_FLAGS "--verbose")
  ENDIF()

  MESSAGE(DEBUG "Lexer flags: ${arg_FLAGS}")
  MESSAGE(STATUS "Generating lexer source file --> '${arg_OUTPUT_FILE}'")

  FILE(MAKE_DIRECTORY ${arg_OUTPUT_DIR})

  #
  # Generate the lexer source source file
  EXECUTE_PROCESS(
    WORKING_DIRECTORY ${arg_OUTPUT_DIR}
    COMMAND ${_lex} ${arg_FLAGS} --outfile "${arg_OUTPUT_FILE}" "${arg_INPUT_FILE}" COMMAND_ECHO STDOUT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _result_code COMMAND_ERROR_IS_FATAL ANY
  )

  IF(arg_INPUT_SED_FILE)
    SED_IT(
      INPUT_SED_FILE
      ${arg_INPUT_SED_FILE}
      INPUT_FILE
      ${arg_OUTPUT_FILE}
      OUTPUT_DIR
      ${arg_OUTPUT_DIR}
      OUTPUT_FILE
      ${arg_OUTPUT_FILE}
    )
  ENDIF()
ENDFUNCTION()
