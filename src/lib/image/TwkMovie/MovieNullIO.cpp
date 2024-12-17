//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkMovie/MovieNullIO.h>
#include <TwkMovie/NullWriter.h>

namespace TwkMovie
{

    MovieNullIO::MovieNullIO()
    {
        unsigned int capabilities = MovieIO::MovieWrite
                                    | MovieIO::MovieWriteAudio
                                    | MovieIO::AttributeWrite;

        StringPairVector video;
        StringPairVector audio;
        video.push_back(StringPair("default", "No overhead test"));
        video.push_back(StringPair("copy", "Copy test"));
        addType("null", "Null I/O", capabilities, video, audio);
    }

    MovieNullIO::~MovieNullIO() {}

    std::string MovieNullIO::about() const { return "Null Source or Sink"; }

    MovieReader* MovieNullIO::movieReader() const { return 0; }

    MovieWriter* MovieNullIO::movieWriter() const { return new NullWriter(); }

} // namespace TwkMovie
