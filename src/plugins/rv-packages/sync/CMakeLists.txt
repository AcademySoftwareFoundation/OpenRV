#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

SET(_target
    "sync"
)

IF(RV_VFX_PLATFORM STREQUAL CY2024)
  SET(SYNC_MU_DEB
      "print(\"sync(%s,%s): %s\\n\" % (myNetworkPort(), d.toString(\"mm:ss:zzz\", qt.QCalendar()), s));\n"
  )
ELSEIF(RV_VFX_PLATFORM STREQUAL CY2023)
  SET(SYNC_MU_DEB
      "print(\"sync(%s,%s): %s\\n\" % (myNetworkPort(), d.toString(\"mm:ss:zzz\"), s));\n"
  )
ENDIF()

CONFIGURE_FILE(sync.mu.in ${CMAKE_CURRENT_SOURCE_DIR}/sync.mu @ONLY)

RV_STAGE(TYPE "RVPKG" TARGET ${_target})
