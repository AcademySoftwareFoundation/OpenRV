//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkAudio__Audio__h__
#define __TwkAudio__Audio__h__

#include <limits>
#include <vector>
#include <stdlib.h>
#include <stl_ext/replace_alloc.h>
#include <TwkAudio/AudioFormats.h>
#include <TwkAudio/dll_defs.h>

#ifdef _MSC_VER
#ifdef max
#undef max
#endif
#endif

#define TWEAK_AUDIO_DEFAULT_SAMPLE_RATE 48000

// #define ENABLE_AUDIOBUFFER_CHECK

#ifdef ENABLE_AUDIOBUFFER_CHECK
#define AUDIOBUFFER_CHECK(_audioBufferInst, _label) \
    {                                               \
        (_audioBufferInst).checkBuffer(_label);     \
    }
#else
#define AUDIOBUFFER_CHECK(_audioBufferInst, _label)
#endif

namespace TwkAudio
{

    typedef double Time;
    static const Time Always = std::numeric_limits<Time>::max();
    static const Time Never = -Always;

    /// Audio has its own Time type for indexing into audio streams

    ///
    /// Using double for Time until it needs to be higher resolution. A double
    /// can hold all 2^32 integers which at 96khz is about 6 hours of stereo.
    ///
    /// The Time unit is seconds.
    ///
    /// SampleTime is actually an offset in samples from the 0th sample
    /// which should correspond to Time(0). So Time(0) and SampleTime(0)
    /// are the same time for any sample rate.
    ///

#ifdef WIN32
    typedef ptrdiff_t
        SampleTime; // should be signed size_t, may need to use ptrdiff_t
#else
    typedef ssize_t
        SampleTime; // should be signed size_t, may need to use ptrdiff_t
#endif

    /// Convert sample count to Audio Time

    inline Time samplesToTime(SampleTime samples, Time rate)
    {
        return double(samples) / rate;
    }

    inline Time samplesToTime(size_t samples, Time rate)
    {
        return double(samples) / rate;
    }

    /// Convert Audio Time to sample count. Make sure to round "down" for
    /// negative times

    inline SampleTime timeToSamples(Time t, Time rate)
    {
        return SampleTime((t < 0) ? t * rate - 0.49 : t * rate + 0.49);
    }

    /// SampleVector is the native storage for audio samples

    typedef std::vector<float, stl_ext::replacement_allocator<float>>
        SampleVector;

    // typedef std::vector<float> SampleVector;

    /// AudioBuffer is the basic building block for Audio

    ///
    /// The AudioBuffer contains (currently) an interleaved sample buffer,
    /// the number of channels, and the start time and rate. The duration
    /// can obviously be obtained from the buffer size and rate.
    ///
    /// We're only dealing with 32 bit float as the audio format. There is
    /// no global rate, so audio processing may require resampling.
    ///
    /// You can make an AudioBuffer from external memory by passing in the
    /// relevant data to the contructor. In that case the memory is not
    /// owned by the AudioBuffer, so it will not be freed when the
    /// AudioBuffer is deleted.
    ///
    /// The AudioBuffer may have extra samples before and after the
    /// begining and end of the "real" data -- called a margin. This is
    /// measured in samples not Time. Its possible to have a margin with
    /// internal and external data. For example, if the margin is 8
    /// samples on a one channel buffer, the value returned by point()
    /// will be 8 floats into the full data range. To access the full
    /// range, use pointerIncludingMargin() and sizeIncludingMargin().
    ///

    class TWKAUDIO_EXPORT AudioBuffer
    {
    public:
        ///
        ///  Types
        ///  BufferPointer is a raw memory pointer to samples
        ///

        typedef SampleVector::value_type SampleType;
        typedef SampleVector::iterator iterator;
        typedef SampleVector::reverse_iterator reverse_iterator;
        typedef SampleVector::const_iterator const_iterator;
        typedef SampleType* BufferPointer;

        ///
        /// Empty AudioBuffer
        ///

        AudioBuffer();

        ///
        /// This constructor will create/own the memory. numSamples should
        /// not include any additional margin samples (which are specified
        /// separately via margin).
        ///

        AudioBuffer(size_t numSamples, ChannelsVector channels, Time rate,
                    Time startTime = 0, size_t margin = 0);

        AudioBuffer(size_t numSamples, TwkAudio::Layout layout, Time rate,
                    Time startTime = 0, size_t margin = 0);

        ///
        /// This constructor variation uses Time instead of exact
        /// samples. The start and duration should not include the margin.
        ///

        AudioBuffer(Time start, Time duration, Time rate,
                    ChannelsVector channels, size_t margin = 0);

        ///
        /// This constructor allows external memory to be used. If you
        /// have a margin, the passed in pointer should be the pointer to
        /// the first value of the entire memory region, not the first
        /// value of the non-margin data. However, the numSamples should
        /// not include the margin. So the total size of the
        /// externalMemory region in samples should be numSamples + 2 *
        /// margin.
        ///

        AudioBuffer(BufferPointer externalMemory, ChannelsVector channels,
                    size_t numSamples, Time start, Time rate,
                    size_t margin = 0);

        ///
        /// Create an AudioBuffer that's a window into another. The passed in
        /// buffer's memory is referenced so the lifetime of that object must
        /// outlive the lifetime of this one.
        ///

        AudioBuffer(AudioBuffer& inbuffer, size_t startOffsetSample,
                    size_t numSamples, Time startTime = 0);

        ~AudioBuffer() {}

        ///
        ///  Reconfiguring will cause external data to be copied
        ///  locally. numSamples should not include the margin.
        ///

        void reconfigure(size_t numSamples, ChannelsVector channels, Time rate,
                         Time startTime = 0, size_t margin = 0);

        void reconfigure(size_t numSamples, TwkAudio::Layout layout, Time rate,
                         Time startTime = 0, size_t margin = 0);

        ///
        /// Raw pointer access
        ///

        BufferPointer pointer() { return m_data; }

        const BufferPointer pointer() const { return m_data; }

        BufferPointer pointerIncludingMargin() { return m_data - m_margin; }

        const BufferPointer pointerIncludingMargin() const
        {
            return m_data - m_margin;
        }

        ///
        /// Returns the margin value (in samples) on either side of the
        /// data
        ///

        size_t margin() const { return m_margin; }

        ///
        /// Rate in samples/seconds (e.g. 44100.0 is 44.1kHz)
        ///

        Time rate() const { return m_rate; }

        ///
        /// Number of channels in a sample. The AudioBuffer currently
        /// contains only interleaved sample data.
        ///

        unsigned int numChannels() const { return m_channels.size(); }

        ///
        /// This is the vector of Channel enums in the order of the
        /// channels contained in the buffer.
        ///

        ChannelsVector channels() const { return m_channels; }

        ///
        /// Size in samples not including margin
        ///

        size_t size() const { return m_numSamples; }

        ///
        /// Size in samples (including margin)
        ///

        size_t sizeIncludingMargin() const
        {
            return m_numSamples + 2 * m_margin;
        }

        ///
        /// Size in bytes. Does not include the margin.
        ///

        size_t sizeInBytes() const
        {
            return m_numSamples * m_channels.size() * sizeof(float);
        }

        ///
        /// Size in bytes. Including the margin.
        ///

        size_t sizeInBytesIncludingMargin() const
        {
            return (m_numSamples + 2 * m_margin) * m_channels.size()
                   * sizeof(float);
        }

        ///
        /// Number of floats in buffer (samples * channels). Does not
        /// include the margin.
        ///

        size_t sizeInFloats() const { return m_numSamples * m_channels.size(); }

        ///
        /// Number of floats in buffer (samples * channels).  Including
        /// the margin.
        ///

        size_t sizeInFloatsIncludingMargin() const
        {
            return (m_numSamples + 2 * m_margin) * m_channels.size();
        }

        ///
        ///  Derived length in seconds. Does not include the margin
        ///  samples.
        ///

        Time duration() const { return Time(size()) / m_rate; }

        ///
        /// Start time in seconds. Does not include the margin.
        ///

        Time startTime() const { return m_startTime; }

        ///
        /// Start sample number, 0 is the first sample. In order to
        /// prevent disadvantageous aliasing, this function will "center"
        /// the time value. Does not include the margin.
        ///

        SampleTime startSample() const
        {
            return timeToSamples(m_startTime, m_rate);
        }

        ///
        /// Setting buffer state
        ///

        void setRate(Time rate) { m_rate = rate; }

        ///
        /// Set the buffer start time directly. Does not include the
        /// margin.
        ///

        void setStartTime(Time startTime) { m_startTime = startTime; }

        ///
        /// Fill the buffer with silence (zeros) including the margin data
        ///

        void zero();

        ///
        /// Fill a region of the buffer with silence and apply low-pass
        /// filter to get rid of any popping
        ///

        void zeroRegion(size_t startSample, size_t numSamples);

        ///
        /// Resize calls reconfigure. Preserves margins.
        ///

        void resize(size_t numSamples)
        {
            reconfigure(numSamples, m_channels, m_rate, m_startTime, m_margin);
        }

        ///
        /// Is the data local to this object
        ///

        bool bufferOwnsData() const;

        ///
        ///  Make the AudioBuffer copy external data to internal
        ///

        void ownData();

        ///
        /// Reverse the data in the buffer.
        /// If reverseChannels is true, the channels are reversed too.
        /// Perform this operation for reverse playback.
        ///

        void reverse(bool reverseChannels = false);

        ///
        /// Returns true if the buffer data is ok and false if its corrupted.
        /// This method is really for development purposes only to
        /// assist in finding the places in the audio processing
        /// chain that might be introducing audio corruption.
        ///

        bool checkBuffer(const char* label = "") const;

    private:
        BufferPointer m_data;  /// Pointer to the data (could be external)
        size_t m_numSamples;   /// Only if external data is used
        SampleVector m_buffer; /// internal buffer (if not external memory)
        Time m_rate;           /// Rate in samples / second
        Time m_startTime;      /// Start time in seconds
        size_t m_margin;       /// additional margin samples at head and tail
        ChannelsVector m_channels; /// named channel order used to mix buffers
    };

} // namespace TwkAudio

#endif // __TwkAudio__Audio__h__
