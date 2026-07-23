#
# Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# RV_GENERATE_SYMBOLS: Generate Breakpad symbols for a target (macOS and Linux)
#
# Usage: RV_GENERATE_SYMBOLS(TARGET <target>)
#
# Generates a Breakpad .sym file for the given target, organized in the standard Breakpad directory structure under stage/app/symbols/. On macOS, also generates
# a dSYM bundle. Only runs on macOS/Linux when Breakpad is available (RV_DEPS_BREAKPAD_VERSION).
#

FUNCTION(RV_GENERATE_SYMBOLS)
  SET(flags
      ""
  )
  SET(args
      ""
  )
  SET(listArgs
      TARGET
  )
  CMAKE_PARSE_ARGUMENTS(arg "${flags}" "${args}" "${listArgs}" ${ARGN})

  IF(NOT arg_TARGET)
    MESSAGE(FATAL_ERROR "RV_GENERATE_SYMBOLS: 'TARGET' parameter is required.")
  ENDIF()

  IF(NOT RV_DEPS_BREAKPAD_VERSION)
    RETURN()
  ENDIF()

  IF(NOT RV_TARGET_DARWIN
     AND NOT RV_TARGET_LINUX
  )
    RETURN()
  ENDIF()

  SET(_symbols_dir
      ${CMAKE_BINARY_DIR}/stage/app/symbols
  )
  SET(_sym_file
      ${CMAKE_CURRENT_BINARY_DIR}/${arg_TARGET}.sym
  )
  SET(_organize_script
      ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../scripts/organize_symbols.sh
  )

  # Ensure Breakpad tools are built before generating symbols
  ADD_DEPENDENCIES(${arg_TARGET} RV_DEPS_BREAKPAD-stage-target)

  IF(RV_TARGET_DARWIN)
    # Generate dSYM bundle (macOS only)
    ADD_CUSTOM_COMMAND(
      TARGET ${arg_TARGET}
      POST_BUILD
      COMMENT "Generating dSYM for ${arg_TARGET}"
      COMMAND dsymutil $<TARGET_FILE:${arg_TARGET}> -o $<TARGET_FILE:${arg_TARGET}>.dSYM
      VERBATIM
    )
  ENDIF()

  # Convert to Breakpad .sym format using dump_syms Note: uses sh -c with positional parameters and VERBATIM so CMake properly escapes all arguments (including
  # the redirect >), avoiding the backslash-space-before-> issue that causes Ninja to create a redirect target with a leading space.
  ADD_CUSTOM_COMMAND(
    TARGET ${arg_TARGET}
    POST_BUILD
    COMMENT "Generating Breakpad symbols for ${arg_TARGET}"
    COMMAND sh -c "${RV_STAGE_BIN_DIR}/dump_syms \"$1\" > \"$2\"" -- $<TARGET_FILE:${arg_TARGET}> ${_sym_file}
    VERBATIM
  )

  # On Linux, RV_STAGE renames the binary to .bin and installs a shell wrapper in its place.  dump_syms runs before the rename (so it sees the real ELF), but
  # records the pre-rename name as the module.  Pass the final .bin name so organize_symbols.sh can patch the module name before building the directory tree.
  SET(_organize_module_name
      ""
  )
  IF(RV_TARGET_LINUX
     AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${arg_TARGET}.wrapper
  )
    SET(_organize_module_name
        "${arg_TARGET}.bin"
    )
  ENDIF()

  # Organize .sym file in Breakpad directory structure (symbols/MODULE_NAME/MODULE_ID/)
  ADD_CUSTOM_COMMAND(
    TARGET ${arg_TARGET}
    POST_BUILD
    COMMENT "Organizing Breakpad symbols for ${arg_TARGET}"
    COMMAND bash ${_organize_script} ${_sym_file} ${_symbols_dir} ${_organize_module_name}
    VERBATIM
  )

  IF(RV_TARGET_DARWIN)
    # The .dSYM is only a transient input for dump_syms (above); once the .sym is generated it has no runtime purpose. Remove it so it does not linger next to
    # the staged binary -- in plugin directories the loaders glob *.dylib and would otherwise pick up the *.dylib.dSYM bundle and fail to dlopen it.
    ADD_CUSTOM_COMMAND(
      TARGET ${arg_TARGET}
      POST_BUILD
      COMMENT "Removing intermediate dSYM for ${arg_TARGET}"
      COMMAND ${CMAKE_COMMAND} -E rm -rf $<TARGET_FILE:${arg_TARGET}>.dSYM
      VERBATIM
    )
  ENDIF()
ENDFUNCTION()
