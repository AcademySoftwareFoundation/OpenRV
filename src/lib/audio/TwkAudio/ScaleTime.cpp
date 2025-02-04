//
//  Copyright (c) 2008 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <TwkAudio/ScaleTime.h>
#include <TwkMath/Function.h>
#include <string.h>

namespace TwkAudio
{
    using namespace std;
    using namespace TwkMath;

    void scaleTime(const AudioBuffer& inbuffer, AudioBuffer& outbuffer)
    {
        const size_t insize = inbuffer.size();
        const size_t outsize = outbuffer.size();
        const size_t ch = outbuffer.numChannels();

        if (insize == outsize)
        {
            memcpy(outbuffer.pointer(), inbuffer.pointer(),
                   inbuffer.sizeInBytes());
        }
        else if (insize > outsize)
        {
            if (ch == 1)
            {

                size_t index = 0;
                double s = outbuffer.size();
                float* o = outbuffer.pointer();

                for (const float *i = inbuffer.pointer(),
                                 *e = i + outbuffer.size();
                     i < e; i++, index++)
                {
                    *o = *i * smoothstep(1.0 - double(index) / s);
                }

                index = 0;
                o = outbuffer.pointer();

                for (const float *i = inbuffer.pointer() + (insize - outsize),
                                 *e = i + outbuffer.size();
                     i < e; i++, index++)
                {
                    *o = *i * smoothstep(double(index) / s);
                }
            }
            else if (ch == 2)
            {
            }
        }
    }

} // namespace TwkAudio
