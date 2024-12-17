//******************************************************************************
// Copyright (c) 2006 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __MovieStdinFB__MovieStdinFB__h__
#define __MovieStdinFB__MovieStdinFB__h__
#include <string>
#include <sys/types.h>
#include <stl_ext/thread_group.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkMovie/MovieReader.h>
#include <TwkMovie/MovieIO.h>

namespace TwkMovie
{

    //
    //  class IOMovieStdinFB
    //
    //  Reads a movie from stdin (cin), expecting a given frame
    //  format. The filename of the movie determines the expected
    //  characteristics of input. For example:
    //
    //      iff,start=1,end=100,inc=1.stdinfb
    //      exr,start=1,end=100,inc=10.stdinfb
    //
    //  image types are any supported generic TwkFB type.
    //

    class MovieStdinFB : public MovieReader
    {
    public:
        //
        //  Types
        //

        typedef std::vector<TwkFB::FrameBuffer*> FrameCache;
        typedef stl_ext::thread_group ThreadGroup;

        //
        //  Constructors
        //

        MovieStdinFB();
        ~MovieStdinFB();

        //
        //  MovieReader API
        //

        virtual void
        open(const std::string& filename, const MovieInfo& as = MovieInfo(),
             const Movie::ReadRequest& request = Movie::ReadRequest());

        //
        //  Movie API
        //

        virtual void imagesAtFrame(const ReadRequest& request,
                                   FrameBufferVector&);
        virtual void identifiersAtFrame(const ReadRequest& request,
                                        IdentifierVector&);

        void cacheFrames();
        void identifier(int frame, std::ostream&);

    private:
        ThreadGroup m_threadGroup;
        FrameCache m_cache;
        const FrameBufferIO* m_io;
        bool m_stopCaching;
        std::string m_imageType;
    };

    //
    //  IO class
    //

    class MovieStdinFBIO : public MovieIO
    {
    public:
        MovieStdinFBIO();
        virtual ~MovieStdinFBIO();

        virtual std::string about() const;
        virtual MovieReader* movieReader() const;
        virtual MovieWriter* movieWriter() const;
        virtual void getMovieInfo(const std::string& filename,
                                  MovieInfo&) const;
    };

} // namespace TwkMovie

#endif // __MovieStdinFB__MovieStdinFB__h__
