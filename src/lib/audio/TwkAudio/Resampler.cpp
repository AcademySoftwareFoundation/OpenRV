//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <TwkAudio/Audio.h>
#include <TwkAudio/Resampler.h>
#include <TwkAudio/Interlace.h>
#include <libresample.h>
#include <algorithm>
#include <iostream>
#include <math.h>
#include <assert.h>

namespace TwkAudio
{
    using namespace std;

    Resampler::Resampler(double factor, size_t blocksize)
        : m_factor(factor)
        , m_handle(0)
        , m_filterwidth(0)
        , m_blocksize(blocksize)
    {
        open();
    }

    Resampler::~Resampler() { close(); }

    size_t Resampler::process(const float* in, size_t inSize, float* out,
                              size_t outSize, bool endFlag, bool enableClamp)
    {
        int inUsed = 0;
        int inUsedTotal = 0;
        int outUsedTotal = 0;

        for (int i = 0; true; i++)
        {
            size_t left = inSize - inUsedTotal;
            int bsize = std::min(left, m_blocksize);

            size_t outUsed =
                resample_process(m_handle, m_factor, (float*)(in + inUsedTotal),
                                 bsize, endFlag && left < m_blocksize, &inUsed,
                                 out + outUsedTotal, outSize - outUsedTotal);

            inUsedTotal += inUsed;

            if (outUsed > 0)
            {
                outUsedTotal += outUsed;
            }
            else if (outUsed <= 0 || (outUsed == 0 && inUsedTotal >= inSize))
            {
                break;
            }
        }

        if (enableClamp)
        {
            float* data = out;
            for (int i = 0; i < outUsedTotal; ++i, data++)
            {
                if ((*data) > 1.0f)
                    (*data) = 1.0f;
                else if ((*data) < -1.0f)
                    (*data) = -1.0f;
            }
        }

        return outUsedTotal;
    }

    void Resampler::process(const SampleVector& in, SampleVector& out,
                            bool endFlag, bool enableClamp)
    {
        size_t n = size_t(double(in.size()) * m_factor);
        out.resize(n);
        process(&in.front(), in.size(), &out.front(), out.size(), endFlag,
                enableClamp);
    }

    void Resampler::reset() { resample_reset(m_handle); }

    void Resampler::open()
    {
        m_handle = resample_open(1, m_factor, m_factor);
        m_filterwidth = resample_get_filter_width(m_handle);
    }

    void Resampler::close()
    {
        if (m_handle)
        {
            resample_close(m_handle);
        }

        m_handle = 0;
        m_filterwidth = 0;
    }

    //----------------------------------------------------------------------

    MultiResampler::MultiResampler(int numChannels, double factor,
                                   size_t blocksize)
        : m_inbuffers(numChannels)
        , m_outbuffers(numChannels)
    {
        for (int ch = 0; ch < numChannels; ch++)
        {
            m_resamplers.push_back(new Resampler(factor, blocksize));
        }
    }

    MultiResampler::~MultiResampler()
    {
        for (int ch = 0; ch < m_resamplers.size(); ch++)
        {
            delete m_resamplers[ch];
            m_resamplers[ch] = 0;
        }
        m_resamplers.clear();
    }

    void MultiResampler::close()
    {
        for (int ch = 0; ch < m_resamplers.size(); ch++)
        {
            m_resamplers[ch]->close();
        }
    }

    size_t MultiResampler::process(const float* in, size_t inSize, float* out,
                                   size_t outSize, bool endFlag,
                                   bool enableClamp)
    {
        deinterlace(in, inSize, m_resamplers.size(), m_inbuffers);

        size_t n = size_t(double(inSize) * factor() + 0.49);
        size_t bsize = max(n, m_outbuffers.front().size());

        if (bsize != m_outbuffers.front().size())
        {
            for (int ch = 0; ch < m_resamplers.size(); ch++)
            {
                m_outbuffers[ch].resize(bsize);
            }
        }

        size_t samples = 0;
        for (int ch = 0; ch < m_resamplers.size(); ch++)
        {
            size_t nsamps = m_resamplers[ch]->process(
                &(m_inbuffers[ch].front()), inSize, &(m_outbuffers[ch].front()),
                outSize, endFlag, enableClamp);
#ifndef NDEBUG
            if (samples != 0 && samples != nsamps)
            {
                cout << "WARNING: resampler samples missmatch" << endl;
            }
#endif
            samples = nsamps;
        }

        interlace(m_outbuffers, out, 0, min(n, outSize));

        return samples;
    }

    void MultiResampler::reset()
    {
        for (int ch = 0; ch < m_resamplers.size(); ch++)
        {
            m_resamplers[ch]->reset();
        }
    }

} // namespace TwkAudio
