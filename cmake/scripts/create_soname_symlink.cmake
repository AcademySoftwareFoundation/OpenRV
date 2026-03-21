#
# Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# create_soname_symlink.cmake — Create a SONAME symlink for a staged shared library
#
# On macOS, a shared library's install name (LC_ID_DYLIB) may differ from its filename
# (e.g. libOpenColorIO.2.3.dylib vs libOpenColorIO.2.3.2.dylib). On Linux, the ELF SONAME
# may differ similarly (e.g. libOpenColorIO.so.2.3 vs libOpenColorIO.so.2.3.2). The linker
# records this name in dependent binaries, so the runtime loader needs a file matching it.
# This script creates a symlink from the recorded name to the actual file if they differ.
#
# Usage:
#   cmake -DLIB_FILE=/path/to/staged/libFoo.1.2.3.dylib -DRV_TARGET_DARWIN=TRUE -P create_soname_symlink.cmake
#   cmake -DLIB_FILE=/path/to/staged/libFoo.so.1.2.3    -DRV_TARGET_LINUX=TRUE  -P create_soname_symlink.cmake
#

IF(NOT DEFINED LIB_FILE)
  MESSAGE(FATAL_ERROR "LIB_FILE is required")
ENDIF()

IF(NOT EXISTS "${LIB_FILE}")
  MESSAGE(FATAL_ERROR "LIB_FILE does not exist: ${LIB_FILE}")
ENDIF()

# Extract the library's recorded name using platform-specific tooling.
SET(_soname_basename "")

IF(RV_TARGET_DARWIN)
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

  GET_FILENAME_COMPONENT(_soname_basename "${_install_name}" NAME)

ELSEIF(RV_TARGET_LINUX)
  EXECUTE_PROCESS(
    COMMAND readelf -d "${LIB_FILE}"
    OUTPUT_VARIABLE _readelf_out
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE _readelf_rc
  )

  IF(NOT _readelf_rc EQUAL 0)
    RETURN()
  ENDIF()

  # readelf -d output contains a line like:
  #   0x000000000000000e (SONAME)  Library soname: [libOpenColorIO.so.2.3]
  STRING(REGEX MATCH "\\(SONAME\\)[^\n]*\\[([^\n]+)\\]" _soname_match "${_readelf_out}")

  IF(NOT CMAKE_MATCH_1)
    RETURN()
  ENDIF()

  SET(_soname_basename "${CMAKE_MATCH_1}")

ELSE()
  # Unsupported platform — nothing to do.
  RETURN()
ENDIF()

GET_FILENAME_COMPONENT(_file_basename "${LIB_FILE}" NAME)
GET_FILENAME_COMPONENT(_file_dir "${LIB_FILE}" DIRECTORY)

IF(NOT "${_soname_basename}" STREQUAL "${_file_basename}")
  SET(_symlink_path "${_file_dir}/${_soname_basename}")
  IF(NOT EXISTS "${_symlink_path}")
    MESSAGE(STATUS "  Creating SONAME symlink: ${_soname_basename} -> ${_file_basename}")
    FILE(CREATE_LINK "${_file_basename}" "${_symlink_path}" SYMBOLIC)
  ENDIF()
ENDIF()
