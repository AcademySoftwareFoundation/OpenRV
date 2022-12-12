#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

IF(RV_TARGET_DARWIN)
  ADD_SUBDIRECTORY(RV_app)
ELSE()
  # assuming this is for Linux & Windows
  ADD_SUBDIRECTORY(rv)
ENDIF()
