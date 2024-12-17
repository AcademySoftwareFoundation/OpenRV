//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkMovie__LeaderFooterMovie__h__
#define __TwkMovie__LeaderFooterMovie__h__
#include <TwkMovie/Movie.h>
#include <TwkMovie/dll_defs.h>

namespace TwkMovie
{

    /// A movie which plays a movie with a leader and footer movie

    ///
    /// A movie with a frame range and an optional leader and footer movie
    /// are played leader first, followed by the movie, followed by the
    /// footer.
    ///

    class TWKMOVIE_EXPORT LeaderFooterMovie : public Movie
    {
    public:
        //
        //  Types
        //

        typedef std::vector<Movie*> MovieVector;
        typedef TwkAudio::Time Time;

        //
        //  Constructors
        //

        LeaderFooterMovie(Movie*, int fs, int fe, Movie* leader = 0,
                          Movie* footer = 0);
        virtual ~LeaderFooterMovie();

        virtual void imagesAtFrame(const ReadRequest&, FrameBufferVector& fbs);

        virtual void identifiersAtFrame(const ReadRequest&, IdentifierVector&);

        virtual size_t audioFillBuffer(const AudioReadRequest&, AudioBuffer&);

        virtual void audioConfigure(const AudioConfiguration& conf);

    private:
        Movie* m_leader;
        Movie* m_movie;
        Movie* m_footer;
        int m_fs;
        int m_fe;
        size_t m_offset;
    };

} // namespace TwkMovie

#endif // __TwkMovie__LeaderFooterMovie__h__
