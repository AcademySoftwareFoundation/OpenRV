//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IPCore/PerFrameAudioRenderer.h>
#include <IPCore/Session.h>
#include <TwkAudio/Audio.h>
#include <TwkMovie/Movie.h>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <sched.h>
#include <sstream>
#include <stl_ext/string_algo.h>

#ifdef PLATFORM_LINUX
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif

namespace IPCore
{
    using namespace std;
    using namespace stl_ext;
    using namespace TwkApp;
    using namespace TwkAudio;

    PerFrameAudioRenderer::PerFrameAudioRenderer(const RendererParameters& p)
        : AudioRenderer(p)
        , m_threadGroup(1, 2)
        , m_audioThread(pthread_self())
        , m_audioThreadRunning(false)
    {
        pthread_mutex_init(&m_runningLock, 0);

        if (m_parameters.rate == 0)
            m_parameters.rate = 48000;
        if (m_parameters.framesPerBuffer == 0)
            m_parameters.framesPerBuffer = 512;
        if (m_parameters.device == "")
            m_parameters.device = "default";

        if (debug)
            outputParameters(m_parameters);
    }

    PerFrameAudioRenderer::~PerFrameAudioRenderer()
    {
        AudioRenderer::stop();
        shutdown();
        pthread_mutex_destroy(&m_runningLock);
    }

    static void trampoline(void* data)
    {
        PerFrameAudioRenderer* a = (PerFrameAudioRenderer*)data;
        a->threadMain();
    }

    void PerFrameAudioRenderer::threadMain()
    {
        // nothing
    }

    void PerFrameAudioRenderer::play(Session* s)
    {
        DeviceState nstate;
        nstate.device = m_parameters.device;
        nstate.format = m_parameters.format;
        nstate.rate = m_parameters.rate;
        nstate.layout = m_parameters.layout;
        nstate.framesPerBuffer = m_parameters.framesPerBuffer;
        nstate.frameSizes = m_parameters.frameSizes;
        nstate.latency = m_parameters.latency;
        setDeviceState(nstate);

        const DeviceState& state = deviceState();

        m_totalAudioFrameSizes = accumulate(nstate.frameSizes.begin(),
                                            nstate.frameSizes.end(), size_t(0));

        m_abuffer.reconfigure(state.framesPerBuffer, state.layout,
                              Time(state.rate));

        AudioRenderer::play(s);

        s->audioVarLock();
        s->setAudioTimeShift(numeric_limits<double>::max());
        s->setAudioStartSample(0);
        s->setAudioFirstPass(true);
        s->audioVarUnLock();

        s->audioConfigure();

        if (!isPlaying())
            play();
    }

    void PerFrameAudioRenderer::play()
    {
        AudioRenderer::play();

        //
        //  Dispatch the playback thread if its not running already. The
        //  last argument (async=false) indicates that we want to wait
        //  until the thread is running before the function returns.
        //

        // m_threadGroup.dispatch(trampoline, this);
    }

    void PerFrameAudioRenderer::stop(Session* s)
    {
        AudioRenderer::stop(s);
        s->setAudioTimeShift(numeric_limits<double>::max());
        if (!m_parameters.holdOpen)
            shutdown();
    }

    void PerFrameAudioRenderer::shutdown()
    {
        //
        //  Shut down the hardware
        //

        const bool holdOpen = m_parameters.holdOpen;
        m_parameters.holdOpen = false;

        AudioRenderer::stop();

        // m_threadGroup.lock(m_runningLock);
        // bool isrunning = m_audioThreadRunning;
        // m_threadGroup.unlock(m_runningLock);

        // if (isrunning) m_threadGroup.control_wait(true, 1.0);
        m_parameters.holdOpen = holdOpen;
    }

    PerFrameAudioRenderer::FrameData
    PerFrameAudioRenderer::dataForFrame(int f, size_t seqIndex, bool silence)
    {
        const DeviceState& state = deviceState();

        if (state.frameSizes.empty() || state.layout == TwkAudio::UnknownLayout)
        {
            return FrameData(0, 0);
        }

        const int numChannels = TwkAudio::channelsCount(state.layout);

        size_t nsequences = f / state.frameSizes.size();
        size_t nframes = state.frameSizes[seqIndex];

        size_t start =
            nsequences * m_totalAudioFrameSizes
            + accumulate(state.frameSizes.begin(),
                         state.frameSizes.begin() + seqIndex, size_t(0));

        AudioBuffer buffer(nframes, state.layout, state.rate,
                           samplesToTime(start, state.rate));

        const size_t formatSize = formatSizeInBytes(state.format);
        m_outBuffer.resize(nframes * numChannels * formatSize);

        buffer.zero();

        //
        //  Fetch the samples
        //

        if (!silence)
        {
            audioFillBuffer(buffer);
        }

        //
        //  Convert to the correct output format
        //

        unsigned char* out = &m_outBuffer.front();

        switch (m_parameters.format)
        {
        case Float32Format:
            memcpy(out, buffer.pointer(), buffer.sizeInBytes());
            break;

        case Int32Format:
            transform(buffer.pointer(),
                      buffer.pointer() + buffer.sizeInFloats(), (int*)out,
                      toType<int>);
            break;

        case Int24Format:
            AudioRenderer::transformFloat32ToInt24(buffer.pointer(), (char*)out,
                                                   buffer.sizeInFloats());
            break;

        case Int16Format:
            transform(buffer.pointer(),
                      buffer.pointer() + buffer.sizeInFloats(), (short*)out,
                      toType<short>);
            break;

        case Int8Format:
            transform(buffer.pointer(),
                      buffer.pointer() + buffer.sizeInFloats(),
                      (signed char*)out, toType<signed char>);
            break;

        case UInt32_SMPTE272M_20Format:
            transform(buffer.pointer(),
                      buffer.pointer() + buffer.sizeInFloats(), (UInt32*)out,
                      toUInt32_SMPTE272M_20_transformer());
            break;

        case UInt32_SMPTE299M_24Format:
            transform(buffer.pointer(),
                      buffer.pointer() + buffer.sizeInFloats(), (UInt32*)out,
                      toUInt32_SMPTE299M_24_transformer());
            break;

        case UnknownFormat:
            // Warning: Unknown audio format detected.
            break;
        }

        // m_serialCount++;

        return FrameData(out, nframes);
    }

} // namespace IPCore
