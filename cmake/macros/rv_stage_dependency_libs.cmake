#
# Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# RV_STAGE_DEPENDENCY_LIBS - Copy dependency libraries/binaries to staging area
#
# Provides a single, consistent staging mechanism for all dependencies, replacing the ad-hoc staging patterns used across dependency files.
#
# Always uses OUTPUT-based ADD_CUSTOM_COMMAND for proper incremental build tracking (never POST_BUILD which always re-runs).
# cmake-format: off
# Usage:
#   RV_STAGE_DEPENDENCY_LIBS( TARGET <target-name>          # REQUIRED: e.g., RV_DEPS_BOOST
#     [LIB_DIR <source-lib-dir>]    # Source lib directory to copy (default: ${_lib_dir} from caller scope; required unless FILES is used)
#     OUTPUTS <file1> [file2...]    # staged files for dependency tracking (or use USE_FLAG_FILE)
#     [BIN_DIR <source-bin-dir>]    # Windows DLLs source (auto-copied on Windows)
#     [INCLUDE_DIR <src-inc-dir>]   # If headers need staging too
#     [STAGE_LIB_DIR <override>]    # Override RV_STAGE_LIB_DIR (e.g., for OpenSSL/Linux)
#     [EXTRA_LIB_DIRS <dir1>...]    # Additional lib dirs to copy to
#     [FILES <file1> [file2...]]    # Individual files to copy_if_different to STAGE_LIB_DIR (alternative to LIB_DIR)
#     [DEPENDS <dep1> [dep2...]]    # Dependencies (default: ${TARGET})
#     [PRE_COMMANDS COMMAND <cmd1> [COMMAND <cmd2>...]]  # Commands to run before copy; each must be prefixed with COMMAND keyword
#     [LIBNAME <filename>]          # Platform-aware shorthand: on Windows uses BIN_DIR+STAGE_BIN_DIR, otherwise STAGE_LIB_DIR
#     [USE_FLAG_FILE]               # Use touch-based flag instead of OUTPUTS for tracking.
#                                   # Caveat: won't re-stage if staged files are deleted, since the flag survives.
#                                   # Prefer OUTPUTS when possible; use this only when outputs are hard to enumerate.
# cmake-format: on
FUNCTION(RV_STAGE_DEPENDENCY_LIBS)
  IF(RV_USE_SYSTEM_DEPS)
    RETURN()
  ENDIF()

  CMAKE_PARSE_ARGUMENTS(
    _ARG "USE_FLAG_FILE" "TARGET;LIB_DIR;BIN_DIR;INCLUDE_DIR;STAGE_LIB_DIR;LIBNAME" "OUTPUTS;EXTRA_LIB_DIRS;DEPENDS;PRE_COMMANDS;FILES" ${ARGN}
  )

  IF(_ARG_UNPARSED_ARGUMENTS)
    MESSAGE(FATAL_ERROR "RV_STAGE_DEPENDENCY_LIBS: Unknown arguments: ${_ARG_UNPARSED_ARGUMENTS}")
  ENDIF()

  # Validate required args
  IF(NOT _ARG_TARGET)
    MESSAGE(FATAL_ERROR "RV_STAGE_DEPENDENCY_LIBS: TARGET is required")
  ENDIF()

  # Handle LIBNAME shorthand for platform-aware staging
  IF(_ARG_LIBNAME
     AND _ARG_OUTPUTS
  )
    MESSAGE(WARNING "RV_STAGE_DEPENDENCY_LIBS: LIBNAME overrides OUTPUTS")
  ENDIF()
  IF(_ARG_LIBNAME)
    IF(RV_TARGET_WINDOWS)
      IF(NOT _ARG_BIN_DIR)
        IF(DEFINED _bin_dir)
          SET(_ARG_BIN_DIR
              "${_bin_dir}"
          )
        ELSE()
          MESSAGE(FATAL_ERROR "RV_STAGE_DEPENDENCY_LIBS: LIBNAME on Windows requires BIN_DIR or _bin_dir in caller scope")
        ENDIF()
      ENDIF()
      SET(_ARG_OUTPUTS
          "${RV_STAGE_BIN_DIR}/${_ARG_LIBNAME}"
      )
    ELSE()
      SET(_ARG_OUTPUTS
          "${RV_STAGE_LIB_DIR}/${_ARG_LIBNAME}"
      )
    ENDIF()
  ENDIF()

  # Default LIB_DIR to caller's _lib_dir if not explicitly passed
  IF(NOT _ARG_LIB_DIR
     AND NOT _ARG_FILES
  )
    IF(DEFINED _lib_dir)
      SET(_ARG_LIB_DIR
          "${_lib_dir}"
      )
    ENDIF()
  ENDIF()

  IF(NOT _ARG_LIB_DIR
     AND NOT _ARG_FILES
  )
    MESSAGE(FATAL_ERROR "RV_STAGE_DEPENDENCY_LIBS: Either LIB_DIR or FILES is required")
  ENDIF()

  # Default depends to TARGET
  IF(NOT _ARG_DEPENDS)
    SET(_ARG_DEPENDS
        ${_ARG_TARGET}
    )
  ENDIF()

  # Default stage lib dir
  IF(NOT _ARG_STAGE_LIB_DIR)
    SET(_ARG_STAGE_LIB_DIR
        ${RV_STAGE_LIB_DIR}
    )
  ENDIF()

  # Build the command list
  SET(_commands)

  # Pre-commands (e.g., install_name_tool)
  IF(_ARG_PRE_COMMANDS)
    LIST(APPEND _commands ${_ARG_PRE_COMMANDS})
  ENDIF()

  # Copy individual files if specified
  IF(_ARG_FILES)
    FOREACH(
      _file
      ${_ARG_FILES}
    )
      LIST(
        APPEND
        _commands
        COMMAND
        ${CMAKE_COMMAND}
        -E
        copy_if_different
        ${_file}
        ${_ARG_STAGE_LIB_DIR}/
      )
    ENDFOREACH()
  ENDIF()

  # Copy lib dir if specified
  IF(_ARG_LIB_DIR)
    LIST(
      APPEND
      _commands
      COMMAND
      ${CMAKE_COMMAND}
      -E
      copy_directory
      ${_ARG_LIB_DIR}
      ${_ARG_STAGE_LIB_DIR}
    )

    # Copy to extra lib dirs if specified
    FOREACH(
      _extra_dir
      ${_ARG_EXTRA_LIB_DIRS}
    )
      LIST(
        APPEND
        _commands
        COMMAND
        ${CMAKE_COMMAND}
        -E
        copy_directory
        ${_ARG_LIB_DIR}
        ${_extra_dir}
      )
    ENDFOREACH()
  ENDIF()

  # On Windows, copy bin dir (DLLs)
  IF(RV_TARGET_WINDOWS
     AND _ARG_BIN_DIR
  )
    LIST(
      APPEND
      _commands
      COMMAND
      ${CMAKE_COMMAND}
      -E
      copy_directory
      ${_ARG_BIN_DIR}
      ${RV_STAGE_BIN_DIR}
    )
  ENDIF()

  # Copy include dir if specified
  IF(_ARG_INCLUDE_DIR)
    LIST(
      APPEND
      _commands
      COMMAND
      ${CMAKE_COMMAND}
      -E
      copy_directory
      ${_ARG_INCLUDE_DIR}
      ${RV_STAGE_INCLUDE_DIR}
    )
  ENDIF()

  # Determine output tracking
  IF(_ARG_USE_FLAG_FILE)
    SET(_flag_file
        ${CMAKE_CURRENT_BINARY_DIR}/${_ARG_TARGET}-stage-flag
    )
    LIST(
      APPEND
      _commands
      COMMAND
      ${CMAKE_COMMAND}
      -E
      touch
      ${_flag_file}
    )
    SET(_tracking_outputs
        ${_flag_file}
    )
  ELSEIF(_ARG_OUTPUTS)
    SET(_tracking_outputs
        ${_ARG_OUTPUTS}
    )
  ELSE()
    MESSAGE(FATAL_ERROR "RV_STAGE_DEPENDENCY_LIBS: Either OUTPUTS or USE_FLAG_FILE is required")
  ENDIF()

  # Create the custom command (always OUTPUT-based for proper incremental builds). Note: ${_commands} contains COMMAND keywords which CMake's keyword parser
  # uses as section boundaries, so they are correctly parsed as commands, not as additional OUTPUT entries.
  ADD_CUSTOM_COMMAND(
    COMMENT "Staging ${_ARG_TARGET} libs into ${_ARG_STAGE_LIB_DIR}"
    OUTPUT ${_tracking_outputs} ${_commands}
    DEPENDS ${_ARG_DEPENDS}
  )

  # Create the stage target
  ADD_CUSTOM_TARGET(
    ${_ARG_TARGET}-stage-target ALL
    DEPENDS ${_tracking_outputs}
  )

  ADD_DEPENDENCIES(dependencies ${_ARG_TARGET}-stage-target)

ENDFUNCTION()
