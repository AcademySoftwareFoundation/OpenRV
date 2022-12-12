#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
# 
# SPDX-License-Identifier: Apache-2.0 
#
"""
Simple tests asserting that our testing setup
does report failures.
"""
import unittest


class TestMiscFailures(unittest.TestCase):
    def test_importing_invalid_package(self):
        with self.assertRaises(ImportError):
            import non_existing_package

    # def test_invalid_syntax(self):
    #    with self.assertRaises(SyntaxError):
    #        if(True) print "Hello from IF Python!"
    #        else print "Hello from ELSE Python !"

    def test_calling_non_existing_method(self):
        with self.assertRaises(NameError):
            pirnt("Hello from Python!")


if __name__ == "__main__":
    unittest.main()
