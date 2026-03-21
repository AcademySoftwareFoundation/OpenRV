#
# Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# create_soname_symlink.cmake — Create a SONAME symlink for a staged macOS dylib
#
# On macOS, a shared library's install name (LC_ID_DYLIB) may differ from its filename
# (e.g. libOpenColorIO.2.3.dylib vs libOpenColorIO.2.3.2.dylib). The linker records the
# install name in binaries that link against the library, so at runtime dyld looks for
# a file matching the install name. This script creates a symlink from the install name
# basename to the actual file if they differ.
#
# Usage: cmake -DLIB_FILE=/path/to/staged/libFoo.1.2.3.dylib -P create_soname_symlink.cmake
#

IF(NOT DEFINED LIB_FILE)
  MESSAGE(FATAL_ERROR "LIB_FILE is required")
ENDIF()

IF(NOT EXISTS "${LIB_FILE}")
  MESSAGE(FATAL_ERROR "LIB_FILE does not exist: ${LIB_FILE}")
ENDIF()

EXECUTE_PROCESS(
  COMMAND otool -D "${LIB_FILE}"
  OUTPUT_VARIABLE _otool_out
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
  RESULT_VARIABLE _otool_rc
)

IF(NOT _otool_rc EQUAL 0)
  RETURN()
ENDIF()

# otool -D output: first line is the file path, second line is the install name
STRING(REGEX MATCH "[^\n]+$" _install_name "${_otool_out}")

IF(NOT _install_name)
  RETURN()
ENDIF()

GET_FILENAME_COMPONENT(_install_basename "${_install_name}" NAME)
GET_FILENAME_COMPONENT(_file_basename "${LIB_FILE}" NAME)
GET_FILENAME_COMPONENT(_file_dir "${LIB_FILE}" DIRECTORY)

IF(NOT "${_install_basename}" STREQUAL "${_file_basename}")
  SET(_symlink_path "${_file_dir}/${_install_basename}")
  IF(NOT EXISTS "${_symlink_path}")
    MESSAGE(STATUS "  Creating SONAME symlink: ${_install_basename} -> ${_file_basename}")
    FILE(CREATE_LINK "${_file_basename}" "${_symlink_path}" SYMBOLIC)
  ENDIF()
ENDIF()
