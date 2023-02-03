#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
# 
# SPDX-License-Identifier: Apache-2.0 
#

def hook_function(in_timeline, argument_map=None):
    print("Warning: Unhandled effect (named {})".format(in_timeline.effect_name))
