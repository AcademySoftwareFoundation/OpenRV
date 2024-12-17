//******************************************************************************
// Copyright (c) 2011 Tweak Software.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__PerFrameAudioRenderer__h__
#define __IPCore__PerFrameAudioRenderer__h__
#include <IPCore/AudioRenderer.h>
#include <TwkAudio/Audio.h>
#include <stl_ext/thread_group.h>
#include <TwkUtil/Timer.h>

namespace IPCore
{

    /// PerFrame implementation of AudioRenderer

    ///
    /// Packages up audio in per-video-frame chunks for sending off to a
    /// remote device. Used for e.g. SDI output to specific SMPTE audio
    /// standards as per-frame data.
    ///

    class PerFrameAudioRenderer : public AudioRenderer
    {
    public:
        typedef stl_ext::thread_group ThreadGroup;
        typedef TwkUtil::Timer Timer;
        typedef std::vector<unsigned char> OutputSampleBuffer;
        typedef std::vector<OutputSampleBuffer> OutputSampleBufferVector;
        typedef std::vector<size_t> AudioFrameSizeVector;

        struct FrameData
        {
            FrameData(void* d, size_t n)
                : data(d)
                , numSamples(n)
            {
            }

            void* data;
            size_t numSamples;
        };

        PerFrameAudioRenderer(const RendererParameters&);
        ~PerFrameAudioRenderer();

        ///
        /// PerFrameAudioRenderer API
        ///

        FrameData
        dataForFrame(int f, size_t seqindex,
                     bool silence = false); // f is relative to startFrame

        ///
        ///  play() will return almost immediately -- a worker thread will
        ///  be released and start playing.
        ///

        virtual void play();
        virtual void play(Session*);

        ///
        ///  stop() will cause the worker thread to wait until play.  A
        ///  subsequent play should restart it.
        ///

        virtual void stop(Session*);

        ///
        ///  Completely stop the audio hardware and the audio thread. You can't
        ///  call play() after you've called shutdown()
        ///

        virtual void shutdown();

        void threadMain();

    private:
        OutputSampleBuffer m_outBuffer;
        AudioFrameSizeVector m_audioFrameSizes;
        size_t m_totalAudioFrameSizes;
        ThreadGroup m_threadGroup;
        size_t m_startSample;
        pthread_t m_audioThread;
        bool m_audioThreadRunning;
        pthread_mutex_t m_runningLock;
    };

} // namespace IPCore

#endif // __IPCore__PerFrameAudioRenderer__h__
