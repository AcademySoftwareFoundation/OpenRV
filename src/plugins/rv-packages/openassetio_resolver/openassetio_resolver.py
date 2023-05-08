"""
Resolver for openassetio assets
"""
# *****************************************************************************
# Copyright (c) 2023 Autodesk, Inc.
# All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# *****************************************************************************

import os
import sys
from functools import partial

from rv.rvtypes import MinorMode
from rv import commands

from openassetio.log import ConsoleLogger, SeverityFilter
from openassetio.hostApi import HostInterface, ManagerFactory
from openassetio.pluginSystem import PythonPluginSystemManagerImplementationFactory

from openassetio_mediacreation.traits.content import LocatableContentTrait
from openassetio_mediacreation.traits.auth import BearerTokenTrait


class OpenAssetIOPlugin(MinorMode):
    class OpenRvHost(HostInterface):
        def identifier(self):
            return "io.aswf.openrv"

        def displayName(self):
            return "OpenRV"

    def __init__(self):
        super(OpenAssetIOPlugin, self).__init__()
        self.init(
            "openassetio_resolver",
            [
                (
                    "incoming-source-path",
                    self.incoming_source_path,
                    "Resolve incoming reference if its an OpenAssetIO URI",
                ),
                (
                    "openassetio-get-related-references",
                    self.get_related_references,
                    "Return related references if there are any",
                ),
            ],
            None,
        )
        self.manager = None

        if os.environ.get("OPENASSETIO_DEFAULT_CONFIG"):
            logger = SeverityFilter(ConsoleLogger())
            impl_factory = PythonPluginSystemManagerImplementationFactory(logger)
            host_interface = self.OpenRvHost()

            self.manager = ManagerFactory.defaultManagerForInterface(
                host_interface, impl_factory, logger
            )

    @staticmethod
    def resolve_asset(event, _, data):
        locatable = LocatableContentTrait(data)
        if locatable.isImbued():
            event.setReturnContent(LocatableContentTrait(data).getLocation())

    @staticmethod
    def fail_with_message(_, batch_element_error):
        sys.stderr.write(f"openassetio_resolver ERROR: {batch_element_error.message}\n")

    def incoming_source_path(self, event):
        event.reject()
        if not self.manager:
            return

        in_path = event.contents().split(";")[0]
        entity_reference = self.manager.createEntityReferenceIfValid(in_path)

        if entity_reference:
            context = self.manager.createContext()
            context.access = context.Access.kRead

            token = commands.sendInternalEvent("url-get-bearer-token", in_path)
            if token:
                BearerTokenTrait.imbueTo(context.locale)
                BearerTokenTrait(context.locale).setToken(token)

            self.manager.resolve(
                [entity_reference],
                {LocatableContentTrait.kId},
                context,
                partial(self.resolve_asset, event),
                self.fail_with_message,
            )

    def get_related_references(self, event):
        event.reject()
        if not self.manager:
            return

        # TODO: resolve related from URI in event.contents() and
        # return them in a delminited string in event.setReturnContent()
        event.setReturnContent("asset2|bal:///rv_asset2;asset3|bal:///rv_asset3")


def createMode():
    return OpenAssetIOPlugin()
