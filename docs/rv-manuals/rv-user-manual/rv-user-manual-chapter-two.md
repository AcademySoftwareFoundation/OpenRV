# Chapter 2 - Installation

Refer to the Open RV README to learn how to build and install Open RV.

If you're using network proxies or self-signed certificates, read on.

## Proxy Configuration

If your network uses a proxy server, you can configure Open RV to use it by setting the following environment variables.

| Environment variable      | Type    | Required? | Description                   |
| ------------------------- | ------- | --------- | ----------------------------- |
| RV_NETWORK_PROXY_HOST     | string  | required  | Proxy host name or IP address |
| RV_NETWORK_PROXY_PORT     | int     | required  | Proxy port number             |
| RV_NETWORK_PROXY_USER     | string  | optional  | Username for proxy service    |
| RV_NETWORK_PROXY_PASSWORD | string  | optional  | Password for proxy service    |

> Note: If you do not set a proxy, Open RV always uses the operating system's proxy. To have Open RV to ignore all proxies (including the system's own), set the environment variable `RV_NETWORK_PROXY_DISABLE`.

## About Self-Signed Certificates and Open RV

Open RV uses QtWebEngine as a browser backend, built on top of Chromium. Because of the Chromium backend, Open RV uses the system certificate store to approve SSL certificates. If your studio uses a custom certificate on the network path between Open RV and your media, you must make sure that the Certificate Authority (CA) is in the system’s centralized store. Adding the certificate to Google Chrome is the easy method, but you can use any method supported by your operating system.

The Python version embedded with Open RV uses its own built-in certificates file, which stores root certificates for the most common certificate authorities. But you can have Python use your self-signed certificate by setting the path to the file in the `SSL_CERT_FILE` environment variable.
