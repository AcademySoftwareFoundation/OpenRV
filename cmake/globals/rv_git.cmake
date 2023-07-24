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
  OUTPUT_VARIABLE RV_GIT_COMMIT_SHORT_HASH_RES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY
)

SET(RV_GIT_COMMIT_SHORT_HASH
    ${RV_GIT_COMMIT_SHORT_HASH_RES}
    CACHE STRING "RV's Commit hash"
)

EXECUTE_PROCESS(
  COMMAND # Should replace the 'COMMIT git log -1 --pretty=format:%h' statement used in the old Makefile
          ${_git} rev-parse --abbrev-ref HEAD
  RESULT_VARIABLE _result
  OUTPUT_VARIABLE RV_GIT_BRANCH_RES
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY
)

SET(RV_GIT_BRANCH
    ${RV_GIT_BRANCH_RES}
    CACHE STRING "RV's branch"
)

MESSAGE(STATUS "Using build branch: ${RV_GIT_BRANCH}")
MESSAGE(STATUS "Using build hash: ${RV_GIT_COMMIT_SHORT_HASH}")
