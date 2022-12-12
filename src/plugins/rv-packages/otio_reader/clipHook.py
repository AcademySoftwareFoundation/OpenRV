#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
# 
# SPDX-License-Identifier: Apache-2.0 
#
from rv import commands
from effectHook import *

# this is an example for clip that can be removed or augmented as required
def hook_function(in_timeline, argument_map=None):
    return in_timeline
