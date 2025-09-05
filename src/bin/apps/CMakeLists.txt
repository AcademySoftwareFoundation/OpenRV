#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

ADD_SUBDIRECTORY(rvshell)

IF(RV_TARGET_LINUX
   OR RV_TARGET_WINDOWS
)
  ADD_SUBDIRECTORY(rv)
ENDIF()

ADD_SUBDIRECTORY(rvpush)
ADD_SUBDIRECTORY(rvpkg)
ADD_SUBDIRECTORY(rvprof)

IF(RV_TARGET_DARWIN)
  ADD_SUBDIRECTORY(rvlinklauncher)
ENDIF()