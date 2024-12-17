//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __MovieFB__MovieFBWriter__h__
#define __MovieFB__MovieFBWriter__h__
#include <TwkMovie/MovieWriter.h>

namespace TwkMovie
{

    //
    //  class MovieFBWriter
    //
    //  MovieFBWriter will create a sequence of individual frames from a
    //  Movie. If you
    //

    class MovieFBWriter : public MovieWriter
    {
    public:
        MovieFBWriter();
        virtual ~MovieFBWriter();

        //
        //  For the FB writer, the filename should be sequence notation.
        //  The writer will use the intersection of the movie contents and
        //  the sequence given.
        //

        virtual bool write(Movie* inMovie, const std::string& filename,
                           WriteRequest&);

    protected:
        Movie* m_movie;
    };

} // namespace TwkMovie

#endif // __MovieFB__MovieFBWriter__h__
