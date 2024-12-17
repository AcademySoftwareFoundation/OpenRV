//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkAudio/AudioCache.h>
#include <TwkUtil/MemPool.h>
#include <TwkUtil/Macros.h>
#include <iostream>
#include <assert.h>
#include <string.h>
#include <stl_ext/replace_alloc.h>

namespace TwkAudio
{
    using namespace std;

    AudioCache::AudioCache()
        : m_packetSize(512)
        , m_packetLayout(TwkAudio::Stereo_2)
        , m_count(0)
        , m_packetRate(0)
        , m_totalSecondsCached(0)
        , m_cachedRangesStatFrameRate(0.0)
    {
        pthread_mutex_init(&m_lock, 0);
    }

    AudioCache::~AudioCache()
    {
        clear();
        pthread_mutex_destroy(&m_lock);
    }

    void AudioCache::clear()
    {
        for (PacketMap::iterator i = m_map.begin(); i != m_map.end(); ++i)
        {
            // delete [] i->second;
            TwkUtil::MemPool::dealloc(i->second);
        }

        for (size_t i = 0; i < m_freePackets.size(); i++)
        {
            TwkUtil::MemPool::dealloc(m_freePackets[i]);
        }

        m_freePackets.clear();
        m_map.clear();
        m_count = 0;
        m_totalSecondsCached = 0;
        m_cachedRangesStat.clear();
    }

    void AudioCache::clearBefore(Time t)
    {
        size_t s = sampleAtTime(t);

        PacketMap::iterator a = m_map.end();
        PacketMap::iterator b = a;

        for (PacketMap::iterator i = m_map.begin(); i != m_map.end(); ++i)
        {
            if (i->first < s)
            {
                m_freePackets.push_back(i->second);
                m_count--;
                i->second = 0;
                if (a == m_map.end())
                    a = i;
            }
            else if (i->first >= s)
            {
                b = i;
                break;
            }
        }

        if (a != b && a != m_map.end())
        {
            m_map.erase(a, b);
            m_cachedRangesStat.clear();
        }

        m_totalSecondsCached =
            Time(m_count) * Time(m_packetSize) / m_packetRate;
    }

    void AudioCache::clearAfter(Time t)
    {
        SampleTime s = sampleAtTime(t);

        PacketMap::iterator a = m_map.end();
        PacketMap::iterator b = a;

        vector<Packet> tbd;

        for (PacketMap::iterator i = m_map.begin(); i != m_map.end(); ++i)
        {
            if (i->first > s)
            {
                m_freePackets.push_back(i->second);
                m_count--;
                i->second = 0;
                if (a == m_map.end())
                    a = i;
            }
        }

        if (a != b && a != m_map.end())
        {
            m_map.erase(a, b);
            m_cachedRangesStat.clear();
        }

        m_totalSecondsCached =
            Time(m_count) * Time(m_packetSize) / m_packetRate;
    }

    void AudioCache::clear(Time time0, Time time1)
    {
        SampleTime s0 = sampleAtTime(time0);
        SampleTime s1 = sampleAtTime(time1);

        PacketMap::iterator a = m_map.end();
        PacketMap::iterator b = a;

        vector<Packet> tbd;

        for (PacketMap::iterator i = m_map.begin(); i != m_map.end(); ++i)
        {
            if (i->first > s0)
            {
                if (i->first < s1)
                {
                    m_freePackets.push_back(i->second);
                    m_count--;
                    i->second = 0;
                    if (a == m_map.end())
                        a = i;
                }
                else
                {
                    b = i;
                    break;
                }
            }
        }

        if (a != b && a != m_map.end())
        {
            m_map.erase(a, b);
            m_cachedRangesStat.clear();
        }

        m_totalSecondsCached =
            Time(m_count) * Time(m_packetSize) / m_packetRate;
    }

    void AudioCache::configurePacket(size_t samples, TwkAudio::Layout layout,
                                     Time rate)
    {
        if (m_packetSize != samples || m_packetLayout != layout
            || m_packetRate != rate)
        {
            clear();
            m_packetSize = samples;
            m_packetLayout = layout;
            m_packetRate = rate;
        }
    }

    const AudioCache::Packet AudioCache::find(SampleTime s) const
    {
        s -= packetOffset(s);

        PacketMap::const_iterator i = m_map.find(s);

        if (i != m_map.end())
        {
            return i->second;
        }
        else
        {
            return 0;
        }
    }

    void AudioCache::add(const AudioBuffer& buffer)
    {
        Time t = buffer.startTime();
        SampleTime s = sampleAtTime(t);

        //
        // Since all samples in the cache are stored in packet size clumps we
        // must make sure that this sample set starts exactly on a packet
        // boundary, is the right amount of samples and channels, and is at the
        // correct sampling rate.
        //

        if (packetOffset(s) != 0 || buffer.size() != m_packetSize
            || buffer.channels() != channels() || buffer.rate() != m_packetRate)
        {
            cout << "WARNING: s = " << s
                 << ", packetOffset = " << packetOffset(s)
                 << ", m_packetSize = " << m_packetSize
                 << ", size = " << buffer.size() << ", m_packetLayout = "
                 << TwkAudio::layoutString(m_packetLayout)
                 << ", channels = " << buffer.numChannels()
                 << ", m_packetRate = " << m_packetRate
                 << ", rate: " << buffer.rate() << endl;
        }
        assert(packetOffset(s) == 0);
        assert(buffer.size() == m_packetSize);
        assert(buffer.channels() == channels());
        assert(buffer.rate() == m_packetRate);

        if (!find(s))
        {
            unlock();
            Packet p = 0;

            if (m_freePackets.empty())
            {
                int numChannels = TwkAudio::channelsCount(m_packetLayout);
                p = (float*)TwkUtil::MemPool::alloc(sizeof(float) * m_packetSize
                                                    * numChannels);
            }
            else
            {
                p = m_freePackets.back();
                m_freePackets.pop_back();
            }

            memcpy(p, buffer.pointer(), buffer.sizeInBytes());
            lock();

            m_map[s] = p;
            m_count++;
            m_totalSecondsCached =
                Time(m_count) * Time(m_packetSize) / m_packetRate;
            m_cachedRangesStat.clear();
        }
        else
        {
            cout << "dup @ " << s << endl;
        }
    }

    bool AudioCache::fillBuffer(AudioBuffer& buffer) const
    {
        const SampleTime s0 = sampleAtTime(buffer.startTime());
        const SampleTime d0 = sampleAtTime(buffer.duration());
        const SampleTime e0 = s0 + d0 - 1;
        int numChannels = TwkAudio::channelsCount(m_packetLayout);

        // assert(d0 == m_packetSize);

        if (d0 <= m_packetSize)
        {
            const Packet p0 = find(s0);
            const Packet p1 = find(e0);

            if (p0 && p0 == p1)
            {
                //
                //  This should be the most common case: the buffer range is
                //  completely contained inside one packet.
                //

                memcpy(buffer.pointer(), p0 + packetOffset(s0) * numChannels,
                       buffer.sizeInBytes());

                return true;
            }
            else if (p0 && p1)
            {
                //
                //  Straddles two packets
                //

                const SampleTime n0 =
                    SampleTime(m_packetSize) - packetOffset(s0);
                const SampleTime n1 = packetOffset(e0) + 1;

                memcpy(buffer.pointer(), p0 + packetOffset(s0) * numChannels,
                       n0 * sizeof(float) * numChannels);

                memcpy(buffer.pointer() + n0 * numChannels, p1,
                       n1 * sizeof(float) * numChannels);

                return true;
            }
        }
        else
        {
            const SampleTime total = buffer.size();
            const SampleTime n = total / SampleTime(m_packetSize);
            const SampleTime r = total % SampleTime(m_packetSize);

            for (size_t i = 0; i < n; i++)
            {
                size_t offset = i * m_packetSize;

                AudioBuffer newBuffer(
                    buffer.pointer() + offset * buffer.numChannels(),
                    buffer.channels(), m_packetSize,
                    buffer.startTime()
                        + double(i * m_packetSize) / m_packetRate,
                    m_packetRate);

                if (!fillBuffer(newBuffer))
                    return false;
            }

            if (r)
            {
                SampleTime offset = n * SampleTime(m_packetSize);

                AudioBuffer newBuffer(
                    buffer.pointer() + offset * buffer.numChannels(),
                    buffer.channels(), r,
                    buffer.startTime()
                        + double(n * m_packetSize) / m_packetRate,
                    m_packetRate);

                if (!fillBuffer(newBuffer))
                    return false;
            }

            return true;
        }

        return false;
    }

    //------------------------------------------------------------------------------
    //
    void AudioCache::computeCachedRangesStat(double frameRate,
                                             FrameRangeVector& array)
    {
        lock();

        if (frameRate != m_cachedRangesStatFrameRate)
        {
            m_cachedRangesStatFrameRate = frameRate;
            m_cachedRangesStat.clear();
        }

        if (m_cachedRangesStat.empty())
        {
            for (PacketMap::iterator i = m_map.begin(); i != m_map.end(); ++i)
            {
                const SampleTime s = i->first;
                const Time t = samplesToTime(s, m_packetRate);
                const Time duration = Time(m_packetSize) / m_packetRate;
                const int startFrame = ROUND(t * frameRate);
                const int endFrame = ROUND((t + duration) * frameRate);

                m_cachedRangesStat.push_back(make_pair(startFrame, endFrame));
            }
        }

        array = m_cachedRangesStat;

        unlock();
    }

} // namespace TwkAudio
