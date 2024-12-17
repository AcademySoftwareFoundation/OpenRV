//******************************************************************************
// Copyright (c) 2001-2005 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef __DEVAUDIORENDERER_H__
#define __DEVAUDIORENDERER_H__

#include <stl_ext/thread_group.h>
#include <TwkMovie/Movie.h>
#include <IPCore/AudioRenderer.h>
#include <OSSDriver/OSSDriver.h>

namespace IPCore
{

    class DevAudioRenderer : public AudioRenderer
    {
    public:
        DevAudioRenderer();
        ~DevAudioRenderer();

        //
        //  play() will return almost immediately -- a worker thread will
        //  be released and start playing.
        //

        virtual void play();

        //
        //  isPlaying() is only true after the first samples have been
        //  retrieved.
        //

        virtual bool isPlaying() const;

        //
        //  stop() will cause the worker thread to wait until play.
        //

        virtual void stop();

        //
        //  Calculated as samples that have played
        //

        virtual double elapsed();

        //
        //  Skip to a specified time
        //

        virtual void setTime(double);

        void playerThread();

    private:
        void playAudioChunk();

    private:
        bool m_isPlaying;
        stl_ext::thread_group m_threads;
        const TwkMovie::MovieInfo* m_movInfo;
        unsigned int m_startSample;
        unsigned int m_beginSample;
        bool m_firstTime;
        bool m_running;
        float m_deviceRate;
        unsigned int m_sampleShift;
        double m_elapsedSincePlay;
        OSSDriver* m_audioDev;
    };

} // namespace IPCore

#endif // End #ifdef __DEVAUDIORENDERER_H__
