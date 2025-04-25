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

    void MovieReader::preloadOpen(const std::string& filename,
                                  const Movie::ReadRequest& request)
    {
        std::cerr << "ERROR: preloadOpen must be implemented" << std::endl;
        throw std::runtime_error("preloadOpen must be implemented");
    }

    void MovieReader::postPreloadOpen(const MovieInfo& info,
                                      const Movie::ReadRequest& request)
    {
        std::cerr << "ERROR: postPreloadOpen must be implemented" << std::endl;
        throw std::runtime_error("postPreloadOpen must be implemented");
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
