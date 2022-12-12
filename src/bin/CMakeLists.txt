#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

ADD_SUBDIRECTORY(apps)
ADD_SUBDIRECTORY(mu)
IF(RV_TARGET_DARWIN)
  ADD_SUBDIRECTORY(nsapps)
ENDIF()
ADD_SUBDIRECTORY(gtotools)
ADD_SUBDIRECTORY(imgtools)
ADD_SUBDIRECTORY(python)
