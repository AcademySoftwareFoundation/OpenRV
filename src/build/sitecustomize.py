#
# Copyright (c) 2025 Autodesk, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

"""
Site-level module that ensures OpenSSL will have up to date certificate authorities
on Linux and macOS. It gets imported when the Python interpreter starts up, both
when launching Python as a standalone interpreter or as an embedded one.
The OpenSSL shipped with Desktop requires a list of certificate authorities to be
distributed with the build instead of relying on the OS keychain. In order to keep
an up to date list, we're going to pull it from the certifi module, which incorporates
all the certificate authorities that are distributed with Firefox.
"""

import site
import sys
import os

try:
    import certifi

    # Do not set SSL_CERT_FILE to our own if it is already set. Someone could
    # have their own certificate authority that they specify with this env var.
    # Unfortunately this is not a PATH like environment variable, so we can't
    # concatenate multiple paths with ":".
    #
    # To learn more about SSL_CERT_FILE and how it is being used by OpenSSL when
    # verifying certificates, visit
    # https://www.openssl.org/docs/man1.1.0/ssl/SSL_CTX_set_default_verify_paths.html
    if "SSL_CERT_FILE" not in os.environ and "DO_NOT_SET_SSL_CERT_FILE" not in os.environ:
        os.environ["SSL_CERT_FILE"] = certifi.where()

except ImportError:
    # certifi not installed yet - this is expected during build when pip installs build dependencies
    pass
except Exception as e:
    if "PYTHONVERBOSE" in os.environ:
        # During regular builds, we silently skip these errors so sitecustomize.py can load even
        # if certifi is temporarily unavailable.
        print("Failed to set certifi.where() as SSL_CERT_FILE.", file=sys.stderr)
        print(e, file=sys.stderr)
        print("Set DO_NOT_SET_SSL_CERT_FILE to skip this step in RV's Python initialization.", file=sys.stderr)

try:
    if "DO_NOT_REORDER_PYTHON_PATH" not in os.environ:
        import site
        import sys

        prefixes = list(set(site.PREFIXES))

        # Python libs and site-packages is the first that should be in the PATH
        new_path_list = list(set(site.getsitepackages()))
        new_path_list.insert(0, os.path.dirname(new_path_list[0]))

        # Then any paths in RV's app package
        for path in sys.path:
            for prefix in prefixes:
                if path.startswith(prefix) is False:
                    continue

                if os.path.exists(path):
                    new_path_list.append(path)

        # Then the remaining paths
        for path in sys.path:
            if os.path.exists(path):
                new_path_list.append(path)

        # Save the new sys.path
        sys.path = new_path_list
        site.removeduppaths()

except Exception as e:
    if "PYTHONVERBOSE" in os.environ:
        # Display path-reordering failures only when verbose tracing is explicitly requested.
        print("Failed to reorder RV's Python search path", file=sys.stderr)
        print(e, file=sys.stderr)
        print("Set DO_NOT_REORDER_PYTHON_PATH to skip this step in RV's Python initialization.", file=sys.stderr)
