#!/usr/bin/env python
# 
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
# 
# SPDX-License-Identifier: Apache-2.0 
# 
import sys

sys.setrecursionlimit(10000)

#
#   Unfortunately, python will throw if k > 6 because its maximum recursion
#   depth is met.
#


def ack(m, n):
    if m == 0:
        return n + 1
    elif n == 0:
        return ack(m - 1, 1)
    else:
        return ack(m - 1, ack(m, n - 1))


k = 8
print(ack(3, k))
