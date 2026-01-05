#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

SET(_target
    "package-rv"
)

ADD_CUSTOM_TARGET(${_target})

# Right now this is mostly to copy to the build folder
CONFIGURE_FILE(rvrc.mu ${RV_STAGE_SCRIPTS_DIR}/rv/rvrc.mu COPYONLY)
CONFIGURE_FILE(rviorc.mu ${RV_STAGE_SCRIPTS_DIR}/rv/rviorc.mu COPYONLY)
CONFIGURE_FILE(rvrc.py ${RV_STAGE_SCRIPTS_DIR}/rv/rvrc.py COPYONLY)
CONFIGURE_FILE(qt.conf ${RV_STAGE_BIN_DIR}/qt.conf COPYONLY)

IF(RV_TARGET_WINDOWS)
  FILE(
    GLOB regfiles
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    *.reg*
  )
  FOREACH(
    regfile
    ${regfiles}
  )
    CONFIGURE_FILE(${regfile} ${RV_APP_ROOT}/etc/${regfile} COPYONLY)
  ENDFOREACH()
  CONFIGURE_FILE(README.regfiles ${RV_APP_ROOT}/etc/README.regfiles COPYONLY)
ENDIF()

IF(RV_TARGET_LINUX)
  CONFIGURE_FILE(rv.install_handler_linux ${RV_STAGE_BIN_DIR} COPYONLY)
  CONFIGURE_FILE(rv.install_handler_linux_rvpush ${RV_STAGE_BIN_DIR} COPYONLY)
ENDIF()