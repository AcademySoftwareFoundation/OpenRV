#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

FIND_PROGRAM(_quote_file quoteFile HINT ${CMAKE_SOURCE_DIR}/src/build/quoteFile.py NO_CACHE REQUIRED)

FUNCTION(quote_files OUTPUT_LIST)

  SET(flags)
  SET(args)
  SET(listArgs
      INPUT_LIST SUFFIX FLAGS
  )
  SET(_cooked_list
      ""
  )

  CMAKE_PARSE_ARGUMENTS(arg "${flags}" "${args}" "${listArgs}" ${ARGN})

  MESSAGE(DEBUG "******************************************************")
  MESSAGE(DEBUG "quote_files params:")
  MESSAGE(DEBUG "--- FLAGS         : '${arg_FLAGS}'")
  MESSAGE(DEBUG "--- INPUT_LIST    : '${arg_INPUT_LIST}'")
  MESSAGE(DEBUG "--- SUFFIX        : '${arg_SUFFIX}'")
  MESSAGE(DEBUG "--- UNPARSED_ARGUMENTS      : '${arg_UNPARSED_ARGUMENTS}'")
  MESSAGE(DEBUG "--- KEYWORDS_MISSING_VALUES : '${arg_KEYWORDS_MISSING_VALUES}'")

  #
  # Rename all of the specified entries as  *_<suffix>.cpp
  FOREACH(
    _entry
    ${arg_INPUT_LIST}
  )
    CMAKE_PATH(GET _entry STEM _stem)
    SET(_src_name
        "${CMAKE_CURRENT_SOURCE_DIR}/${arg_SUFFIX}/${_entry}.${arg_SUFFIX}"
    )
    SET(_dest_name
        "${CMAKE_CURRENT_BINARY_DIR}/${_stem}_${arg_SUFFIX}.cpp"
    )

    GET_FILENAME_COMPONENT(input_file_name ${_src_name} NAME)
    SET(input_timestamp_file
        ${CMAKE_CURRENT_BINARY_DIR}/${input_file_name}.timestamp
    )
    FILE(TIMESTAMP ${_src_name} _input_file_timestamp)
    IF(EXISTS ${_dest_name}
       AND EXISTS ${input_timestamp_file}
    )
      FILE(READ ${input_timestamp_file} _saved_input_file_timestamp)
      IF(_input_file_timestamp STREQUAL _saved_input_file_timestamp)
        LIST(APPEND _cooked_list ${_dest_name})
        CONTINUE()
      ENDIF()
    ENDIF()
    FILE(
      WRITE ${input_timestamp_file}
      ${_input_file_timestamp}
    )

    MESSAGE(STATUS "Creating '${arg_SUFFIX}' file: '${_src_name}' --> '${_dest_name}'")
    EXECUTE_PROCESS(COMMAND python3 ${_quote_file} "${_src_name}" "${_dest_name}" "${_stem}_${arg_SUFFIX}" COMMAND_ERROR_IS_FATAL ANY)

    LIST(APPEND _cooked_list ${_dest_name})
  ENDFOREACH()

  # This returns the new list to the caller scope
  SET(${OUTPUT_LIST}
      ${_cooked_list}
      PARENT_SCOPE
  )
ENDFUNCTION()

# TODO: add documentation
FUNCTION(quote_file OUTPUT_FILENAME)

  SET(flags)
  SET(args)
  SET(listArgs
      INPUT_FILENAME CPP_SYMBOL FLAGS
  )
  SET(_cooked_list
      ""
  )

  CMAKE_PARSE_ARGUMENTS(arg "${flags}" "${args}" "${listArgs}" ${ARGN})

  MESSAGE(DEBUG "******************************************************")
  MESSAGE(DEBUG "quote_file params:")
  MESSAGE(DEBUG "--- FLAGS           : '${arg_FLAGS}'")
  MESSAGE(DEBUG "--- INPUT_FILENAME  : '${arg_INPUT_FILENAME}'")
  MESSAGE(DEBUG "--- OUTPUT_FILENAME : '${OUTPUT_FILENAME}'")
  MESSAGE(DEBUG "--- CPP_SYMBOL      : '${arg_CPP_SYMBOL}'")
  MESSAGE(DEBUG "--- UNPARSED_ARGUMENTS      : '${arg_UNPARSED_ARGUMENTS}'")
  MESSAGE(DEBUG "--- KEYWORDS_MISSING_VALUES : '${arg_KEYWORDS_MISSING_VALUES}'")

  CMAKE_PATH(GET arg_INPUT_FILENAME STEM _stem)
  SET(_src_name
      "${CMAKE_CURRENT_SOURCE_DIR}/${arg_INPUT_FILENAME}"
  )
  SET(_dest_name
      "${CMAKE_CURRENT_BINARY_DIR}/${_stem}.cpp"
  )

  GET_FILENAME_COMPONENT(input_file_name ${arg_INPUT_FILENAME} NAME)
  SET(input_timestamp_file
      ${CMAKE_CURRENT_BINARY_DIR}/${input_file_name}.timestamp
  )
  FILE(TIMESTAMP ${arg_INPUT_FILENAME} _input_file_timestamp)
  IF(EXISTS ${input_timestamp_file})
    FILE(READ ${input_timestamp_file} _saved_input_file_timestamp)
    IF(${_input_file_timestamp} EQUAL ${_saved_input_file_timestamp})
      SET(${OUTPUT_FILENAME}
          ${_dest_name}
          PARENT_SCOPE
      )
      RETURN()
    ENDIF()
  ENDIF()
  FILE(
    WRITE ${input_timestamp_file}
    ${_input_file_timestamp}
  )

  # This check is about creating file if it doesn't exists
  IF(NOT EXISTS ${_dest_name})
    MESSAGE(STATUS "Quoting '${_src_name}' as '${_dest_name}'")
    EXECUTE_PROCESS(
      COMMAND python3 ${_quote_file} "${_src_name}" "${_dest_name}" "${arg_CPP_SYMBOL}"
      RESULT_VARIABLE _result
    )
    IF(_result)
      MESSAGE(FATAL_ERROR "Couldn't create file from '${_src_name}'")
    ENDIF()
  ENDIF()

  # This check is about confirming the above did created a file
  IF(EXISTS ${_dest_name})
    # File exists ... check size
    FILE(SIZE ${_dest_name} _size)
    IF(_size LESS_EQUAL 50) # arbitrary, but we ain't expecting anything that small
      MESSAGE(FATAL_ERROR "Generated file '${_dest_name}' is smaller than expected, size:${_size}")
    ENDIF()

    # All checks are green returns the generated filename back to caller
    SET(${OUTPUT_FILENAME}
        ${_dest_name}
        PARENT_SCOPE
    )
  ELSE()
    MESSAGE(FATAL_ERROR "Couldn't find the created file '${_dest_name}'")
  ENDIF()
ENDFUNCTION()
