"""
Example plugin showing how otio files can be loaded into an RV context
"""
#
# Copyright Contributors to the OpenTimelineIO project
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#

from __future__ import print_function

import os
import sys

from rv import commands, extra_commands
from rv import rvtypes

import opentimelineio as otio

import otio_reader
import otio_writer


class Mode(object):
    sleeping = 1
    loading = 2
    processing = 3


class ExampleOTIOReaderPlugin(rvtypes.MinorMode):
    def __init__(self):
        super(ExampleOTIOReaderPlugin, self).__init__()
        self.init(
            "example_otio_reader",
            [
                (
                    "incoming-source-path",
                    self.incoming_source_path,
                    "Catch otio supported files and return movieproc",
                ),
                (
                    "source-group-complete",
                    self.source_group_complete,
                    "Expand otio supported files (synchronous load)",
                ),
                (
                    "after-progressive-loading",
                    self.after_progressive_loading,
                    "Expand otio supported files (asynchronous load)",
                ),
                (
                    "otio-import-enabled",
                    self.is_enabled,
                    "Check if OTIO import is enabled",
                ),
                (
                    "open-file-dialog-filters",
                    self.set_open_otio_filter,
                    "Add otio extension to open file filters",
                ),
                ("otio-export", self.otio_export, "Export to OTIO file"),
            ],
            None,
        )

        # Start as sleeping
        self.mode = Mode.sleeping

    def incoming_source_path(self, event):
        """
        Detects if a file supported by otio is being loaded, and replaces
        it with an empty movie proc containing an otioFile tag. This will be
        replaced in expand_sources().
        """
        event.reject()

        parts = event.contents().split(";")
        in_path = parts[0]
        _, ext = os.path.splitext(in_path)
        if ext:
            ext = ext[1:]

        if ext in otio.adapters.suffixes_with_defined_adapters(read=True):
            self.mode = Mode.loading
            movieproc = "blank,otioFile={}.movieproc".format(in_path)
            event.setReturnContent(movieproc)

    def after_progressive_loading(self, event):
        """
        After progress loading event, is used for asynchronous addSource
        file loading.
        """
        event.reject()

        if self.mode != Mode.loading:
            return
        self.mode = Mode.processing
        self.expand_sources()

    def source_group_complete(self, event):
        """
        Source group complete event, is used for synchronous addSource
        file loading.
        """
        event.reject()
        if self.mode == Mode.sleeping:
            # this plugin isn't doing anything
            return

        if self.mode == Mode.processing:
            # already processing otio
            return

        if commands.loadTotal() > 0:
            # async load
            return

        self.mode = Mode.processing
        self.expand_sources()

    def is_enabled(self, event):
        """
        Detects if OTIO Package is installed for OTIO file import.
        """
        event.reject()
        event.setReturnContent("enabled")

    def set_open_otio_filter(self, event):
        """
        Adds OTIO to Open File dialog filters
        """
        event.reject()
        event.setReturnContent(
            "{}:{}".format(event.returnContents(), "otio;OpenTimelineIO")
        )

    def otio_export(self, event):
        """
        Exports the currently viewed node to the OTIO file from the event
        """
        event.reject()
        otio_timeline = otio_writer.write_otio_file(
            commands.viewNode(), event.contents()
        )

    def expand_sources(self):
        """
        Expand any movie movieproc otioFile sources.
        """
        # disable caching for load speed
        cache_mode = commands.cacheMode()
        commands.setCacheMode(commands.CacheOff)

        try:
            # find sources with a movieproc with an otioFile=foo.otio tag
            default_inputs, _ = commands.nodeConnections("defaultSequence")
            for src in commands.nodesOfType("RVSource"):
                src_group = commands.nodeGroup(src)
                if src_group not in default_inputs:
                    # not in default sequence, already processed
                    continue

                # get the source file name
                paths = [
                    info["file"]
                    for info in commands.sourceMediaInfoList(src)
                    if "file" in info
                ]
                for info_path in paths:
                    # Looking for: 'blank,otioFile=/foo.otio.movieproc'
                    parts = info_path.split("=", 1)
                    itype = parts[0]
                    if not itype.startswith("blank,otioFile"):
                        continue
                    # remove the .movieproc extension
                    path, _ = os.path.splitext(parts[1])

                    # remove temp movieproc source from current view, and all
                    # the default views
                    _remove_source_from_views(src_group)

                    result = otio_reader.read_otio_file(path)
                    commands.setViewNode(result)
                    break
        except Exception as e:
            message = "ERROR: {}".format(e)
            extra_commands.displayFeedback(message, 5.0)
            print(message, file=sys.stderr)
            raise e
        finally:
            # turn cache mode back on and go back to sleep
            commands.setCacheMode(cache_mode)
            self.mode = Mode.sleeping


def _remove_source_from_views(source_group):
    """
    Remove a source group from all views.
    """
    for view in commands.viewNodes():
        view_inputs = commands.nodeConnections(view)[0]
        if source_group in view_inputs:
            view_inputs.remove(source_group)
            commands.setNodeInputs(view, view_inputs)


def createMode():
    support_files_path = os.path.join(
        os.path.dirname(os.path.realpath(__file__)), "..", "SupportFiles", "otio_reader"
    )

    manifest_path = os.environ.get("OTIO_PLUGIN_MANIFEST_PATH", "")
    if manifest_path:
        manifest_path += os.pathsep
    os.environ["OTIO_PLUGIN_MANIFEST_PATH"] = manifest_path + os.path.join(
        support_files_path, "manifest.json"
    )
    sys.path.append(support_files_path)

    return ExampleOTIOReaderPlugin()
