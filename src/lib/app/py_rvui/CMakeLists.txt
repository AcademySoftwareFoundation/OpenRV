#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

SET(_target
    py_rvui
)

RV_STAGE(
  TARGET
  ${_target}
  TYPE
  "PYTHON_SOURCE_MODULE"
  FILES
  rv
  rv_commands_setup.py
)
