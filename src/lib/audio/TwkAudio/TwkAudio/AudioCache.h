//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkAudio__AudioCache__h__
#define __TwkAudio__AudioCache__h__
#include <TwkAudio/Audio.h>
#include <TwkAudio/AudioFormats.h>
#include <TwkAudio/dll_defs.h>
#include <map>
#include <pthread.h>
#include <utility>
#include <vector>

namespace TwkAudio
{

    /// Caches AudioBuffers of a fixed size

    ///
    /// AudioCache is used to implement look-ahead audio evaluation and
    /// general audio caching. Samples are stored as "packets" in the form
    /// of AudioBuffers. The packet size can be independant of the look-up
    /// packet size. For example, you can evaluate 1k packets to minimize
    /// file system hits and lookup 128k regions after the fact.
    ///

    class TWKAUDIO_EXPORT AudioCache
    {
    public:
        typedef float* Packet;
        typedef std::map<SampleTime, Packet> PacketMap;
        typedef std::vector<Packet> PacketVector;
        typedef std::pair<int, int> FrameRange;
        typedef std::vector<FrameRange> FrameRangeVector;

        struct AudioCacheLock
        {
            AudioCacheLock(AudioCache& c)
                : cache(c)
            {
                cache.lock();
            }

            ~AudioCacheLock() { cache.unlock(); }

            AudioCache& cache;
        };

        AudioCache();
        ~AudioCache();

        /// Get current packet size

        size_t packetSize() const { return m_packetSize; }

        Time rate() const { return m_packetRate; }

        TwkAudio::ChannelsVector channels() const
        {
            return TwkAudio::layoutChannels(m_packetLayout);
        }

        TwkAudio::Layout layout() const { return m_packetLayout; }

        /// Free all cached packets

        void clear();

        /// Free all packets before a certain time

        void clearBefore(Time);

        /// Free all packets after a certain time

        void clearAfter(Time);

        /// Free all packets after time0 and before time1

        void clear(Time time0, Time time1);

        /// A lock is requred before public API calls

        void lock() { pthread_mutex_lock(&m_lock); }

        /// Unlock previous lock

        void unlock() { pthread_mutex_unlock(&m_lock); }

        ///
        /// Set the packet size and rate. This will cause a clear() if the
        /// new parameters differs from the old. The default packet size
        /// is 512 with 2 channels. Packet size is samples (not floats).
        /// The default packet rate is TWEAK_AUDIO_DEFAULT_SAMPLE_RATE.
        ///

        void configurePacket(size_t samples, TwkAudio::Layout layout,
                             Time rate);

        ///
        /// The AudioBuffer configuration must match package
        /// configuration. The memory in the AudioBuffer will be
        /// copied. You MUST have locked the cache before calling this:
        /// the function will unlock the cache when it does the memory
        /// allocation and copy and relock when it inserts it.
        ///

        void add(const AudioBuffer&);

        ///
        /// Fills the AudioBuffer from the packet cache as much as
        /// possible. Returns true if the entire buffer was filled. The
        /// input buffer rate and channels should match the packet
        /// configuration, but the duration and start can be anything.
        ///

        bool fillBuffer(AudioBuffer&) const;

        ///
        /// If the packet exists at the sample time returns true. Note
        /// that SampleTime is sample offset so it could be negative.
        ///

        bool exists(SampleTime packet) const { return find(packet) != 0; }

        ///
        /// Returns the total number of seconds of cached audio
        ///

        Time totalSecondsCached() const { return m_totalSecondsCached; }

        ///
        /// Computes the cached range stat
        ///

        void computeCachedRangesStat(double frameRate, FrameRangeVector&);

    protected:
        SampleTime sampleAtTime(Time t) const
        {
            return timeToSamples(t, m_packetRate);
        }

        SampleTime packetOffset(SampleTime s) const { return s % m_packetSize; }

        const Packet find(SampleTime) const;

    private:
        size_t m_packetSize;
        TwkAudio::Layout m_packetLayout;
        Time m_packetRate;
        PacketMap m_map;
        int m_count;
        pthread_mutex_t m_lock;
        Time m_totalSecondsCached;
        PacketVector m_freePackets;
        FrameRangeVector m_cachedRangesStat;
        double m_cachedRangesStatFrameRate;
    };

} // namespace TwkAudio

#endif // __TwkAudio__AudioCache__h__
