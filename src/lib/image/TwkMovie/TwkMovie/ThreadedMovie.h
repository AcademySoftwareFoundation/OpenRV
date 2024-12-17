//
//  Copyright (c) 2008 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __TwkMovie__ThreadedMovie__h__
#define __TwkMovie__ThreadedMovie__h__
#include <iostream>
#include <TwkMovie/Movie.h>
#include <TwkMovie/dll_defs.h>
#include <stl_ext/thread_group.h>
#include <map>

namespace TwkMovie
{

    /// Use multiple threads to parallel process over frames

    ///
    /// The expected order that frames will be asked for is determined on
    /// construction. One thread will be created for each input movie
    /// passed into the constructor. Typically, these are created by
    /// cloning one movie object or if possible duplicating its pointer.
    ///

    class TWKMOVIE_EXPORT ThreadedMovie : public Movie
    {
    public:
        struct ThreadData
        {
            ThreadData()
                : request(0)
                , movie(0)
                , threadedMovie(0)
                , id(0)
                , running(false)
                , init(false)
            {
            }

            ThreadedMovie* threadedMovie;
            Movie* movie;
            ReadRequest request;
            bool running;
            size_t id;
            bool init;
            pthread_t thread;
        };

        typedef std::vector<ThreadData> ThreadDataVector;
        typedef stl_ext::thread_group ThreadGroup;
        typedef TwkFB::FrameBuffer FrameBuffer;
        typedef std::map<int, FrameBufferVector> FBMap;
        typedef std::vector<ReadRequest> RequestVector;
        typedef std::vector<Movie*> Movies;
        typedef stl_ext::thread_group::thread_api ThreadAPI;
        typedef void (*InitializeFunc)();

        ThreadedMovie(const Movies&, const Frames& frames,
                      size_t stackMultiplier = 8, ThreadAPI* api = 0,
                      InitializeFunc = NULL);

        virtual ~ThreadedMovie();

        virtual void imagesAtFrame(const ReadRequest&, FrameBufferVector& fbs);
        virtual void identifiersAtFrame(const ReadRequest&, IdentifierVector&);
        virtual size_t audioFillBuffer(const AudioReadRequest&, AudioBuffer&);
        virtual void audioConfigure(unsigned int channels, TwkAudio::Time rate,
                                    size_t bufferSize);
        virtual void flush();
        virtual Movie* clone() const;

        virtual void threadMain();

        void dispatchAll();

    protected:
        void lock();
        void unlock();

        struct Lock
        {
            Lock(ThreadedMovie* m)
                : movie(m)
            {
                movie->lock();
            }

            ~Lock() { movie->unlock(); }

            ThreadedMovie* movie;
        };

        friend class ThreadedMovie::Lock;

    private:
        Movies m_movies;
        ThreadGroup m_threadGroup;
        FBMap m_map;
        pthread_mutex_t m_mapLock;
        pthread_mutex_t m_runLock;
        Frames m_frames;
        ThreadDataVector m_threadData;
        int m_currentIndex;
        int m_requestIndex;
        bool m_init;
        InitializeFunc m_initialize;
    };

} // namespace TwkMovie

#endif // __TwkMovie__ThreadedMovie__h__
