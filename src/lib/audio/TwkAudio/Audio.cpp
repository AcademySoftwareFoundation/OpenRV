//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkAudio/Audio.h>
#include <iostream>
#include <algorithm>
#include <limits>

#include <string.h>
#include <math.h>
#include <assert.h>

namespace TwkAudio
{
    using namespace std;

    AudioBuffer::AudioBuffer()
        : m_rate(0)
        , m_startTime(0)
        , m_numSamples(0)
        , m_data(0)
        , m_margin(0)
    {
    }

    AudioBuffer::AudioBuffer(size_t numSamples, ChannelsVector channels,
                             Time rate, Time startTime, size_t margin)
        : m_rate(rate)
        , m_startTime(startTime)
        , m_numSamples(numSamples)
        , m_data(0)
        , m_margin(margin)
        , m_channels(channels)
    {
        m_buffer.resize((m_numSamples + 2 * m_margin) * m_channels.size());
        m_data =
            numSamples ? (&m_buffer.front() + margin * m_channels.size()) : 0;
    }

    AudioBuffer::AudioBuffer(size_t numSamples, TwkAudio::Layout layout,
                             Time rate, Time startTime, size_t margin)
        : m_rate(rate)
        , m_startTime(startTime)
        , m_numSamples(numSamples)
        , m_data(0)
        , m_margin(margin)
        , m_channels(TwkAudio::layoutChannels(layout))
    {
        m_buffer.resize((m_numSamples + 2 * m_margin) * m_channels.size());
        m_data =
            numSamples ? (&m_buffer.front() + margin * m_channels.size()) : 0;
    }

    AudioBuffer::AudioBuffer(Time start, Time duration, Time rate,
                             ChannelsVector channels, size_t margin)
        : m_startTime(start)
        , m_rate(rate)
        , m_numSamples(size_t(duration * rate + 0.49))
        , m_margin(margin)
        , m_channels(channels)
    {
        m_buffer.resize((m_numSamples + 2 * m_margin) * m_channels.size());
        m_data =
            m_numSamples ? (&m_buffer.front() + margin * m_channels.size()) : 0;
    }

    AudioBuffer::AudioBuffer(BufferPointer externalMemory,
                             ChannelsVector channels, size_t numSamples,
                             Time start, Time rate, size_t margin)
        : m_startTime(start)
        , m_rate(rate)
        , m_data(externalMemory + margin)
        , m_numSamples(numSamples)
        , m_margin(margin)
        , m_channels(channels)
    {
    }

    AudioBuffer::AudioBuffer(AudioBuffer& inbuffer, size_t startOffsetSample,
                             size_t numSamples, Time startTime)
    {
        m_channels = inbuffer.channels();
        m_numSamples = numSamples;
        m_margin = 0;
        m_rate = inbuffer.rate();
        m_startTime = startTime;
        m_data =
            inbuffer.size()
                ? (inbuffer.pointer() + startOffsetSample * m_channels.size())
                : 0;
    }

    void AudioBuffer::ownData()
    {
        if (!bufferOwnsData())
        {
            m_buffer.resize((m_numSamples + 2 * m_margin) * m_channels.size());

            if (!m_buffer.empty())
            {
                memcpy(&m_buffer.front(), m_data - m_margin,
                       sizeof(float) * m_channels.size()
                           * (m_numSamples + 2 * m_margin));

                m_data = &m_buffer.front() + m_margin * m_channels.size();
            }
            else
            {
                m_data = 0;
            }

            m_numSamples = m_buffer.size() / m_channels.size() - 2 * m_margin;
        }
    }

    void AudioBuffer::reconfigure(size_t numSamples, ChannelsVector channels,
                                  Time rate, Time startTime, size_t margin)
    {
        ownData();
        m_buffer.resize((numSamples + 2 * margin) * channels.size());

        m_channels = channels;
        m_numSamples = numSamples;
        m_rate = rate;
        m_startTime = startTime;
        m_margin = margin;
        m_data =
            numSamples ? (&m_buffer.front() + margin * m_channels.size()) : 0;
    }

    void AudioBuffer::reconfigure(size_t numSamples, TwkAudio::Layout layout,
                                  Time rate, Time startTime, size_t margin)
    {
        return reconfigure(numSamples, layoutChannels(layout), rate, startTime,
                           margin);
    }

    void AudioBuffer::zero()
    {
        if (m_data)
            memset(m_data - m_margin * m_channels.size(), 0,
                   sizeInBytesIncludingMargin());
    }

    void AudioBuffer::zeroRegion(size_t start, size_t n)
    {
        if (!m_data)
            return;
        assert(n <= (m_numSamples - start));

        size_t a = (m_margin + start) * m_channels.size();
        memset(m_data + a, 0, n * m_channels.size() * sizeof(SampleType));
    }

    bool AudioBuffer::bufferOwnsData() const
    {
        if (m_buffer.empty())
        {
            return m_data == 0;
        }
        else
        {
            return m_data == (&m_buffer.front() + m_channels.size() * m_margin);
        }
    }

    void AudioBuffer::reverse(bool reverseChannels)
    {
        float* data = m_data;

        if (reverseChannels)
        {
            std::reverse(data, data + m_numSamples * m_channels.size());
            std::reverse(m_channels.begin(), m_channels.end());
        }
        else
        {
            size_t halfNumSamples = m_numSamples / 2;

            for (size_t i = 0; i < halfNumSamples; ++i)
            {
                float* out0 = data + i * m_channels.size();
                float* out1 = data + (m_numSamples - 1 - i) * m_channels.size();

                for (size_t j = 0; j < m_channels.size(); ++j)
                {
                    float tmp = out0[j];
                    out0[j] = out1[j];
                    out1[j] = tmp;
                }
            }
        }
    }

    bool AudioBuffer::checkBuffer(const char* label) const
    {
        size_t zeroCount = 0;
        size_t outOfBoundsCount = 0;
        size_t maxCount = 0;
        size_t gapCount = 0;
        const float* data = m_data;
        const size_t numOfFloats = m_numSamples * m_channels.size();

        float maxValue = -std::numeric_limits<float>::max();
        float minValue = std::numeric_limits<float>::max();

        for (size_t i = 0; i < numOfFloats; ++i)
        {
            if (data[i] == 0.0f)
            {
                if (zeroCount == 0)
                {
                    ++gapCount;
                }

                ++zeroCount;
            }
            else
            {
                if (zeroCount > maxCount)
                {
                    maxCount = zeroCount;
                }
                zeroCount = 0;
            }

            if ((data[i] > 1.0f) || (data[i] < -1.0f))
            {
                if (data[i] > maxValue)
                {
                    maxValue = data[i];
                }
                else if (data[i] < minValue)
                {
                    minValue = data[i];
                }

                ++outOfBoundsCount;
            }
        }

        if (maxCount > 1 && m_startTime != 0.0)
        {
            cerr << "Time=" << m_startTime
                 << " : AudioBuffer::check() called from " << label << ": "
                 << "zeroCount=" << maxCount << " gapCount=" << gapCount
                 << endl;

            return false;
        }

        if (outOfBoundsCount)
        {
            cerr << "Time=" << m_startTime
                 << " : AudioBuffer::check() called from " << label << ": "
                 << "outOfBoundsCount=" << outOfBoundsCount
                 << "maxValue=" << maxValue << "minValue=" << minValue << endl;

            return false;
        }

        return true;
    }

} // namespace TwkAudio
