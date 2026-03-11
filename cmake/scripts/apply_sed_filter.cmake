#
# Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# CMake script-mode file for applying sed-style substitutions.
# Replaces bash+sed and Python-based approaches with pure CMake.
#
# Usage:
#   cmake -DINPUT_FILE=<path> -DOUTPUT_FILE=<path> -DSED_FILE=<path> -P apply_sed_filter.cmake
#
# Supports:
#   - Literal s/PATTERN/REPLACEMENT/ substitutions
#   - PCRE negative lookbehind patterns (?<!PREFIX)MATCH via three-pass workaround
#   - Any single-character delimiter after 's'
#

IF(NOT INPUT_FILE)
  MESSAGE(FATAL_ERROR "INPUT_FILE not specified")
ENDIF()
IF(NOT OUTPUT_FILE)
  MESSAGE(FATAL_ERROR "OUTPUT_FILE not specified")
ENDIF()
IF(NOT SED_FILE)
  MESSAGE(FATAL_ERROR "SED_FILE not specified")
ENDIF()

FILE(READ "${INPUT_FILE}" _content)
FILE(READ "${SED_FILE}" _sed_content)

# Protect semicolons before splitting into CMake list (CMake uses ; as list separator)
STRING(REPLACE ";" "@@SEMICOLON@@" _sed_content "${_sed_content}")
STRING(REPLACE "\n" ";" _sed_lines "${_sed_content}")

SET(_placeholder_counter 0)

FOREACH(_line IN LISTS _sed_lines)
  # Restore semicolons within each line
  STRING(REPLACE "@@SEMICOLON@@" ";" _line "${_line}")
  STRING(STRIP "${_line}" _line)

  IF("${_line}" STREQUAL "" OR "${_line}" MATCHES "^#")
    CONTINUE()
  ENDIF()

  # Must start with 's'
  STRING(SUBSTRING "${_line}" 0 1 _prefix)
  IF(NOT "${_prefix}" STREQUAL "s")
    CONTINUE()
  ENDIF()

  # Detect delimiter (char after 's')
  STRING(SUBSTRING "${_line}" 1 1 _delim)
  STRING(SUBSTRING "${_line}" 2 -1 _rest)

  # Find delimiter separating pattern from replacement
  STRING(FIND "${_rest}" "${_delim}" _split_pos)
  IF(_split_pos EQUAL -1)
    CONTINUE()
  ENDIF()
  STRING(SUBSTRING "${_rest}" 0 ${_split_pos} _pattern)
  MATH(EXPR _repl_start "${_split_pos} + 1")
  STRING(SUBSTRING "${_rest}" ${_repl_start} -1 _repl_rest)

  # Find trailing delimiter
  STRING(FIND "${_repl_rest}" "${_delim}" _end_pos)
  IF(_end_pos GREATER -1)
    STRING(SUBSTRING "${_repl_rest}" 0 ${_end_pos} _replacement)
  ELSE()
    SET(_replacement "${_repl_rest}")
  ENDIF()

  # Check for PCRE negative lookbehind: (?<!PREFIX)MATCH
  STRING(FIND "${_pattern}" "(?<!" _lb_pos)
  IF(NOT _lb_pos EQUAL -1)
    # Extract the lookbehind prefix
    MATH(EXPR _prefix_start "${_lb_pos} + 4")
    STRING(SUBSTRING "${_pattern}" ${_prefix_start} -1 _after_lb)
    STRING(FIND "${_after_lb}" ")" _lb_end)
    STRING(SUBSTRING "${_after_lb}" 0 ${_lb_end} _lb_prefix)

    # Extract the bare match (after the closing paren)
    MATH(EXPR _bare_start "${_lb_end} + 1")
    STRING(SUBSTRING "${_after_lb}" ${_bare_start} -1 _bare_pattern)

    # Unescape BRE special chars for literal matching
    STRING(REPLACE "\\." "." _bare_pattern "${_bare_pattern}")
    STRING(REPLACE "\\*" "*" _bare_pattern "${_bare_pattern}")
    STRING(REPLACE "\\/" "/" _bare_pattern "${_bare_pattern}")

    # Unescape replacement too
    STRING(REPLACE "\\." "." _replacement "${_replacement}")
    STRING(REPLACE "\\*" "*" _replacement "${_replacement}")
    STRING(REPLACE "\\/" "/" _replacement "${_replacement}")

    # The already-qualified form is prefix + bare_pattern
    SET(_qualified "${_lb_prefix}${_bare_pattern}")

    # Three-pass workaround:
    SET(_placeholder "@@RV_SED_PLACEHOLDER_${_placeholder_counter}@@")
    MATH(EXPR _placeholder_counter "${_placeholder_counter} + 1")

    # Pass 1: Protect already-qualified occurrences
    STRING(REPLACE "${_qualified}" "${_placeholder}" _content "${_content}")
    # Pass 2: Qualify all remaining bare occurrences
    STRING(REPLACE "${_bare_pattern}" "${_replacement}" _content "${_content}")
    # Pass 3: Restore protected occurrences
    STRING(REPLACE "${_placeholder}" "${_qualified}" _content "${_content}")
  ELSE()
    # Literal replacement — unescape BRE special chars
    STRING(REPLACE "\\." "." _pattern "${_pattern}")
    STRING(REPLACE "\\*" "*" _pattern "${_pattern}")
    STRING(REPLACE "\\/" "/" _pattern "${_pattern}")

    STRING(REPLACE "${_pattern}" "${_replacement}" _content "${_content}")
  ENDIF()
ENDFOREACH()

FILE(WRITE "${OUTPUT_FILE}" "${_content}")
