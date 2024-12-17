//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkMovie__NullWriter__h__
#define __TwkMovie__NullWriter__h__
#include <TwkMovie/MovieWriter.h>
#include <TwkMovie/dll_defs.h>

namespace TwkMovie
{

    /// A sink for images which goes nowhere

    ///
    /// This class consumes movie frames without writing them. Its most
    /// useful to test throughput without output. The class provides a
    /// special codec called "copy" which will copy the data in the images
    /// before discarding it. This can be used to simulate performance of
    /// a renderer for example which requires a copy.
    ///
    /// If the verbose flag of the request is true, the NullWriter will
    /// output timing information to cout.
    ///

    class TWKMOVIE_EXPORT NullWriter : public MovieWriter
    {
    public:
        NullWriter();
        virtual ~NullWriter();

        ///
        /// Opens the Movie filename for writing with the specified codec
        /// (or "" for default codec). The thread will return from the
        /// function when the movie as finished writing.
        ///

        virtual bool write(Movie* inMovie, const std::string& filename,
                           WriteRequest&);
    };

} // namespace TwkMovie

#endif // __TwkMovie__NullWriter__h__
