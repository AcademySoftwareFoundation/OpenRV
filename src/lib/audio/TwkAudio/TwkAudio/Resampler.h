//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkAudio__Resampler__h__
#define __TwkAudio__Resampler__h__
#include <TwkAudio/Audio.h>
#include <TwkAudio/dll_defs.h>

namespace TwkAudio
{

    /// Change sampling rate of a single channel stream.

    ///
    /// The Resampler class can take a single channel audio stream (as
    /// floats) and convert to another sampling rate. This is the lowest
    /// level interface for resampling.
    ///
    /// To make a Resampler you need to compute the resampling
    /// factor. This is simply the output rate divided by the input
    /// rate. So for example, if you have samples at 44.1kHz and you want
    /// to convert to 48kHz, the factor would be 48000.0 / 44100.0.
    ///

    class TWKAUDIO_EXPORT Resampler
    {
    public:
        Resampler(double factor, size_t blocksize = 64);
        ~Resampler();

        void close();

        size_t blocksize() const { return m_blocksize; }

        double factor() const { return m_factor; }

        size_t filterWidth() const { return m_filterwidth; }

        size_t outputSize(size_t inputSize) const
        {
            return size_t(m_factor * double(inputSize));
        }

        ///
        /// This function is called repeatedly to resample the audio. The
        /// return value indicates how much of the output was actually
        /// converted. It may not be equal to the outSize.
        ///

        size_t process(const float* in, size_t inSize, float* out,
                       size_t outSize, bool endFlag, bool enableClamp);

        ///
        /// This is a convenience function which calls the other process
        /// function.
        ///

        void process(const SampleVector& in, SampleVector& out, bool endFlag,
                     bool enableClamp);

        ///
        /// Resets the resampler to its initial state.
        ///

        void reset();

    private:
        void open();

    private:
        double m_factor;
        size_t m_blocksize;
        void* m_handle;
        int m_filterwidth;
    };

    /// Resample a n-channel audio stream into a n-channel output
    /// stream

    ///
    /// This class uses n Resampler instances to generate a
    /// n-channel output stream from n-channel input
    /// stream. Interleaving and deinterleaving are handed
    /// automatically.
    ///

    class TWKAUDIO_EXPORT MultiResampler
    {
    public:
        typedef SampleVector FloatBuffer;
        typedef std::vector<FloatBuffer> FloatBuffers;
        typedef std::vector<Resampler*> Resamplers;

        MultiResampler(int numChannels, double factor, size_t blocksize = 64);
        ~MultiResampler();

        void close();

        size_t blocksize() const { return m_resamplers.front()->blocksize(); }

        double factor() const { return m_resamplers.front()->factor(); }

        size_t size() const { return m_resamplers.size(); }

        size_t filterWidth() const
        {
            return m_resamplers.front()->filterWidth();
        }

        size_t outputSize(size_t inputSize) const
        {
            return m_resamplers.front()->outputSize(inputSize);
        }

        size_t process(const float* in, size_t inSize, float* out,
                       size_t outSize, bool endFlag, bool enableClamp);

        void reset();

    private:
        void open();

    private:
        Resamplers m_resamplers;
        FloatBuffers m_inbuffers;
        FloatBuffers m_outbuffers;
    };

} // namespace TwkAudio

#endif // __TwkAudio__Resampler__h__
