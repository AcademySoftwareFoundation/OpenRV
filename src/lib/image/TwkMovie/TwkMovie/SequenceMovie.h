//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkMovie__SequenceMovie__h__
#define __TwkMovie__SequenceMovie__h__
#include <TwkMovie/Movie.h>
#include <TwkMovie/dll_defs.h>

namespace TwkMovie
{

    /// A movie which plays any number of input movies back-to-back

    ///
    /// SequenceMovie takes any number of input movies and plays them
    /// back-to-back starting at the first frame of each movie and ending
    /// with the last. The class has no EDL or other abilities. For that
    /// the IPGraph should be used.
    ///
    /// The total length of the sequence movie is the sum of its input
    /// movie lengths.
    ///
    /// Audio and Image requests are passed directly to the input movies.
    /// No reformating occurs (see ReformattingMovie)
    ///
    /// The FPS comes from the first movie
    ///

    class TWKMOVIE_EXPORT SequenceMovie : public Movie
    {
    public:
        //
        //  Types
        //

        typedef std::vector<Movie*> MovieVector;
        typedef TwkAudio::Time Time;

        struct InputMovie
        {
            InputMovie()
                : movie(0)
                , frames(0)
                , inStart(0)
                , inEnd(0)
            {
            }

            Movie* movie;
            int frames;
            int inStart;
            int inEnd;
            size_t startSample;
        };

        typedef std::vector<InputMovie> InputMovies;

        //
        //  Constructors
        //

        SequenceMovie(int fs);
        virtual ~SequenceMovie();

        void addMovie(Movie*, int fs, int fe);

        virtual void imagesAtFrame(const ReadRequest&, FrameBufferVector& fbs);

        virtual void identifiersAtFrame(const ReadRequest&, IdentifierVector&);

        virtual size_t audioFillBuffer(const AudioReadRequest&, AudioBuffer&);

        virtual void audioConfigure(const AudioConfiguration& conf);

        virtual Movie* clone() const;

    private:
        InputMovies m_movies;
        int m_fs;
    };

} // namespace TwkMovie

#endif // __TwkMovie__SequenceMovie__h__
