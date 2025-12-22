#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
ADD_SUBDIRECTORY(rmsImageDiff)
ADD_SUBDIRECTORY(rvio)

IF(RV_TARGET_LINUX)
  ADD_SUBDIRECTORY(rvio_sw)
ENDIF()

ADD_SUBDIRECTORY(makeFBIOformats)
ADD_SUBDIRECTORY(makeMovieIOformats)
ADD_SUBDIRECTORY(rvls)
