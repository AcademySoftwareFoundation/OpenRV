#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

MACRO(RV_COPY_LIB_BIN_FOLDERS)

  IF(NOT _target)
    MESSAGE(FATAL_ERROR "The '_target' variable was not set.")
  ENDIF()

  IF(NOT _libname)
    MESSAGE(FATAL_ERROR "The '_libname' variable was not set.")
  ENDIF()

  IF(NOT _lib_dir)
    MESSAGE(FATAL_ERROR "The '_lib_dir' variable was not set.")
  ENDIF()

  IF(NOT _bin_dir)
    MESSAGE(FATAL_ERROR "The '_bin_dir' variable was not set.")
  ENDIF()

  IF(RV_TARGET_WINDOWS)

    ADD_CUSTOM_COMMAND(
      TARGET ${_target}
      POST_BUILD
      COMMENT "Installing ${_target}'s libs and bin into ${RV_STAGE_LIB_DIR} and ${RV_STAGE_BIN_DIR}"
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${RV_STAGE_LIB_DIR}
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${_bin_dir} ${RV_STAGE_BIN_DIR}
    )
    ADD_CUSTOM_TARGET(
      ${_target}-stage-target ALL
      DEPENDS ${RV_STAGE_BIN_DIR}/${_libname}
    )
  ELSE()
    ADD_CUSTOM_COMMAND(
      COMMENT "Installing ${_target}'s libs into ${RV_STAGE_LIB_DIR}"
      OUTPUT ${RV_STAGE_LIB_DIR}/${_libname}
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${_lib_dir} ${RV_STAGE_LIB_DIR}
      DEPENDS ${_target}
    )
    ADD_CUSTOM_TARGET(
      ${_target}-stage-target ALL
      DEPENDS ${RV_STAGE_LIB_DIR}/${_libname}
    )
  ENDIF()
ENDMACRO()
