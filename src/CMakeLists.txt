#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

ADD_SUBDIRECTORY(plugins)
ADD_SUBDIRECTORY(lib)

# Build dependencies in Release
set(CMAKE_BUILD_TYPE_BACKUP ${CMAKE_BUILD_TYPE})
set(CMAKE_BUILD_TYPE Release)

ADD_SUBDIRECTORY(pub)

set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE_BACKUP})

ADD_SUBDIRECTORY(bin)
ADD_SUBDIRECTORY(test)
