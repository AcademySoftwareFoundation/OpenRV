//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <TwkMovie/Movie.h>
#include <TwkExc/Exception.h>
#include <TwkFB/Exception.h>
#include <TwkMath/Color.h>
#include <TwkMath/Time.h>
#include <iomanip>
#include <ios>
#include <iostream>
#include <libswresample/swresample.h>
#include <pthread.h>
#include <stl_ext/stl_ext_algo.h>
#include <stl_ext/thread_group.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef _MSC_VER
#include <dlfcn.h>
#include <unistd.h>
#endif

namespace TwkMovie
{
    using namespace std;
    using namespace stl_ext;
    using namespace TwkMath;
    using namespace TwkAudio;

    Movie::Movie()
        : m_threadSafe(false)
        , m_audioAsync(true)
    {
    }

    Movie::~Movie() {}

    bool Movie::hasAudio() const { return info().audio; }

    bool Movie::hasVideo() const { return info().video; }

    void Movie::imagesAtFrame(const ReadRequest&, FrameBufferVector& fbs) {}

    void Movie::identifiersAtFrame(const ReadRequest&, IdentifierVector&) {}

    void Movie::attributesAtFrame(const ReadRequest&, FrameBufferVector&) {}

    void Movie::flush() {}

    size_t Movie::audioFillBuffer(const AudioReadRequest&, AudioBuffer&)
    {
        return 0;
    }

    bool Movie::canConvertAudioRate() const { return false; }

    bool Movie::canConvertAudioChannels() const { return false; }

    void Movie::audioConfigure(const AudioConfiguration&) {}

    Movie* Movie::clone() const
    {
        abort();
        return 0;
    }

    void Movie::cacheAudioForFrames(AudioBuffer& buffer, const Frames& frames)
    {
        //
        //  Take a list of (img) frames and assemble an audio track that
        //  corresponds to these
        //

        const unsigned int numChannels = m_info.audioChannels.size();
        const double rate = m_info.audioSampleRate;
        const double afpf = rate / double(m_info.fps);
        const size_t aframes = size_t(afpf * frames.size()) + 1;

        buffer.reconfigure(aframes, m_info.audioChannels, rate);

        Time t0 = m_info.start / Time(m_info.fps);

        float* ap = buffer.pointer();

        for (int i = 0; i < frames.size(); i++)
        {
            int f = frames[i];
            Time ts = double(f) / m_info.fps - t0;
            Time te = double(f + 1) / m_info.fps - t0;
            size_t start = size_t(ts * rate);
            size_t end = size_t(te * rate) - 1;
            size_t len = end - start;

            double startTime = double(start) / rate;
            double duration = double(len) / rate;

            AudioBuffer temp(ap, m_info.audioChannels, len, startTime, rate);
            AudioReadRequest req(startTime, duration);
            audioFillBuffer(req, temp);

            ap += len * numChannels;
        }
    }

    bool Movie::getBoolAttribute(const std::string& name) const
    {
        return false;
    }

    void Movie::setBoolAttribute(const std::string& name, bool value) {}

    int Movie::getIntAttribute(const std::string& name) const { return false; }

    void Movie::setIntAttribute(const std::string& name, int value) {}

    string Movie::getStringAttribute(const std::string& name) const
    {
        return "";
    }

    void Movie::setStringAttribute(const std::string& name,
                                   const std::string& value)
    {
    }

    double Movie::getDoubleAttribute(const std::string& name) const
    {
        return 0.0;
    }

    void Movie::setDoubleAttribute(const std::string& name, double value) const
    {
    }

#if 0
//
// ----------------------------------------------------------------------
//  NO longer used


void
Movie::resampleAudioCache(double inrate, double outrate)
{
    const unsigned int nc = m_audioCache.numChannels;
    typedef vector< vector<float> > BufferList;
    BufferList buffers(nc);

    deinterlace(m_audioCache.buffer, m_audioCache.numChannels, buffers);

    const size_t outsize = Resampler(outrate/inrate).outputSize(buffers[0].size());
    SampleVector temp;
    temp.resize(outsize);
    m_audioCache.buffer.resize(outsize * m_audioCache.numChannels);

    for (unsigned int c = 0; c < nc; c++)
    {
        Resampler resampler(outrate / inrate);
        resampler.process(&buffers[c].front(),
                          buffers[c].size(),
                          &temp.front(),
                          temp.size(),
                          true);

        for (float* t = &temp.front(),
                 *e = t + outsize,
                 *p = &m_audioCache.buffer.front() + c;
             t < e;
             t++, p+=nc)
        {
            *p = *t;
        }
    }

    m_audioCache.rate = outrate;
}

void 
Movie::resampleAudioToOutput(const AudioCache& in,
                             float* out,
                             unsigned int start,
                             unsigned int outNumSamples,
                             unsigned int outNumChannels,
                             double outrate)
{
    const double inrate = in.rate;

    typedef vector< vector<float> > BufferList;
    BufferList buffers(in.numChannels);
    deinterlace(in.buffer, in.numChannels, buffers);
         

//     double x = double(start) / outrate;

//     for (float* p = out, *e = out + (outNumSamples * outNumChannels); 
//          p < e;
//          p++)
//     {
//         const double v = sin(x * 3.141529 * 2.0 * 440.0);
//         *p = v; p++;
//         *p = v;
//         x += (1.0 / outrate);
//     }

    const size_t outsize = size_t(outrate / inrate * buffers[0].size());
    SampleVector temp;
    temp.resize(outsize);

    if (outNumChannels == in.numChannels)
    {
        for (unsigned int c = 0; c < outNumChannels; c++)
        {
            Resampler resampler(outrate / inrate);
            resampler.process(&buffers[c].front(),
                              buffers[c].size(),
                              &temp.front(),
                              temp.size(),
                              true);

            for (float* t = &temp.front(),
                     *e = t + min(outsize, size_t(outNumSamples)),
                     *p = out + c;
                 t < e;
                 t++, p += outNumChannels)
            {
                *p = *t;
            }
        }
    }
    else if (outNumChannels == 2 && buffers.size() == 1)
    {
        Resampler resampler(outrate / inrate);
        resampler.process(&buffers[0].front(),
                          buffers[0].size(),
                          &temp.front(),
                          temp.size(),
                          true);

        for (float* t = &temp.front(),
                 *e = t + min(outsize, size_t(outNumSamples)),
                 *p = out;
             t < e;
             t++, p++)
        {
            const float v = *t;
            *p = v; p++;
            *p = v;
        }
    }
    else
    {
        abort();
    }
}

#endif

} // namespace TwkMovie
