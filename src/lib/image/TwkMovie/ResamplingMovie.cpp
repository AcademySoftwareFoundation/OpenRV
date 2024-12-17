//******************************************************************************
// Copyright (c) 2016 Autodesk
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkMovie/ResamplingMovie.h>

namespace TwkMovie
{
    using namespace std;
    using namespace TwkAudio;

    bool ResamplingMovie::debug = false;
    bool ResamplingMovie::dump = false;

    ofstream* sourceAudioDump = 0;
    ofstream* resampledAudioDump = 0;

    void dumpAudio(AudioBuffer* buffer, string filename, bool isBackwards,
                   ofstream** dumpfile)
    {
        if (isBackwards)
            buffer->reverse();

        if (!*dumpfile)
            *dumpfile = new ofstream(filename.c_str(), ios::binary | ios::out);

        (*dumpfile)->write(reinterpret_cast<const char*>(buffer->pointer()),
                           buffer->sizeInBytes());

        if (isBackwards)
            buffer->reverse();
    }

    ResamplingMovie::ResamplingMovie(Movie* mov)
        : m_movie(mov)
        , m_resampler(0)
        , m_readPosition(0)
        , m_readPositionOffset(0)
        , m_readStart(0)
        , m_readContinuation(false)
        , m_isBackwards(false)
        , m_accumBuffer(0)
        , m_accumStart(0)
        , m_accumSamples(0)
        , m_accumAllocated(0)
    {
    }

    ResamplingMovie::~ResamplingMovie()
    {
        delete m_resampler;
        delete[] m_accumBuffer;
    }

    void ResamplingMovie::setDebug(bool b) { ResamplingMovie::debug = b; }

    void ResamplingMovie::setDumpAudio(bool b) { ResamplingMovie::dump = b; }

    TwkAudio::SampleTime ResamplingMovie::offsetStart(double rate)
    {
        double sourceRate = audioMovie()->info().audioSampleRate;
        bool doOffset =
            (!audioMovie()->canConvertAudioRate() && sourceRate < rate
             && m_readPositionOffset > AUDIO_READPOSITIONOFFSET_THRESHOLD);
        Time startTime =
            doOffset ? m_readStart - m_readPositionOffset : m_readStart;
        return TwkAudio::timeToSamples(startTime, rate);
    }

    void ResamplingMovie::shiftBuffer(size_t samples, unsigned int channels)
    {
        if (samples != m_accumSamples && samples != 0)
        {
            memmove(m_accumBuffer, m_accumBuffer + samples * channels,
                    (m_accumSamples - samples) * channels * sizeof(float));
        }
    }

    void ResamplingMovie::reset(int channels, float factor, size_t samples,
                                int blocksize)
    {
        if (ResamplingMovie::debug)
        {
            cerr << "AUDIO: ResamplingMovie::reset channels=" << channels
                 << " factor=" << factor << " samples=" << samples
                 << " blocksize=" << blocksize << endl;
        }

        channels = (!audioMovie()->canConvertAudioChannels())
                       ? audioMovie()->info().audioChannels.size()
                       : channels;

        if (factor == 1.0)
        {
            delete m_resampler;
            delete[] m_accumBuffer;
            m_accumBuffer = 0;
            m_resampler = 0;
            m_accumSamples = 0;
            m_accumStart = 0;
            m_readPosition = 0;
            m_accumAllocated = 0;
        }
        else if (!m_resampler || factor != m_resampler->factor()
                 || channels != m_resampler->size())
        {
            if (m_resampler)
                delete m_resampler;
            if (m_accumBuffer)
                delete m_accumBuffer;
            m_accumSamples = 0;
            m_accumStart = 0;
            m_readPosition = 0;
            m_accumAllocated = 0;

            size_t bsize = samples * 2;
            m_accumBuffer = new float[bsize * channels];
            m_resampler = new MultiResampler(channels, factor, blocksize);
            m_accumAllocated = bsize;
        }
    }

    size_t ResamplingMovie::audioFillBuffer(AudioBuffer& buffer)
    {
        Movie* mov = audioMovie();

        const bool needsChannelMix =
            !(mov->canConvertAudioChannels()
              || buffer.channels() == mov->info().audioChannels);
        const ChannelsVector audioChannels =
            (needsChannelMix) ? mov->info().audioChannels : buffer.channels();

        const double rate = buffer.rate();
        const unsigned int numChannels = audioChannels.size();
        const size_t numSamples = buffer.size();

        const size_t margin = 0;
        const bool needsResample = !(mov->canConvertAudioRate()
                                     || rate == mov->info().audioSampleRate);
        const SampleTime bufferStart = buffer.startSample();
        const Time startTime = samplesToTime(bufferStart, rate);

        m_readStart = startTime;
        m_readContinuation = false;

        //
        //  If this media cannot resample itself and has a different source
        //  audio sampling rate from the device, then we need to retime the
        //  audio sample target and prepare the resampler.
        //

        Time length = needsResample ? retimeAudioSampleTarget(buffer, startTime,
                                                              numChannels)
                                    : samplesToTime(numSamples, rate);

        //
        //  Don't bother asking for samples if we are out of the valid range
        //
        //  NOTE: The orginal buffer start sample is used and not the offset
        //  start sample as calculated for resampling. This is because the min
        //  and max available samples are calculated without taking resampling
        //  skew into consideration.
        //

        if (bufferStart > maxAvailableSample(rate)
            || bufferStart + SampleTime(numSamples) < minAvailableSample(rate))
        {
            if (ResamplingMovie::debug)
                cerr << "ERROR: read " << offsetStart(rate)
                     << " is out of range" << endl;
            return 0;
        }

        //
        //  Fill in the audio read request and call the movie with a temporary
        //  audio buffer.
        //

        Movie::AudioReadRequest request(samplesToTime(offsetStart(rate), rate),
                                        length, margin);

        size_t nread = mov->audioFillBuffer(request, buffer);

        if (ResamplingMovie::dump)
            dumpAudio(&buffer, "audio-source-dump", m_isBackwards,
                      &sourceAudioDump);

        if (nread == 0)
        {
            // This might happen on loopback when the startSample goes to a very
            // large number.
            return 0;
        }

        //
        //  Check for and remove NANs in buffer
        //  XXX: This will clamp files with values outside of -1.0 to 1.0
        //

        float* fp = (float*)buffer.pointerIncludingMargin();
        float* lim = fp + buffer.sizeInFloatsIncludingMargin();
        int nanCount = 0;
        int outOfBounds = 0;
        while (fp < lim)
        {
            if (*fp != *fp)
            {
                ++nanCount;
                *fp = 0.0;
            }
            if (*fp > 1.0f)
            {
                ++outOfBounds;
                *fp = 1.0f;
            }
            if (*fp < -1.0f)
            {
                ++outOfBounds;
                *fp = -1.0f;
            }
            ++fp;
        }

        if (ResamplingMovie::debug && (nanCount || outOfBounds))
        {
            cerr << "ERROR: " << nanCount << " NANs in audio stream and "
                 << outOfBounds << " floats out of bounds." << endl;
        }

        //
        //  Expect samples is in terms of the returned rate not the desired
        //  rate
        //

        const size_t expectedSamples = timeToSamples(length, buffer.rate());

        if (buffer.size() != expectedSamples)
            buffer.resize(expectedSamples);
        if (nread < expectedSamples)
        {
            memset(buffer.pointer() + nread * numChannels, 0,
                   (expectedSamples - nread) * numChannels * sizeof(float));
        }

        if (needsResample && m_resampler)
        {
            AUDIOBUFFER_CHECK(buffer, "FileSourceIPNode::audioFillFromMedia() "
                                      "from media (before resample)")

            resampleAudio(buffer, nread, numSamples, startTime, audioChannels,
                          rate);

            AUDIOBUFFER_CHECK(
                buffer, "FileSourceIPNode::audioFillFromMedia() after resample")
        }

        if (ResamplingMovie::dump)
            dumpAudio(&buffer, "audio-source-resampled-dump", m_isBackwards,
                      &resampledAudioDump);

        return nread;
    }

    TwkAudio::Time ResamplingMovie::retimeAudioSampleTarget(
        AudioBuffer buffer, const Time startTime, const int numChannels)
    {
        const double rate = buffer.rate();       // requested rate
        const size_t numSamples = buffer.size(); // requested samples

        //
        //  Find out if readStart is in the accum buffer window.
        //
        //  NOTE: we're assuming that we'll never be asked for a region larger
        //  than the device samples size multiplied by a magic number at
        //  least 2.
        //
        //  Find out if the requested time window starts anywhere inside our
        //  buffer.
        //

        const Time tdiff = m_readStart - m_accumStart;
        const double aepsilon = 1.0 / (rate * 2.0); // half an audio sample time

        //
        //  If there are samples in our read buffer already, and if readStart
        //  falls within our (potential) read buffer, we'll continue reading
        //  where we left off.
        //

        if (m_accumSamples && tdiff >= -aepsilon
            && tdiff < samplesToTime(m_accumAllocated, rate))
        {
            //
            //  We're within range of our buffer. If readStart is past the first
            //  sample, we need to shift the buffer back so that readStart is
            //  the first sample. Once this is accomplished we can fill the
            //  remaining buffer.
            //

            size_t shiftSamples = timeToSamples(tdiff, rate);

            if (shiftSamples > m_accumSamples)
            {
                //
                //  The shift is beyond the data we have so far, if it's within
                //  the readable region. Just nuke the existing samples and
                //  continue to read as much as possible. Otherwise, we can't
                //  continue and need to start up from scratch.
                //

                if (m_accumAllocated - shiftSamples >= numSamples)
                {
                    m_accumStart = m_readPosition;
                    m_accumSamples = 0;
                    m_readContinuation = true;
                }
                else
                {
                    m_accumStart = m_readStart;
                    m_accumSamples = 0;
                    m_readContinuation = false;
                }
                if (m_resampler)
                    m_resampler->reset();
            }
            else if (fabs(tdiff) > aepsilon)
            {
                //
                //  Erase the samples which are in the past. Read the
                //  remainder.
                //

                shiftBuffer(shiftSamples, numChannels);

                m_accumStart = m_readStart;
                m_accumSamples -= shiftSamples;
                m_readContinuation = true;
            }
            else if (shiftSamples == 0)
            {
                m_readContinuation = true;
            }
        }

        Time length;
        if (m_readContinuation)
        {
            m_readStart = m_readPosition;
            length = samplesToTime(m_accumAllocated - m_accumSamples, rate);
        }
        else
        {
            m_accumStart = 0;
            m_accumSamples = 0;
            m_readPositionOffset = 0;
            m_readStart = startTime;
            length = samplesToTime(m_accumAllocated, rate);

            //
            // We reset the resampler under these circumstances:
            // 1) For forward play, when the startSample is less than the
            // minAvailableSample. 2) For backwards play, when the startSample
            // is greater than the maxAvailableSample.
            //
            // When backwards the "first" sample is startSample + numSamples.
            //
            // In other words the resampler gets reset at first/last sample
            // transitions and loop backs. This prevents discontinuitites at
            // these points to make their way into the resampler's internal
            // buffer (which is about 4k).
            //
            // NOTE: Just like when deciding whether or not to request audio
            // from the underlying plugin we use the non-resampled original
            // buffer start sample when comparing to min and max available
            // samples as they are reported indifferent to resampling skew.
            //

            const SampleTime bufferStart = buffer.startSample();
            if (((!m_isBackwards && bufferStart <= minAvailableSample(rate))
                 || (m_isBackwards
                     && bufferStart + SampleTime(numSamples)
                            >= maxAvailableSample(rate)))
                && m_resampler)
            {
                m_resampler->reset();
            }
        }

        return length;
    }

    SampleTime ResamplingMovie::minAvailableSample(double audioSampleRate)
    {
        int audioStartFrame = 0;
        Time startOffset = (double(audioStartFrame) / audioMovie()->info().fps);
        return timeToSamples(startOffset, audioSampleRate);
    }

    SampleTime ResamplingMovie::maxAvailableSample(double audioSampleRate)
    {
        int audioEndFrame =
            audioMovie()->info().end - audioMovie()->info().start + 1;
        Time endOffet = (double(audioEndFrame) / audioMovie()->info().fps);
        return timeToSamples(endOffet, audioSampleRate) - 1;
    }

    void ResamplingMovie::resampleAudio(AudioBuffer& buffer, const size_t nread,
                                        const size_t numSamples,
                                        const Time startTime,
                                        const ChannelsVector audioChannels,
                                        const double rate)
    {
        const int numChannels = audioChannels.size();

        //
        //  Fill in the audio read request and call the movie with a
        //  temporary audio buffer. ping and pong are used as "ping-pong"
        //  buffers. ping should always contain the currently being
        //  processed audio. Some operations in here will copy to pong and
        //  then swap them to avoid a copy.
        //

        AudioBuffer* ping = &buffer;
        AudioBuffer* pong = &m_buffer;

        //
        //  Convert to the proper sampling rate
        //

        const double factor = m_resampler->factor();
        size_t nresampled =
            timeToSamples(samplesToTime(nread, ping->rate()), rate);

        pong->reconfigure(nresampled, audioChannels, rate, m_readStart,
                          size_t(factor * ping->margin()));

        //
        // NB: When upsampling i.e. factor > 1.0; we enable clamping
        // as overshoots are possible.
        //

        if (m_isBackwards)
            ping->reverse();
        nresampled = m_resampler->process(
            ping->pointerIncludingMargin(), ping->sizeIncludingMargin(),
            pong->pointerIncludingMargin(), pong->sizeIncludingMargin(),
            false, // buffer already padded
            (factor > 1.0));

        pong->resize(nresampled);

        swap(ping, pong);

        if (m_isBackwards)
            ping->reverse();

        //
        // NB: the duration we add on is really the duration actually obtained
        // from the plugin... i.e. its possible that length !=
        // samplesToTime(nread, srate).
        //

        const double srate = audioMovie()->info().audioSampleRate;
        m_readPosition = m_readStart + samplesToTime(nread, srate);

        //
        // We accumulate the readPositionOffset; otherwise we get a slip from
        // accumulated differences in readPosition values from the amount of
        // time read by the plugin (samples in source rate) versus this amount
        // of time after conversion... which is the amount of time stored in
        // accumBuffer (computed in device rate as device rate samples after
        // resampling) NB: This slip only happens for the case srate < rate
        // because for this
        //     case length is computed in terms of rate i.e. since its higher
        //     sample rate. In the case srate > rate, length is calculated in
        //     terms of srate and so there is no slippage.
        //

        if (srate < rate)
        {
            m_readPositionOffset +=
                samplesToTime(nread, srate) - samplesToTime(nresampled, rate);
        }

        //
        //  Reassemble if needed
        //

        pong->reconfigure(numSamples, audioChannels, rate, startTime);

        if (m_readContinuation)
        {
            //
            //  Assemble the requested audio packet into buffer and then
            //  swap with ping
            //

            const bool accumFlushed = m_accumSamples < numSamples;
            const size_t asamps = (accumFlushed) ? m_accumSamples : numSamples;

            if (asamps)
            {
                memcpy(pong->pointer(), m_accumBuffer,
                       asamps * numChannels * sizeof(float));

                shiftBuffer(asamps, numChannels);
                m_accumSamples -= asamps;
                m_accumStart += samplesToTime(asamps, rate);
            }

            const size_t rsamps = (accumFlushed) ? numSamples - asamps : 0;

            if (rsamps)
            {
                memcpy(pong->pointer() + numChannels * asamps, ping->pointer(),
                       rsamps * numChannels * sizeof(float));
            }

            size_t resampledSamplesToAccumulate = ping->size() - rsamps;
            if (m_accumSamples + resampledSamplesToAccumulate
                > m_accumAllocated)
            {
                //
                // We should really never get this condition.
                // But if we did this will prevent an out of bounds memory
                // write.
                //

                cerr << "*****ERROR: accumBuffer copy exceeded! - 1" << endl;
                resampledSamplesToAccumulate =
                    m_accumAllocated - m_accumSamples;
            }

            memcpy(m_accumBuffer + m_accumSamples * numChannels,
                   ping->pointer() + rsamps * numChannels,
                   resampledSamplesToAccumulate * numChannels * sizeof(float));

            m_accumSamples += resampledSamplesToAccumulate;
            m_accumStart += samplesToTime(rsamps, rate);
        }
        else
        {
            //
            //  Outside the buffer window. In this case copy the used ones into
            //  a new properly sized buffer and then update the accum buffer
            //  with the unused samples.
            //

            size_t nt = numSamples * numChannels * sizeof(float);
            size_t copied = 0;
            if (pong->sizeInBytes() >= nt && ping->sizeInBytes() >= nt)
            {
                memcpy(pong->pointer(), ping->pointer(), nt);
                copied += numSamples;
            }

            if (ping->size())
            {
                size_t resampledSamplesToAccumulate =
                    (ping->size() >= numSamples) ? ping->size() - numSamples
                                                 : ping->size();

                if (resampledSamplesToAccumulate > m_accumAllocated)
                {
                    //
                    // We should really never get this condition.
                    // But if we did this will prevent an out of bounds memory
                    // write.
                    //

                    cerr << "*****ERROR: accumBuffer copy exceeded! - 2"
                         << endl;
                    resampledSamplesToAccumulate = m_accumAllocated;
                }

                m_accumSamples = resampledSamplesToAccumulate;
                m_accumStart = startTime + samplesToTime(copied, rate);
                if (resampledSamplesToAccumulate)
                {
                    memcpy(m_accumBuffer,
                           ping->pointer() + copied * numChannels,
                           resampledSamplesToAccumulate * numChannels
                               * sizeof(float));
                }
            }
        }

        swap(ping, pong);
    }

} // namespace TwkMovie
