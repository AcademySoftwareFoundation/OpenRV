#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

SET(_target
    "custom_mattes"
)

SET(_files
    custom_mattes.py mattes
)

RV_STAGE(TYPE "RVPKG" TARGET ${_target} FILES ${_files})
