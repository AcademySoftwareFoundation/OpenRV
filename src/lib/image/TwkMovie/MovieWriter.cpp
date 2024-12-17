//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkMovie/MovieWriter.h>

namespace TwkMovie
{

    //----------------------------------------------------------------------

    static bool reallyVerbose = false;

    MovieWriter::MovieWriter() {}

    MovieWriter::~MovieWriter() {}

    static bool really;

    bool MovieWriter::reallyVerbose() { return really; }

    void MovieWriter::setReallyVerbose(bool b) { really = b; }

    MovieInfo MovieWriter::open(const MovieInfo& info,
                                const std::string& filename,
                                const WriteRequest& request)
    {
        m_request = request;
        m_filename = filename;
        return info;
    }

    bool MovieWriter::write(Movie* inMovie)
    {
        return write(inMovie, m_filename, m_request);
    }

} // namespace TwkMovie
