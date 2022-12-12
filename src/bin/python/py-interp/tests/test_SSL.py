#!/usr/bin/env python

# *****************************************************************************
# Copyright (c) 2020 Autodesk, Inc.
# All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# *****************************************************************************


def _fetch(name, func, url):
    try:
        func(url)
    except Exception as e:
        print(f"[{name}]: Unable to fetch {url} => {e}")
        return False

    print(f"[{name}]: Successfully fetched {url}".format(name=name, url=url))
    return True


def fetch_httplib(url):
    def impl(url):
        from six.moves import http_client
        from six.moves import urllib_parse

        u = urllib_parse.urlparse(url)
        http_client.HTTPSConnection(u.netloc).request("GET", u.path)

    return _fetch("httplib", impl, url)


def fetch_urllib(url):
    def impl(url):
        from six.moves import urllib

        urllib.request.urlopen(url).read()

    return _fetch("urllib", impl, url)


def fetch_requests(url):
    def impl(url):
        import requests

        requests.get(url).raise_for_status()

    return _fetch("requests", impl, url)


app = None


def fetch_pyside(url):
    def impl(url):
        from PySide2.QtCore import QEventLoop, QUrl, QCoreApplication
        from PySide2.QtNetwork import (
            QNetworkRequest,
            QNetworkReply,
            QNetworkAccessManager,
        )

        global app
        if app is None:
            app = QCoreApplication.instance() or QCoreApplication()

        event_loop = QEventLoop()

        manager = QNetworkAccessManager()
        manager.finished[QNetworkReply].connect(lambda: event_loop.exit(0))

        reply = manager.get(QNetworkRequest(QUrl(url)))
        event_loop.exec_()

        if reply.error() != QNetworkReply.NoError:
            raise AssertionError(reply.errorString())

    return _fetch("PySide2.QNetwork.QNetworkRequest", impl, url)


def fetch_all(url, is_valid_url):
    results = list()

    results.append(fetch_httplib(url))
    results.append(fetch_urllib(url))
    results.append(fetch_requests(url))
    results.append(fetch_pyside(url))

    if is_valid_url is True and all(results) is False:
        raise AssertionError(f"FAILED: One of the libraries failed to fetch {url}")

    elif is_valid_url is False and any(results) is True:
        raise AssertionError(
            f"FAILED: One of the libraries failed to raise an error for {url}"
        )

    print(f" - All libraries behaved well on {url}\n")


import unittest


class TestSSL(unittest.TestCase):
    def test_good_urls(self):
        # Source: https://aws.amazon.com/blogs/security/how-to-prepare-for-aws-move-to-its-own-certificate-authority/
        good_url = [
            "https://good.sca1a.amazontrust.com/",
            "https://good.sca2a.amazontrust.com/",
            "https://good.sca3a.amazontrust.com/",
            "https://good.sca4a.amazontrust.com/",
            "https://good.sca0a.amazontrust.com/",
        ]
        for url in good_url:
            fetch_all(url, True)

    def test_bad_urls(self):
        bad_url = ["https://untrusted-root.badssl.com/"]

        for url in bad_url:
            fetch_all(url, False)


if __name__ == "__main__":
    unittest.main()
