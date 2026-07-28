// Copyright 2026 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
// Stub HTTP transport for Open RV Linux builds. RV stores crash dumps locally
// only (no upload URL), so HTTPTransport::Create() returns nullptr and upload
// attempts fail without linking system libcurl or libldap.

#include "util/net/http_transport.h"

#include <memory>

namespace crashpad
{

    // static
    std::unique_ptr<HTTPTransport> HTTPTransport::Create() { return nullptr; }

} // namespace crashpad
