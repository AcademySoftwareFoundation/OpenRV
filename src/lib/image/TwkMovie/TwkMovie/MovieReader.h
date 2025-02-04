//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkMovie__MovieReader__h__
#define __TwkMovie__MovieReader__h__
#include <TwkMovie/Movie.h>
#include <TwkMovie/dll_defs.h>

namespace TwkMovie
{

    /// Base class for leaf Movies

    ///
    /// Typically, this class is derived from to create a source for
    /// images and/or audio. The MovieReader doesn't really exhibit any
    /// behavior that differs from a regular Movie class other than the
    /// open() function.
    ///
    /// NOTE: it is possible to make a MovieReader which does not access a
    /// file, but instead uses memory or a procedure to generate images
    /// and/or audio.
    ///

    class TWKMOVIE_EXPORT MovieReader : public Movie
    {
    public:
        //
        //  Types
        //

        typedef TwkFB::FrameBuffer FrameBuffer;
        typedef TwkFB::FrameBuffer::DataType DataType;
        typedef TwkFB::FrameBufferIO FrameBufferIO;
        typedef FrameBufferIO::Capabilities Capabilities;
        typedef Movie::ReadRequest ReadRequest;

        MovieReader();
        virtual ~MovieReader();

        ///
        ///  Movie API.
        ///

        const std::string& filename() const { return m_filename; }

        ///
        ///  Open may throw.
        ///

        virtual void open(const std::string& filename,
                          const MovieInfo& info = MovieInfo(),
                          const ReadRequest& request = ReadRequest()) = 0;

        ///
        /// if an additional scanning pass is required after open this
        /// should return true from needsScan() while that is the case.
        ///
        /// scan() will be called by a thread which may not be the same
        /// that called opne(), and should block until complete.
        ///
        /// In addition scanProgress() which may be called from other
        /// threads should return scan progress.
        ///

        virtual bool needsScan() const;
        virtual void scan();
        virtual float scanProgress() const;

        ///
        /// clone the movie object. If it references external resources
        /// (like a file) open the file too. This is used to create
        /// multiple input points for multi-threaded readers
        ///

        virtual Movie* clone() const;

    private:
        MovieReader(const Movie& copy) {} /// no copying

        MovieReader& operator=(const Movie& copy)
        {
            return *this;
        }; /// no assignment

    protected:
        std::string
            m_filename; /// derived class should set this to the filename
        ReadRequest m_request;
    };

} // namespace TwkMovie

#endif // __TwkMovie__MovieReader__h__
