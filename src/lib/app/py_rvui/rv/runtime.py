#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
# 
# SPDX-License-Identifier: Apache-2.0 
#
import os


def __dummy__():
    pass


def setup_webview_default_profile():
    default_profile = QWebEngineProfile.defaultProfile()
    user_agent = (
        default_profile.httpUserAgent() + " RV/" + os.environ["TWK_APP_VERSION"]
    )
    default_profile.setHttpUserAgent(user_agent)


from PySide2.QtWebEngineWidgets import QWebEngineProfile

setup_webview_default_profile()
