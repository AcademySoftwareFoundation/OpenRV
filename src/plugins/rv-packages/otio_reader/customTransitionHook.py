#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
from rv import commands


def _create_node(in_transition, context={}):
    return commands.newNode("CustomTransition", in_transition.name or "custom_transition")


def hook_function(in_timeline, argument_map=None):
    pass
