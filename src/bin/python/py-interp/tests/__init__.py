#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
# 
# SPDX-License-Identifier: Apache-2.0 
#
import os
import unittest


class TestBaseSetup(unittest.TestCase):
    def test_qt_env_var(self):
        qt_platform = os.environ.get("QT_QPA_PLATFORM")
        self.assertIsNotNone(
            qt_platform, "Was expecting the QT_QPA_PLATFORM environment variable"
        )
        self.assertEqual("minimal", qt_platform)
