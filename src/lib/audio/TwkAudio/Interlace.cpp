//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkAudio/Interlace.h>
#include <iostream>
#include <assert.h>
#include <string.h>

namespace TwkAudio
{
    using namespace std;

    void deinterlace(const float* inbuffer, size_t inSamples, int nchannels,
                     vector<SampleVector>& outbuffers)
    {
        outbuffers.resize(nchannels);

        for (unsigned int i = 0; i < nchannels; i++)
        {
            outbuffers[i].resize(inSamples);
        }

        //
        //  De-interlace
        //

        const float* inp = inbuffer;
        const float* endp = inp + (inSamples * nchannels);

        for (unsigned int c = 0; c < nchannels; c++)
        {
            float* o = &(outbuffers[c].front());

            for (const float* p = inp + c; p < endp; p += nchannels, o++)
            {
                *o = *p;
            }
        }
    }

    void deinterlace(const SampleVector& inbuffer, int nchannels,
                     vector<SampleVector>& outbuffers)
    {
        outbuffers.resize(nchannels);

        for (unsigned int i = 0; i < outbuffers.size(); i++)
        {
            outbuffers[i].resize(inbuffer.size() / nchannels);
        }

        //
        //  De-interlace
        //

        const float* inp = &(inbuffer.front());
        const float* endp = inp + inbuffer.size();

        for (unsigned int c = 0; c < nchannels; c++)
        {
            float* o = &(outbuffers[c].front());

            for (const float* p = inp + c; p < endp; p += nchannels, o++)
            {
                *o = *p;
            }
        }
    }

    void interlace(const vector<SampleVector>& inbuffers,
                   SampleVector& outbuffer)
    {
        const unsigned int nc = inbuffers.size();
        outbuffer.resize(inbuffers.front().size() * inbuffers.size());

        //
        //  Have a special case for the most used forms
        //

        if (inbuffers.size() == 1)
        {
            memcpy(&outbuffer.front(), &inbuffers[0].front(),
                   inbuffers[0].size() * sizeof(float));
        }
        else if (inbuffers.size() == 2)
        {
            const float* p0 = &inbuffers[0].front();
            const float* p1 = &inbuffers[1].front();
            float* o = &outbuffer.front();

            for (const float* e = o + outbuffer.size(); o < e; o++, p0++, p1++)
            {
                *o = *p0;
                o++;
                *o = *p1;
            }
        }
        else
        {
            for (unsigned int i = 0; i < outbuffer.size(); i += nc)
            {
                for (unsigned int q = 0; q < nc; q++)
                {
                    outbuffer[i + q] = inbuffers[q][i / nc];
                }
            }
        }
    }

    void interlace(const vector<SampleVector>& inbuffers, float* outbuffer,
                   size_t start, size_t num)
    {
        const unsigned int nc = inbuffers.size();
        const size_t n = num > 0 ? num : inbuffers.front().size();

        //
        //  Have a special case for the most used forms
        //

        if (inbuffers.size() == 1)
        {
            memcpy(outbuffer + start, &inbuffers[0].front(), n * sizeof(float));
        }
        else if (inbuffers.size() == 2)
        {
            assert(inbuffers[0].size() == inbuffers[1].size());
            assert(inbuffers[0].size() >= n);
            assert(inbuffers[0].size() > 0);
            assert(inbuffers[1].size() > 0);
            const float* p0 = &(inbuffers[0].front());
            const float* p1 = &(inbuffers[1].front());
            float* o = outbuffer + (start * 2);

            for (const float* e = o + (n * 2); o < e; o++, p0++, p1++)
            {
                *o = *p0;
                o++;
                *o = *p1;
            }
        }
        else
        {
            for (unsigned int i = 0; i < (n * nc); i += nc)
            {
                for (unsigned int q = 0; q < nc; q++)
                {
                    outbuffer[i + q + start * nc] = inbuffers[q][i / nc];
                }
            }
        }
    }

} // namespace TwkAudio
