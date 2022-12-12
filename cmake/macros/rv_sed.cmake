#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

FIND_PROGRAM(_sed sed NO_CACHE REQUIRED)

#
# 01234567890123456789012345678901234567890123456789012345678901234567890123456789 ! sed_it2 : wraps a call to the 'sed' utility.
#
# The functon is checking specified parameters allowing clearer error messages from the CMake context.
#
# Various RV supported systems have different version of the 'sed' utility having issues with the '-i' switch and/or usage of same input and output filename.
# This led to the convoluted scheme (below) having to 'sed' to a temporary output file then rename the temporary file to user-specified output filename.
# Although more complicated and more steps, it did seem to have the mre consistent behavior accross the supported platforms.
#
# All of this is happening at configure time and should produce and output file typically used as module's source files.
#
# \param:PARAM1 PARAM1 specify the fooness of the function \param:PARAM2 PARAM2 should always be 42 \group:GROUP1 GROUP1 is a list of project to foo
# \group:GROUP2 This group represent optional project to pass to bar
#
FUNCTION(sed_it)

  SET(flags)
  SET(args)
  SET(listArgs
      INPUT_FILE INPUT_SED_FILE OUTPUT_FILE FLAGS OUTPUT_DIR
  )

  CMAKE_PARSE_ARGUMENTS(arg "${flags}" "${args}" "${listArgs}" ${ARGN})

  MESSAGE(DEBUG "******************************************************")
  MESSAGE(DEBUG "sed_it params:")
  MESSAGE(DEBUG "--- FLAGS         : '${arg_FLAGS}'")
  MESSAGE(DEBUG "--- INPUT_FILE    : '${arg_INPUT_FILE}'")
  MESSAGE(DEBUG "--- INPUT_SED_FILE: '${arg_INPUT_SED_FILE}'")
  MESSAGE(DEBUG "--- OUTPUT_FILE   : '${arg_OUTPUT_FILE}'")
  MESSAGE(DEBUG "--- UNPARSED_ARGUMENTS      : '${arg_UNPARSED_ARGUMENTS}'")
  MESSAGE(DEBUG "--- KEYWORDS_MISSING_VALUES : '${arg_KEYWORDS_MISSING_VALUES}'")

  IF(NOT arg_INPUT_SED_FILE)
    MESSAGE(FATAL ERROR "The 'INPUT_SED_FILE' parameter was not specified.")
  ENDIF()

  IF(NOT arg_INPUT_FILE)
    MESSAGE(FATAL ERROR "The 'INPUT_FILE' parameter was not specified.")
  ENDIF()

  IF(NOT arg_OUTPUT_FILE)
    MESSAGE(FATAL ERROR "The 'OUTPUT_FILE' parameter was not specified.")
  ENDIF()

  # Adds a target with the given name that executes the given commands. The target has no output file and is always considered out of date even if the commands
  # try to create a file with the name of the target.
  CMAKE_PATH(GET arg_OUTPUT_FILE FILENAME _filename)
  CMAKE_PATH(GET arg_OUTPUT_FILE PARENT_PATH _path)
  MESSAGE(DEBUG "_filename: '${_filename}'")
  MESSAGE(DEBUG "_path: '${_path}'")

  GET_FILENAME_COMPONENT(input_file_name ${arg_INPUT_FILE} NAME)
  SET(input_timestamp_file
      ${CMAKE_CURRENT_BINARY_DIR}/${input_file_name}.timestamp
  )
  FILE(TIMESTAMP ${arg_INPUT_FILE} _input_file_timestamp)
  IF(EXISTS${arg_OUTPUT_DIR}/${arg_OUTPUT_FILE}
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

  FILE(MAKE_DIRECTORY ${arg_OUTPUT_DIR})

  SET(_temp
      "${arg_OUTPUT_DIR}/_temp_${_filename}"
  )

  MESSAGE(DEBUG "_temp: '${_temp}'")

  #
  # Actual SED operation
  EXECUTE_PROCESS(
    WORKING_DIRECTORY ${arg_OUTPUT_DIR}
    COMMAND ${_sed} -f ${arg_INPUT_SED_FILE} ${arg_INPUT_FILE} COMMAND_ECHO STDOUT
    OUTPUT_FILE ${_temp}
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _result_code COMMAND_ERROR_IS_FATAL ANY
  )

  # Finally, rename the temporary file to the user-specified output filename
  FILE(REMOVE ${arg_OUTPUT_DIR}/${arg_OUTPUT_FILE})
  FILE(RENAME ${_temp} ${arg_OUTPUT_DIR}/${arg_OUTPUT_FILE})

ENDFUNCTION()
