#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
# 
# SPDX-License-Identifier: Apache-2.0 
#
from rv import commands
from effectHook import *

# this is an example for clip that can be removed or augmented as required
def hook_function(in_timeline, argument_map=None):
    if isinstance(in_timeline.media_reference, otio.schema.ExternalReference):
        related_assets_str = commands.sendInternalEvent(
            "openassetio-get-related-references",
            in_timeline.media_reference.target_url,
        )

        if related_assets_str:
            media_references = in_timeline.media_references()

            for asset_pair in related_assets_str.split(';'):
                name, location = asset_pair.split("|")
                if name and location:
                    media_references[name] = otio.schema.ExternalReference(
                        target_url=location,
                    )

            in_timeline.set_media_references(
                media_references,
                in_timeline.active_media_reference_key,
            )

    return in_timeline
