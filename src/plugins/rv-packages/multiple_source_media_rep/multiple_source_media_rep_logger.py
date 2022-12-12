# Copyright (c) 2022 Autodesk.
#
# CONFIDENTIAL AND PROPRIETARY
#
# This work is provided "AS IS" and subject to the ShotGrid Pipeline Toolkit
# Source Code License included in this distribution package. See LICENSE.
# By accessing, using, copying or modifying this work you indicate your
# agreement to the ShotGrid Pipeline Toolkit Source Code License. All rights
# not expressly granted therein are reserved by ShotGrid Software Inc.

import logging
import os

logging.basicConfig()

package_logger = logging.getLogger("MultipleSourceMediaRep")

if "RV_MULTI_MEDIA_REP_DEBUG" in os.environ:
    package_logger.setLevel(logging.DEBUG)
else:
    package_logger.setLevel(logging.INFO)
