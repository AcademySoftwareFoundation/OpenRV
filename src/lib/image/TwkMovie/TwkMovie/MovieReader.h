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

        /// New preloading methods for MovieReader's public API.
        ///
        /// To support accelerated loading of your media (helps when
        /// loading long playlists and timelines), RV now
        /// supports parallel preloading of media (on disk & online)
        /// by opening up to 32 simultaneous threads that open
        /// movie files at the same time.
        ///
        /// Indeed, since creating the RV graph is done in the main thread,
        /// opening files one-by-one from an online source (querying the
        /// web session's http cookies and headers, opening an http request,
        /// sending the http request, waiting for the http response, and
        /// downloading the requests's data) we end up cumulating an
        /// enormous amount of time spent waiting for each file's I/O to
        /// complete, with RV being completely idle during this time.
        /// With a large playlist, really adds up.
        ///
        /// To eliminate the waiting time, we instead try to open all files
        /// at the same time (parallelizing the waiting time instead of
        /// serializing it) such that by the time FileSourceIPNode::openMovie
        /// is called, the I/O will have already completed and openMovie()
        /// will completely much more quickly.
        ///
        /// RV's MovieReader::open() method used to take a 3 parametrs:
        /// filename, movieinfo, and read request. At preload time, we
        /// know the filename, we can construct a default read request
        /// (with the web session's cookies) but we don't have any movieInfo
        /// nor do we know what other metadata RV might but in the read request
        ///
        /// Thus, we've broken down the open() process into 2 parts:
        ///
        /// (1) the generic part, which just makes a disk or web request for
        ///     the movie's first chunk of data (eg: movie header?)
        ///
        /// (2) the non-generic part, which consumes the other metadata from
        ///     the first part, and fixes-up whatever needs to be done
        ///     when FileSourceIPNode is ready to call its openMovie() method
        ///
        /// preloadOpen(filename, readrequest)
        ///      runs:       : in the background, called by the preloader
        ///      filename    : the name of the file.
        ///      readrequest : the read request, containing http headers
        ///                    and cookies. This read request is created
        ///                    by the preloader and contains minimal
        ///                    information.
        ///
        /// postPreloadOpen(movieinfo, readrequest)
        ///      runs        : in the main thread, called by FileSourceIPNode
        ///      movieinfo   : movie's general info, like before.
        ///      readrequest : the read request, containing htto headers
        ///                    and cookies, but possibly other information.
        ///                    This one is created by FileSourceIPNode
        ///
        /// As a result of the read requests being possibly different,
        /// we encourage you to save to update your read request
        /// in postPreloadOpen.
        ///
        /// Migrating your open() method is actually quite easy.
        /// It usually simply involves putting the first lines of code
        /// into preloadOpen(), and leave the rest in postPreloadOpen().
        ///
        /// In fact, it can be as simple as only saving the filename in
        /// preloadOpen() (see MovieSideCar) or a bit more complex
        /// (see MovieFFMpegReader)
        ///
        /// Note: open() must still exist, but please do not override it
        ///       in your derived classes. It must exist because if the
        ///       preloader was not invoked for some reason, then open()
        ///       will be called instead, but open() now simply calls
        ///       preloadOpen() and postPrelodaOpen().
        ///

        virtual void preloadOpen(const std::string& filename, const ReadRequest& request);

        virtual void postPreloadOpen(const MovieInfo& info = MovieInfo(), const ReadRequest& request = ReadRequest());

        ///
        ///  Open may throw.
        ///
        virtual void open(const std::string& filename, const MovieInfo& info = MovieInfo(), const ReadRequest& request = ReadRequest());

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

        MovieReader& operator=(const Movie& copy) { return *this; }; /// no assignment

    protected:
        std::string m_filename; /// derived class should set this to the filename
        ReadRequest m_request;
    };

} // namespace TwkMovie

#endif // __TwkMovie__MovieReader__h__
