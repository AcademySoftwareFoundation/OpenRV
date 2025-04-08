//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkMovie/MovieReader.h>

namespace TwkMovie
{

    MovieReader::MovieReader()
        : Movie()
    {
    }

    void MovieReader::open(const std::string& filename, const MovieInfo& info,
                           const Movie::ReadRequest& request)
    {
        preloadOpen(filename, request);
        postPreloadOpen(info, request);
    }

    MovieReader::~MovieReader() {}

    Movie* MovieReader::clone() const { return 0; }

    void MovieReader::scan() {}

    float MovieReader::scanProgress() const { return 1.0; }

    bool MovieReader::needsScan() const { return false; }

} // namespace TwkMovie
