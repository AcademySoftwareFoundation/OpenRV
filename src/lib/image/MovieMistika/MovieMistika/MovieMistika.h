//******************************************************************************
// Copyright (c) 2006 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __MovieMistika__MovieMistika__h__
#define __MovieMistika__MovieMistika__h__
#include <string>
// #include <sys/types.h>
// #include <TwkFB/FrameBuffer.h>
#include <TwkMovie/MovieReader.h>
#include <TwkMovie/MovieIO.h>

namespace TwkMovie
{

    class MistikaFileHeader;

    //
    //  class IOMovieMistika
    //

    class MovieMistika : public MovieReader
    {
    public:
        //
        //  Constructors
        //

        MovieMistika();
        ~MovieMistika();

        enum Format
        {
            RGB8,
            RGBA8,
            RGB8_PLANAR,
            RGB10_A2,
            RGB16,
            RGBA16,
            RGB16_PLANAR
        };

        //
        //  MovieReader API
        //

        virtual MovieReader* clone() const;
        virtual void
        open(const std::string& filename, const MovieInfo& as = MovieInfo(),
             const Movie::ReadRequest& request = Movie::ReadRequest());

        //
        //  Movie API

        void setErrorMessage(const std::string& s) { m_errorMessage = s; }

        virtual void imagesAtFrame(const ReadRequest& request,
                                   FrameBufferVector&);
        virtual void identifiersAtFrame(const ReadRequest& request,
                                        IdentifierVector&);

        static Format pixelFormat;

    private:
        void identifier(int frame, std::ostream&);

    private:
        std::string m_errorMessage;
        MistikaFileHeader* m_header;
        bool m_swap;
    };

    //
    //  IO class
    //

    class MovieMistikaIO : public MovieIO
    {
    public:
        MovieMistikaIO();
        virtual ~MovieMistikaIO();

        virtual std::string about() const;
        virtual MovieReader* movieReader() const;
        virtual MovieWriter* movieWriter() const;
        virtual void getMovieInfo(const std::string& filename,
                                  MovieInfo&) const;
    };

} // namespace TwkMovie

#endif // __MovieMistika__MovieMistika__h__
