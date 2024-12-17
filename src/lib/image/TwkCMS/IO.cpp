//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <IO.h>
#include <stl_ext/string_algo.h>

namespace TwkCMS
{

    IO::IO(const char* path)
    {
        vector<string> tokens;
        stl_ext::tokenize(tokens, path, ":");
    }

} // namespace TwkCMS
