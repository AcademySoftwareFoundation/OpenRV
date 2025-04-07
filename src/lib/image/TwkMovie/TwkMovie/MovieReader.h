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

        /// Explanation for preloadOpen(), postPreloadOpen(), and open()
        ///
        /// It used to be that in FileSourceIPNode called
        /// GenericIO::openMovieReader(), which in turn would eventually end up
        /// calling the correct:
        ///
        ///    MovieReader::open(filename, info, request).
        ///
        /// For each derived class of MovieReader, this call performed the
        /// opening of the data file (the I/O part) and afterwards did the work
        /// related to placing the media in the timeline and fixing up
        /// reading/streaming options (eg: realing with the Info/Request
        /// parameters).
        ///
        /// Unfortunately, this was all done synchronsly. So when we loaded a
        /// playlist which had tens, or even several hundreds of clips to open,
        /// this caused a huge amount of wait time due synchonous waiting on the
        /// "file open" I/O, especially when the I/O was over the network, and
        /// even worse if it was done over the internet.
        //
        /// In GenerioIO::openMovieReader(), we introduced a Preloader class,
        /// which, upon receiving a timeline or a list of clips to open, is
        /// meant to start multithreaded preload of the media, thereby greatly
        /// reducing the I/O wait time because it's mostly all done in parallel.
        /// Unfortunately, when we received a list of clips, we don't have
        /// timeline information (eg: we don't have the Info and Request
        /// information), so we can't do that part in parallel. What we can do
        /// however is open the media asynchronously, and then, fixup the
        /// timeline information synchronously when the FileSourceIPNode asks
        /// for the reader.
        ///
        /// For this reason, we added the method preloadOpen(), which is called
        /// by the preloader (async), and then we call postPreloadOpen(), which
        /// deals with the info and request (sync). The open() method, which is
        /// meant be called in sync mode, simply calls both in order.
        ///
        virtual void preloadOpen(const std::string& filename) = 0;
        virtual void
        postPreloadOpen(const MovieInfo& info = MovieInfo(),
                        const ReadRequest& request = ReadRequest()) = 0;

        virtual void open(const std::string& filename,
                          const MovieInfo& info = MovieInfo(),
                          const ReadRequest& request = ReadRequest());

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
