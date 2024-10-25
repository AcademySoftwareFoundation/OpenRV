#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
import unittest

try:
    from PySide2 import QtCore
except ImportError:
    try:
        from PySide6 import QtCore
    except ImportError:
        pass


class TestPySide2(unittest.TestCase):
    def test_getting_qt_instance(self):
        self.assertIsNotNone(
            QtCore.QCoreApplication.instance(),
            "We should have an application defined from py-interp's qt bindings.",
        )


if __name__ == "__main__":
    unittest.main()
