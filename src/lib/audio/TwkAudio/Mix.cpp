//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkAudio/Mix.h>
#include <TwkAudio/AudioFormats.h>
#include <TwkAudio/Interlace.h>
#include <iostream>
#include <string.h>

namespace TwkAudio
{
    using namespace TwkMath;
    using namespace std;

    void mixChannels(const AudioBuffer& in, AudioBuffer& out,
                     const float lVolume, const float rVolume,
                     const bool compose)
    {
        const size_t inSize = in.size();
        const size_t outSize = out.size();

        if (inSize != outSize)
        {
            cout << "WARNING: audio mix: inSize = " << inSize
                 << ", outSize = " << outSize << endl;
            return;
        }

        // XXX Uncomment to skip mix
        // cerr << "MIX inChans: " << in.numChannels() << " inSize: " <<
        // in.size() << endl; memcpy(out.pointer(), in.pointer(),
        // in.numChannels() * in.size() * sizeof(float)); return;

        const float* ip = in.pointer();
        float* op = out.pointer();

        if ((in.channels() == out.channels()) && (lVolume == rVolume))
        {
            size_t sampleCount = inSize * in.numChannels();
            if (compose)
            {
                for (size_t s = 0; s < sampleCount; s++)
                {
                    (*op++) += (*ip++) * lVolume;
                }
            }
            else
            {
                for (size_t s = 0; s < sampleCount; s++)
                {
                    (*op++) = (*ip++) * lVolume;
                }
            }
        }
        else
        {
            ChannelsMap chmap;
            initChannelsMap(in.channels(), out.channels(), chmap);
            size_t sampleCount = inSize;
            if (compose)
            {
                for (size_t s = 0; s < sampleCount; s++)
                {
                    for (int och = 0; och < out.channels().size(); och++)
                    {
                        const ChannelMixState& cmixState =
                            chmap[out.channels()[och]];

                        float lMix = 0.0f;
                        const std::vector<ChannelState>& leftChs =
                            cmixState.lefts;
                        for (int n = 0; n < leftChs.size(); ++n)
                        {
                            const ChannelState& chState = leftChs[n];
                            lMix += chState.weight * ip[chState.index];
                        }

                        float rMix = 0.0f;
                        const std::vector<ChannelState>& rightCh =
                            cmixState.rights;
                        for (int n = 0; n < rightCh.size(); ++n)
                        {
                            const ChannelState& chState = rightCh[n];
                            rMix += chState.weight * ip[chState.index];
                        }

                        (*op++) += lMix * lVolume + rMix * rVolume;
                    }

                    ip += in.numChannels();
                }
            }
            else
            {
                for (size_t s = 0; s < sampleCount; s++)
                {
                    for (int och = 0; och < out.channels().size(); och++)
                    {
                        const ChannelMixState& cmixState =
                            chmap[out.channels()[och]];

                        float lMix = 0.0f;
                        const std::vector<ChannelState>& leftChs =
                            cmixState.lefts;
                        for (int n = 0; n < leftChs.size(); ++n)
                        {
                            const ChannelState& chState = leftChs[n];
                            lMix += chState.weight * ip[chState.index];
                        }

                        float rMix = 0.0f;
                        const std::vector<ChannelState>& rightCh =
                            cmixState.rights;
                        for (int n = 0; n < rightCh.size(); ++n)
                        {
                            const ChannelState& chState = rightCh[n];
                            rMix += chState.weight * ip[chState.index];
                        }

                        (*op++) = lMix * lVolume + rMix * rVolume;
                    }

                    ip += in.numChannels();
                }
            }
        }
    }

} // namespace TwkAudio
