#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

import os
import requests

from http.cookies import SimpleCookie
from tempfile import NamedTemporaryFile
from typing import Dict, Iterable
from urllib.parse import urlparse

# !!!!! W A R N I N G !!!!!
#
# This is a demo package for RV. It is not meant to be used in production.
# It is meant to show how to use the media library API to create a media library
# that can be used to browse media from a web server and/or download the media before
# playing it.
#
# You can use this package as a starting point for your own media library plugin.
# If you want to enable this package to play around it, you can set the following environment variable.
#
# !!!!! W A R N I N G !!!!!
ENABLED_ENV = 'RV_MEDIA_LIBRARY_UNSECURE_DEMO_ENABLED'

# The environment variables is used to set the cookies for the media library.
# The cookies are set as a string of the form:
#   cookie1=value1; [Domain=domain1; Path=path1;] cookie2=value2; [Domain=domain1; Path=path2;]...
# The cookies are set for all urls. However, the cookies can be set for specific urls
# by using the url parameter in the get_http_cookies function and your own logic.
COOKIES_ENV = 'RV_MEDIA_LIBRARY_UNSECURE_DEMO_COOKIES'

# The environment variables that is used to set the headers for the media library.
# The headers are set as a string of the form:
#   header1: value1; header2: value2; ...
# The headers are set for all urls. However, the headers can be set for specific urls
# by using the url parameter in the get_http_headers function and your own logic.
HEADERS_ENV = 'RV_MEDIA_LIBRARY_UNSECURE_DEMO_HEADERS'


# The environment variables that is used to enable the redirection of the urls to disk.
# The redirection is enabled if the environment variable is set.
DOWNLOAD_ENV = 'RV_MEDIA_LIBRARY_UNSECURE_DEMO_FORCE_LOCAL_CACHE'


def is_plugin_enabled() -> bool:
    """
    Returns True if the media library plugin is enabled.

    In this demonstration function, the media library is enabled if the environment variable
    is set.
    """
    return ENABLED_ENV in os.environ


def is_library_media_url(url: str) -> bool:
    """
    Returns True if the given url is a url that should be handled by the media library.

    In this demonstration function, all urls are handled by the media library if any of the
    environment variables are set.
    """
    return COOKIES_ENV in os.environ or HEADERS_ENV in os.environ or DOWNLOAD_ENV in os.environ


def is_streaming(url: str) -> bool:
    """
    Returns True if the given url is a streaming url and should receive cookies and headers.

    In this demostration function, all urls are streaming urls unless DOWNLOAD_ENV is set.
    """

    return DOWNLOAD_ENV not in os.environ


def is_redirecting(url: str) -> bool:
    """
    Returns True if the given url will get redirected by the media library.

    In this demonstration function, if DOWNLOAD_ENV is set, we will be redirecting to a file on disk if the url is from http or https.
    """

    if DOWNLOAD_ENV not in os.environ:
        return False

    return urlparse(url).scheme in ["http", "https"]


def get_http_cookies(url: str) -> Iterable[Dict]:
    """
    Returns a list of cookies to be used for the given url.

    The cookies are returned as a list of dictionaries with the following keys:
        name: The name of the cookie. (mandatory)
        value: The value of the cookie. (mandatory)
        domain: The domain of the cookie.
        path: The path of the cookie.

    In this demonstration function, the cookies are set for all urls. However,
    the cookies can be set for specific urls by using the url parameter.
    """
    cookies_str = os.environ.get(COOKIES_ENV, "")
    if not cookies_str:
        yield from ()

    cookies = SimpleCookie()
    cookies.load(cookies_str)

    for cookie in cookies.values():
        print(cookie)

        yield {
            "name": cookie.key,
            "value": cookie.value,
            "domain": cookie["domain"],
            "path": cookie["path"]
        }

    yield from ()



def get_http_headers(url: str) -> Iterable[Dict]:
    """
    Returns a list of headers to be used for the given url.

    The headers are returned as a list of dictionaries with the following keys:
        name: The name of the header.
        value: The value of the header.

    In this demonstration function, the headers are set for all urls. However, the
    headers can be set for specific urls by using the url parameter.
    """
    headers_str = os.environ.get(HEADERS_ENV, "")
    if not headers_str:
        yield from ()

    for header in headers_str.split(";"):
        if ":" not in header:
            continue

        print(header)

        name, value = header.split(":", 1)
        yield {
            "name": name.strip(),
            "value": value.strip()
        }

    yield from ()

redirection_cache = {}


def get_http_redirection(url: str) -> str:
    """
    Returns the url to redirect to.

    In this demonstration function, it will download the file on disk and redirect to it. Subsequent calls will skip the
    download step.
    """

    global redirection_cache

    if url in redirection_cache:
        return redirection_cache[url].name

    filename = os.path.basename(urlparse(url).path)
    file = NamedTemporaryFile(suffix=filename)

    session = requests.Session()

    for cookie in get_http_cookies(url):
        session.cookies.set(cookie["name"], cookie["value"], domain=cookie["domain"], path=cookie["path"])

    for header in get_http_headers(url):
        session.headers[header["name"]] = header["value"]

    req = session.get(url, stream=True)

    total = req.headers.get("content-length")

    print(f"Downloading {url}.")
    if total is None:
        file.write(req.content)
    else:
        total = int(total)
        current = 0

        for chunk in req.iter_content(round(total / 100)):
            current += len(chunk)
            file.write(chunk)
            print(f"{current} / {total} ({round((current / total) * 100)}%)")
    print(f"Download of {url} completed.")

    redirection_cache[url] = file
    return file.name
