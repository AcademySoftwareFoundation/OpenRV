//******************************************************************************
// Copyright (c) 2001-2005 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <iostream>
#include <TwkApp/Document.h>
#include <TwkAudio/Audio.h>
#include <IPCore/Session.h>
#include <IPCore/SourceIPNode.h>
#include <IPCore/DevAudioRenderer.h>

// MUST be evenly divisable by 2
#define AUDIO_CHUNK_SIZE 1024

namespace IPCore
{
    using namespace TwkApp;
    using namespace std;

    DevAudioRenderer::DevAudioRenderer()
        : m_isPlaying(false)
        , m_firstTime(true)
        , m_audioDev(NULL)
        , m_movInfo(NULL)
        , m_threads(1)
    {
        Session* session = Session::activeSession();
        const Session::Sources& sources = session->sources();
        m_movInfo = &sources.front()->movie()->info();

        m_audioDev = new OSSDriver(OSSDriver::S16_LE, 2, OSSDriver::play,
                                   TWEAK_AUDIO_DEFAULT_SAMPLE_RATE);
    }

    DevAudioRenderer::~DevAudioRenderer()
    {
        if (m_isPlaying)
            stop();

        delete m_audioDev;
        m_audioDev = NULL;
    }

    static void staticPlayerThread(void* darPtr)
    {
        DevAudioRenderer* dar = (DevAudioRenderer*)darPtr;
        dar->playerThread();
    }

    bool DevAudioRenderer::isPlaying() const
    {
        return (m_isPlaying == true) && (!m_firstTime);
    }

    void DevAudioRenderer::play()
    {
        if (m_isPlaying)
            return;
        m_timeShift = 0.0;
        m_startSample = 0;
        m_firstTime = true;
        m_elapsedSincePlay = 0.0;

        try
        {
            m_audioDev->open();
            m_audioDev->initialize();
            // m_audioDev->setSampleRate((int)m_movInfo->audioSampleRate);
            // cout << "movInfo audioRate = " << m_movInfo->audioSampleRate <<
            // endl;
            m_audioDev->setSampleRate(TWEAK_AUDIO_DEFAULT_SAMPLE_RATE);
            m_deviceRate = (float)m_audioDev->getSampleRate();

            // if (m_deviceRate != m_movInfo->audioSampleRate)
            if (m_deviceRate != TWEAK_AUDIO_DEFAULT_SAMPLE_RATE)
            {
                std::cerr
                    << "WARNING: audio device rate != movie sample rate\n";
            }

            m_isPlaying = true;
            m_threads.dispatch(staticPlayerThread, this);
        }
        catch (const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }
    }

    void DevAudioRenderer::stop()
    {
        if (!m_isPlaying)
            return;

        try
        {
            m_isPlaying = false;
            m_audioDev->reset();
            m_threads.control_wait();
            m_audioDev->close();
            m_timeShift = 0.0;
        }
        catch (const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }
    }

    double DevAudioRenderer::elapsed() { return m_elapsedSincePlay; }

    void DevAudioRenderer::playerThread()
    {
        Session* session = Session::activeSession();

        try
        {
            size_t t = size_t(double(session->shift()) / session->fps()
                              * m_deviceRate);
            m_startSample = t;
            m_sampleShift = t;
            m_firstTime = false;
            m_beginSample = 0;

            while (m_isPlaying)
            {
                while (m_audioDev->willBlock(AUDIO_CHUNK_SIZE) && m_isPlaying)
                {
                    usleep(10);
                }

                const double e = session->timer().elapsed();
                const int od = m_audioDev->getODelay();
                const int psamps = m_startSample - od / 4;
                const double ae =
                    double(psamps - m_sampleShift) / double(m_deviceRate);

                m_timeShift = ae - e;

#if 0
            cout << "m_timeShift = " << m_timeShift 
                 << " od = " << od
                 << " psamps = " << psamps
                 << " ae = " << ae
                 << " e = " << e
                 << endl;
#endif

                playAudioChunk();
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "DevAudioRenderer::playerThread: " << e.what()
                      << std::endl;
        }
    }

    void DevAudioRenderer::playAudioChunk()
    {
        typedef signed short int16;
        Session* session = Session::activeSession();

        const int numSamples = AUDIO_CHUNK_SIZE / 2;

        static float fout[AUDIO_CHUNK_SIZE];
        static int16 sout[AUDIO_CHUNK_SIZE];

        const Session::Sources& sources = session->sources();

        unsigned int n = session->rootNode()->audioFillBuffer(
            fout, m_deviceRate, m_startSample, numSamples);

        if (n != numSamples)
        {
            memset(fout + n * 2, 0, (numSamples - n) * 2 * sizeof(float));
        }

        //
        //  Convert to 16 bit int and copy
        //

        const float* f = fout;
        const int16* e = sout + numSamples * 2;

        for (int16* p = sout; p != e; p++, f++)
        {
            *p = int16(*f * 32768.0f);
        }

        size_t aframeSize = sizeof(int16) * 2;
        m_startSample += m_audioDev->write(sout, numSamples) / aframeSize;
        m_elapsedSincePlay += double(numSamples) / double(m_deviceRate);
    }

    void DevAudioRenderer::setTime(double t)
    {
        m_startSample = (unsigned int)(t * double(m_deviceRate));
    }

} //  End namespace IPCore
