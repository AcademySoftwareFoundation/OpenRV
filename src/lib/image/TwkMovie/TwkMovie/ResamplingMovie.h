//******************************************************************************
// Copyright (c) 2016 Autodesk
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkMovie__ResamplingMovie__h__
#define __TwkMovie__ResamplingMovie__h__
#include <TwkMovie/Movie.h>
#include <TwkMovie/dll_defs.h>
#include <TwkAudio/Resampler.h>
#include <fstream>

#define AUDIO_READPOSITIONOFFSET_THRESHOLD \
    0.01 // In secs; this is the max amount of slip we will allow

namespace TwkMovie
{

    class TWKMOVIE_EXPORT ResamplingMovie
    {
    public:
        ResamplingMovie(Movie* mov);
        ~ResamplingMovie();

        static bool debug;
        static void setDebug(bool b);
        static bool dump;
        static void setDumpAudio(bool b);

        virtual Movie* audioMovie() { return m_movie; };

        size_t audioFillBuffer(TwkAudio::AudioBuffer& buffer);

        void setBackwards(bool backwards) { m_isBackwards = backwards; }

        void reset(int channels, float factor, size_t samples,
                   int blocksize = 256);

    private:
        TwkAudio::SampleTime offsetStart(double rate);
        void shiftBuffer(size_t samples, unsigned int channels);
        TwkAudio::Time retimeAudioSampleTarget(TwkAudio::AudioBuffer buffer,
                                               const TwkAudio::Time startTime,
                                               const int numChannels);
        void resampleAudio(TwkAudio::AudioBuffer& buffer, const size_t nread,
                           const size_t numSamples,
                           const TwkAudio::Time startTime,
                           const TwkAudio::ChannelsVector audioChannels,
                           const double rate);
        TwkAudio::SampleTime minAvailableSample(double rate);
        TwkAudio::SampleTime maxAvailableSample(double rate);

        Movie* m_movie;
        TwkAudio::MultiResampler* m_resampler;
        TwkAudio::Time m_readPosition;
        TwkAudio::Time m_readPositionOffset;
        TwkAudio::Time m_readStart;
        bool m_readContinuation;
        bool m_isBackwards;
        float* m_accumBuffer;
        TwkAudio::Time m_accumStart;
        size_t m_accumSamples;
        size_t m_accumAllocated;
        TwkAudio::AudioBuffer m_buffer;
    };

} // namespace TwkMovie

#endif // __TwkMovie__ResamplingMovie__h__
