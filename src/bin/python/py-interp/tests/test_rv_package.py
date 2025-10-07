#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
"""
Simple test just asserting that our application
setup is allowing the import of RV's main package.
"""

import unittest


class TestRvPackage(unittest.TestCase):
    def test_importing_rv_package(self):
        import rv  # noqa: F401


if __name__ == "__main__":
    unittest.main()
