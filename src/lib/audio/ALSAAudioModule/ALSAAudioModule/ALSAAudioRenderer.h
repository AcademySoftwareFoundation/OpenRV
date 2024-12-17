//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__ALSAAudioRenderer__h__
#define __IPCore__ALSAAudioRenderer__h__
#include <IPCore/AudioRenderer.h>
#include <TwkAudio/Audio.h>
#include <stl_ext/thread_group.h>
#include <TwkUtil/Timer.h>

#include <alsa/asoundlib.h>

namespace IPCore
{

    /// Pure ALSA implementation of AudioRenderer

    ///
    ///
    ///

    class ALSAAudioRenderer : public AudioRenderer
    {
    public:
        //
        //  Types
        //

        struct ALSADevice
        {
            std::string plug;
            std::string card;
            std::string name;
        };

        typedef std::vector<ALSADevice> ALSADeviceVector;
        typedef TwkAudio::AudioBuffer AudioBuffer;
        typedef stl_ext::thread_group ThreadGroup;
        typedef std::vector<unsigned char> OutputSampleBuffer;
        typedef TwkUtil::Timer Timer;

        ALSAAudioRenderer(const RendererParameters&);
        ~ALSAAudioRenderer();

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

        ///
        /// These are currently  ignored by ALSAAudioRenderer
        ///

        virtual void availableLayouts(const Device&, LayoutsVector&);
        virtual void availableFormats(const Device&, FormatVector&);
        virtual void availableRates(const Device&, Format, RateVector&);

        void threadMain();
        void configureDevice();

    protected:
        virtual void createDeviceList();

    private:
        OutputSampleBuffer m_outBuffer;
        ALSADeviceVector m_alsaDevices;
        ThreadGroup m_threadGroup;
        size_t m_startSample;
        snd_pcm_t* m_pcm;
        snd_pcm_uframes_t m_periodSize;
        snd_pcm_uframes_t m_bufferSize;
        pthread_t m_audioThread;
        bool m_audioThreadRunning;
        pthread_mutex_t m_runningLock;
    };

} // namespace IPCore

#endif // __IPCore__ALSAAudioRenderer__h__
