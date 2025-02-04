//
//  Copyright (c) 2008 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <TwkAudio/Filters.h>
#include <assert.h>

namespace TwkAudio
{
    using namespace std;

    void lowPassFilter(AudioBuffer& inBuffer, AudioBuffer& prevBuffer,
                       AudioBuffer& outBuffer, float freq, bool isBackwards)
    {
        //
        //  Simple low-pass, seeded with outPrevious's last sample
        //

        //  f = 1 / (2 * pi * R * C) --- where R = resistance, C = capacitance
        //  alpha = dt / (R * C + dt)
        //  dt = 1 / rate
        //
        //  so:
        //
        //  alpha = dt / ( 1 / (f * 2 * pi) + dt )
        //

        assert(inBuffer.rate() == outBuffer.rate()
               && inBuffer.rate() == prevBuffer.rate());

        const double dt = 1.0 / inBuffer.rate();
        const float alpha =
            float(dt / (1.0 / (double(freq) * 2 * 3.14159265359) + dt));

        // Reverse the buffers on backwards playback
        // otherwise the buffer segments will be discontinuous
        // at the boundaries.
        if (isBackwards)
        {
            inBuffer.reverse();
            prevBuffer.reverse();
        }

        AudioBuffer::BufferPointer out = outBuffer.pointer();
        const AudioBuffer::BufferPointer in = inBuffer.pointer();
        const AudioBuffer::BufferPointer prev = prevBuffer.pointer();
        const size_t nc = outBuffer.numChannels();
        const size_t n = outBuffer.size();
        const size_t pend = (prevBuffer.size() - 1) * nc;

        //
        //  Seed the first value
        //

        for (size_t c = 0; c < nc; c++)
        {
            const float p = prev[pend + c];
            out[c] = p + alpha * (in[c] - p);
        }

        //
        //  Do the rest
        //

        for (size_t i = 1; i < n; i++)
        {
            for (size_t c = 0; c < nc; c++)
            {
                const size_t q = i * nc + c;
                const float p = out[q - nc];

                out[q] = p + alpha * (in[q] - p);
            }
        }

        if (isBackwards)
        {
            inBuffer.reverse();
            prevBuffer.reverse();
            if (in != out)
            {
                outBuffer.reverse();
            }
        }
    }

} // namespace TwkAudio
