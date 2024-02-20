# Note that this event based Media Library's implementation is not available
# from rvio because rvio does not support modes.
plugin_enabled = True
try:
    from rv.commands import sendInternalEvent
except:
    plugin_enabled = False

from ast import literal_eval
from typing import Dict, Iterable


def is_plugin_enabled() -> bool:
    """
    Returns True if the media library plugin is enabled.
    """
    return plugin_enabled and bool(
        literal_eval(sendInternalEvent("media-library-events-are-enabled") or "True")
    )


def is_library_media_url(url: str) -> bool:
    """
    Returns True if the given url is a url that should be handled by the media library.
    """
    return bool(literal_eval(sendInternalEvent("media-library-is-url", url) or "False"))


def is_streaming(url: str) -> bool:
    """
    Returns True if the given url is a streaming url and should receive cookies and headers.
    """

    return bool(
        literal_eval(sendInternalEvent("media-library-is-streaming", url) or "True")
    )


def is_redirecting(url: str) -> bool:
    """
    Returns True if the given url will get redirected by the media library.
    """

    return bool(
        literal_eval(sendInternalEvent("media-library-is-streaming", url) or "False")
    )


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

    cookies = sendInternalEvent("media-library-get-http-cookies", url)
    if not cookies:
        yield from ()

    for cookie in cookies.splitlines():
        name, value, domain, path = cookie.split(";", 3)

        yield {
            "name": name.strip(),
            "value": value.strip(),
            "domain": domain.strip(),
            "path": path.strip(),
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
    headers = sendInternalEvent("media-library-get-http-headers", url)
    if not headers:
        yield from ()

    for header in headers.splitlines():
        name, value = header.split(":", 1)

        yield {"name": name.strip(), "value": value.strip()}

    yield from ()


def get_http_redirection(url: str) -> str:
    """
    Returns the url to redirect to.

    In this demonstration function, it will download the file on disk and redirect to it. Subsequent calls will skip the
    download step.
    """

    return sendInternalEvent("media-library-get-http-redirection", url) or url
