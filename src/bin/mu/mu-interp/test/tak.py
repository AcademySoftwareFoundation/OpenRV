#! /depot/python/arch/bin/python
# 
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
# 
# SPDX-License-Identifier: Apache-2.0 
#
# takeuchi benchmark in Python
# see <url:http://www.lib.uchicago.edu/keith/crisis/benchmarks/tak/>
# Keith Waclena <k-waclena@uchicago.edu>


def tak(x, y, z):
    if not (y < x):
        return z
    else:
        return tak(tak(x - 1, y, z), tak(y - 1, z, x), tak(z - 1, x, y))


# print tak(18, 12, 6)
print(tak(28, 12, 6))
