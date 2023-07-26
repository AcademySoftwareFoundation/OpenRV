#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

#
# The following was inspired by the following post: https://cmake.org/pipermail/cmake/2018-October/068388.html
#
FIND_PROGRAM(_git git NO_CACHE REQUIRED)

EXECUTE_PROCESS(
  COMMAND # Should replace the 'COMMIT git log -1 --pretty=format:%h' statement used in the old Makefile
          ${_git} rev-parse --short HEAD
  RESULT_VARIABLE _result
  OUTPUT_VARIABLE RV_GIT_COMMIT_SHORT_HASH_RESULT
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY
)

EXECUTE_PROCESS(
  COMMAND # Should replace the 'COMMIT git log -1 --pretty=format:%h' statement used in the old Makefile
          ${_git} rev-parse --abbrev-ref HEAD
  RESULT_VARIABLE _result
  OUTPUT_VARIABLE RV_GIT_BRANCH_RESULT
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY
)

MESSAGE(STATUS "Using Open RV build branch: ${RV_GIT_BRANCH_RESULT}")
MESSAGE(STATUS "Using Open RV build hash: ${RV_GIT_COMMIT_SHORT_HASH_RESULT}")

IF(DEFINED RV_GIT_COMMIT_SHORT_HASH)
  SET(RV_GIT_COMMIT_SHORT_HASH
      "${RV_GIT_COMMIT_SHORT_HASH},[Open RV]${RV_GIT_COMMIT_SHORT_HASH_RESULT}"
  )
ELSE()
  SET(RV_GIT_COMMIT_SHORT_HASH
      "${RV_GIT_COMMIT_SHORT_HASH_RESULT}"
  )
ENDIF()
