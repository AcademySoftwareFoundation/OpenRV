#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

FUNCTION(yacc_it)

  # Find the parser generator: prefer win_bison on Windows, fall back to bison
  IF(RV_TARGET_WINDOWS)
    FIND_PROGRAM(_yacc win_bison NO_CACHE)
    IF(NOT _yacc)
      FIND_PROGRAM(_yacc bison NO_CACHE REQUIRED)
    ENDIF()
  ELSE()
    FIND_PROGRAM(_yacc bison NO_CACHE REQUIRED)
  ENDIF()

  SET(flags)
  SET(args)
  SET(listArgs
      YY_PREFIX INPUT_FILE OUTPUT_FILE INPUT_SED_FILE FLAGS OUTPUT_DIR
  )

  CMAKE_PARSE_ARGUMENTS(arg "${flags}" "${args}" "${listArgs}" ${ARGN})

  MESSAGE(DEBUG "******************************************************")
  MESSAGE(DEBUG "yacc_it params:")
  MESSAGE(DEBUG "--- YY_PREFIX               : '${arg_YY_PREFIX}'")
  MESSAGE(DEBUG "--- FLAGS                   : '${arg_FLAGS}'")
  MESSAGE(DEBUG "--- INPUT_FILE              : '${arg_INPUT_FILE}'")
  MESSAGE(DEBUG "--- OUTPUT_FILE             : '${arg_OUTPUT_FILE}'")
  MESSAGE(DEBUG "--- INPUT_SED_FILE          : '${arg_INPUT_SED_FILE}'")
  MESSAGE(DEBUG "--- UNPARSED_ARGUMENTS      : '${arg_UNPARSED_ARGUMENTS}'")
  MESSAGE(DEBUG "--- KEYWORDS_MISSING_VALUES : '${arg_KEYWORDS_MISSING_VALUES}'")

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
    MESSAGE(STATUS "Parser generator flags not specified, using RV defaults.")
    # GNU bison generates parsers for LALR(1) grammars.
    #
    # -d, --defines              also produce a header file -t, --debug                instrument the parser for debugging -v, --verbose              same as
    # `--report=state' -p, --name-prefix=PREFIX   prepend PREFIX to the external symbols -y, --yacc                 emulate POSIX yacc
    #
    # set(arg_FLAGS --yacc --debug --defines --name-prefix=${arg_YY_PREFIX})
    SET(arg_FLAGS
        --debug --defines --name-prefix=${arg_YY_PREFIX}
    )
  ENDIF()
  IF(RV_VERBOSE_INVOCATION)
    LIST(APPEND arg_FLAGS "--verbose")
  ENDIF()
  MESSAGE(DEBUG "Parser flags: ${arg_FLAGS}")

  #
  # After parser generation, we need to copy the generated .hpp to .h Using the '-defines' switch, the bison tool generate a .hpp with same base name as the
  # input file. We're expecting a .h in the code base, we therefore need to copy the generated include file. Note that on Windows, the parser generated .cpp
  # file will include the .hpp file so we cannot simply rename it
  CMAKE_PATH(GET arg_INPUT_FILE STEM _stem_name)
  SET(_yacc_generated_include_filename
      "${_stem_name}.hpp"
  )
  SET(_renamed_include_filename
      "${_stem_name}.h"
  )
  MESSAGE(DEBUG "_renamed_include_filename: '${_renamed_include_filename}'")

  FILE(MAKE_DIRECTORY ${arg_OUTPUT_DIR})

  #
  # Generate parser source source file
  EXECUTE_PROCESS(
    WORKING_DIRECTORY ${arg_OUTPUT_DIR}
    COMMAND ${_yacc} ${arg_FLAGS} --output=${arg_OUTPUT_FILE} ${arg_INPUT_FILE} COMMAND_ECHO STDOUT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _result_code COMMAND_ERROR_IS_FATAL ANY
  )

  #
  # Copy the generated .hpp to a .h file
  EXECUTE_PROCESS(
    WORKING_DIRECTORY ${arg_OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${arg_OUTPUT_DIR}/${_yacc_generated_include_filename} ${arg_OUTPUT_DIR}/${_renamed_include_filename} COMMAND_ECHO STDOUT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _result_code
  )
  MESSAGE(DEBUG "copy result code: '${_result_code}'")
  IF(NOT ${_result_code} EQUAL 0)
    MESSAGE(FATAL_ERROR "copy result code '${_result_code}'")
  ENDIF()

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
