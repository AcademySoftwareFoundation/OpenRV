//******************************************************************************
// Copyright (c) 2015 Autodesk Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <TwkAudio/AudioFormats.h>

#include <algorithm>
#include <iostream>
#include <string.h>

namespace TwkAudio
{

    bool ChannelsVector::hasAllChannels(const ChannelsVector& b) const
    {
        const ChannelsVector& a = *this;
        const size_t asize = a.size();
        const size_t bsize = b.size();
        if (asize != bsize)
            return false;
        else
        {
            for (int ch = 0; ch < asize; ch++)
            {
                ChannelsVector::const_iterator it;
                it = find(b.begin(), b.end(), a[ch]);
                if (it == b.end())
                {
                    return false;
                }
            }
        }
        return true;
    }

    bool ChannelsVector::identical(const ChannelsVector& a,
                                   const ChannelsVector& b) const
    {
        const size_t asize = a.size();
        const size_t bsize = b.size();
        if (asize != bsize)
            return false;
        return (memcmp(&a.front(), &b.front(), asize * sizeof(Channels))
                    ? false
                    : true);
    }

    std::string formatString(Format f)
    {
        std::string fstr;
        switch (f)
        {
        case Float32Format:
            fstr = "32 bit float";
            break;
        case Int32Format:
            fstr = "32 bit";
            break;
        case Int24Format:
            fstr = "24 bit";
            break;
        default:
        case Int16Format:
            fstr = "16 bit";
            break;
        case Int8Format:
            fstr = "8 bit";
            break;
        case UInt32_SMPTE272M_20Format:
            fstr = "20 bit (NV/SMPTE272M)";
            break;
        case UInt32_SMPTE299M_24Format:
            fstr = "24 bit (NV/SMPTE299M)";
            break;
        }
        return fstr;
    }

    size_t formatSizeInBytes(Format f)
    {
        size_t fsize;
        switch (f)
        {
        case Float32Format:
            fsize = sizeof(float);
            break;
        case Int32Format:
            fsize = sizeof(int);
            break;
        case Int24Format:
            fsize = sizeof(char) * 3;
            break;
        default:
        case Int16Format:
            fsize = sizeof(short);
            break;
        case Int8Format:
            fsize = sizeof(signed char);
            break;
        case UInt32_SMPTE272M_20Format:
            fsize = sizeof(unsigned int);
            break;
        case UInt32_SMPTE299M_24Format:
            fsize = sizeof(unsigned int);
            break;
        }
        return fsize;
    }

    std::string channelString(Channels channel)
    {
        std::string chstr;
        switch (channel)
        {
        case FrontLeft:
            chstr = "FL: front left";
            break;
        case FrontRight:
            chstr = "FR: front right";
            break;
        case FrontCenter:
            chstr = "FC: front center";
            break;
        case LowFrequency:
            chstr = "LF: low frequency";
            break;
        case BackLeft:
            chstr = "BL: back left";
            break;
        case BackRight:
            chstr = "BR: back right";
            break;
        case FrontLeftOfCenter:
            chstr = "FLC: front left center";
            break;
        case FrontRightOfCenter:
            chstr = "FRC: front right center";
            break;
        case BackCenter:
            chstr = "BC: back center";
            break;
        case SideLeft:
            chstr = "SL: side left";
            break;
        case SideRight:
            chstr = "SR: side right";
            break;
        case LeftHeight:
            chstr = "LH: left height";
            break;
        case RightHeight:
            chstr = "RH: right height";
            break;
        case Channel14:
            chstr = "CH14: channel 14";
            break;
        case Channel15:
            chstr = "CH15: channel 15";
            break;
        case Channel16:
            chstr = "CH16: channel 16";
            break;
        case UnknownLayout:
        default:
            chstr = "UKN: Unknown";
            break;
        }
        return chstr;
    }

    std::string layoutString(Layout layout)
    {
        std::string chstr;
        switch (layout)
        {
        case Mono_1:
            chstr = "Mono";
            break;
        case Stereo_2:
            chstr = "Stereo";
            break;
        case Stereo_2_1:
            chstr = "2.1";
            break;
        case Quad_4_0:
            chstr = "Quadrophonic";
            break;
        case Surround_4_1:
            chstr = "4.1";
            break;
        case Generic_4_1:
            chstr = "4.1 (Swap)";
            break;
        case Surround_5_1:
            chstr = "5.1";
            break;
        case Back_5_1:
            chstr = "5.1 (Back)";
            break;
        case Generic_5_1:
            chstr = "5.1 (Swap)";
            break;
        case AC3_5_1:
            chstr = "5.1 (AC3)";
            break;
        case DTS_5_1:
            chstr = "5.1 (DTS)";
            break;
        case AIFF_5_1:
            chstr = "5.1 (AIFF)";
            break;
        case Generic_6_1:
            chstr = "6.1";
            break;
        case SDDS_7_1:
            chstr = "7.1 (SDDS)";
            break;
        case Surround_7_1:
            chstr = "7.1";
            break;
        case Back_7_1:
            chstr = "7.1 (Back)";
            break;
        case Surround_9_1:
            chstr = "9.1";
            break;
        case Generic_16:
            chstr = "16";
            break;
        case UnknownLayout:
        default:
            chstr = "Unknown";
            break;
        }
        return chstr;
    }

    LayoutsVector channelLayouts(int channelCount)
    {
        LayoutsVector lv;
        switch (channelCount)
        {
        case 1:
            lv.push_back(Mono_1);
            break;
        case 2:
            lv.push_back(Stereo_2);
            break;
        case 3:
            lv.push_back(Stereo_2_1);
            break;
        case 4:
            lv.push_back(Quad_4_0);
            break;
        case 5:
            lv.push_back(Surround_4_1);
            lv.push_back(Generic_4_1);
            break;
        case 6:
            lv.push_back(Surround_5_1);
            lv.push_back(Back_5_1);
            lv.push_back(Generic_5_1);
            lv.push_back(AC3_5_1);
            lv.push_back(DTS_5_1);
            lv.push_back(AIFF_5_1);
            break;
        case 7:
            lv.push_back(Generic_6_1);
            break;
        case 8:
            lv.push_back(Surround_7_1);
            lv.push_back(Back_7_1);
            break;
            lv.push_back(SDDS_7_1);
            break;
        case 10:
            lv.push_back(Surround_9_1);
            break;
        case 16:
            lv.push_back(Generic_16);
            break;
        case 0:
        default:
            lv.push_back(UnknownLayout);
            break;
        }
        return lv;
    }

    Layout channelLayout(const ChannelsVector& cv)
    {
        if (cv == layoutChannels(Stereo_2))
        {
            return Stereo_2;
        }
        else if (cv == layoutChannels(Surround_5_1))
        {
            return Surround_5_1;
        }
        else if (cv == layoutChannels(Surround_7_1))
        {
            return Surround_7_1;
        }
        else if (cv == layoutChannels(Back_5_1))
        {
            return Back_5_1;
        }
        else if (cv == layoutChannels(Back_7_1))
        {
            return Back_7_1;
        }
        else if (cv == layoutChannels(Generic_5_1))
        {
            return Generic_5_1;
        }
        else if (cv == layoutChannels(AC3_5_1))
        {
            return AC3_5_1;
        }
        else if (cv == layoutChannels(DTS_5_1))
        {
            return DTS_5_1;
        }
        else if (cv == layoutChannels(AIFF_5_1))
        {
            return AIFF_5_1;
        }
        else if (cv == layoutChannels(Mono_1))
        {
            return Mono_1;
        }
        else if (cv == layoutChannels(Quad_4_0))
        {
            return Quad_4_0;
        }
        else if (cv == layoutChannels(Surround_4_1))
        {
            return Surround_4_1;
        }
        else if (cv == layoutChannels(Generic_4_1))
        {
            return Generic_4_1;
        }
        else if (cv == layoutChannels(SDDS_7_1))
        {
            return SDDS_7_1;
        }
        else if (cv == layoutChannels(Generic_16))
        {
            return Generic_16;
        }

        return UnknownLayout;
    }

    int channelsCount(Layout layout)
    {
        int count;
        switch (layout)
        {
        case Mono_1:
            count = 1;
            break;
        case Stereo_2:
            count = 2;
            break;
        case Stereo_2_1:
            count = 3;
            break;
        case Quad_4_0:
            count = 4;
            break;
        case Surround_4_1:
            count = 5;
            break;
        case Generic_4_1:
            count = 5;
            break;
        case Surround_5_1:
            count = 6;
            break;
        case Back_5_1:
            count = 6;
            break;
        case Generic_5_1:
            count = 6;
            break;
        case AC3_5_1:
            count = 6;
            break;
        case DTS_5_1:
            count = 6;
            break;
        case AIFF_5_1:
            count = 6;
            break;
        case Generic_6_1:
            count = 7;
            break;
        case Surround_7_1:
            count = 8;
            break;
        case SDDS_7_1:
            count = 8;
            break;
        case Back_7_1:
            count = 8;
            break;
        case Surround_9_1:
            count = 10;
            break;
        case Generic_16:
            count = 16;
            break;
        case UnknownLayout:
        default:
            std::cout << "AUDIO: channels format unsupported: Layout="
                      << (int)layout << std::endl;
            count = 0;
            break;
        }
        return count;
    }

    ChannelsVector layoutChannels(Layout layout)
    {
        ChannelsVector chv;
        switch (layout)
        {
        case Mono_1:
            chv.push_back(FrontCenter);
            break;
        case Stereo_2:
            chv.push_back(FrontLeft);
            chv.push_back(FrontRight);
            break;
        case Stereo_2_1:
            chv.push_back(FrontLeft);
            chv.push_back(FrontRight);
            chv.push_back(LowFrequency);
            break;
        case Quad_4_0:
            chv.push_back(FrontLeft);
            chv.push_back(FrontRight);
            chv.push_back(BackLeft);
            chv.push_back(BackRight);
            break;
        case Surround_4_1:
            chv.push_back(FrontLeft);
            chv.push_back(FrontRight);
            chv.push_back(FrontCenter);
            chv.push_back(LowFrequency);
            chv.push_back(BackCenter);
            break;
        case Generic_4_1:
            chv.push_back(FrontLeft);
            chv.push_back(FrontRight);
            chv.push_back(BackLeft);
            chv.push_back(BackRight);
            chv.push_back(LowFrequency);
            break;
        case Back_5_1:
            chv.push_back(FrontLeft);
            chv.push_back(FrontRight);
            chv.push_back(FrontCenter);
            chv.push_back(LowFrequency);
            chv.push_back(BackLeft);
            chv.push_back(BackRight);
            break;
        case Surround_5_1:
            chv.push_back(FrontLeft);
            chv.push_back(FrontRight);
            chv.push_back(FrontCenter);
            chv.push_back(LowFrequency);
            chv.push_back(SideLeft);
            chv.push_back(SideRight);
            break;
        case Generic_5_1:
            chv.push_back(FrontLeft);
            chv.push_back(FrontRight);
            chv.push_back(BackLeft);
            chv.push_back(BackRight);
            chv.push_back(FrontCenter);
            chv.push_back(LowFrequency);
            break;
        case AC3_5_1:
            chv.push_back(FrontLeft);
            chv.push_back(FrontCenter);
            chv.push_back(FrontRight);
            chv.push_back(SideLeft);
            chv.push_back(SideRight);
            chv.push_back(LowFrequency);
            break;
        case DTS_5_1:
            chv.push_back(FrontCenter);
            chv.push_back(FrontLeft);
            chv.push_back(FrontRight);
            chv.push_back(SideLeft);
            chv.push_back(SideRight);
            chv.push_back(LowFrequency);
            break;
        case AIFF_5_1:
            chv.push_back(FrontLeft);
            chv.push_back(BackLeft);
            chv.push_back(FrontCenter);
            chv.push_back(FrontRight);
            chv.push_back(BackRight);
            chv.push_back(LowFrequency);
            break;
        case Generic_6_1:
            chv.push_back(FrontLeft);
            chv.push_back(FrontRight);
            chv.push_back(FrontCenter);
            chv.push_back(LowFrequency);
            chv.push_back(BackLeft);
            chv.push_back(BackRight);
            chv.push_back(BackCenter);
            break;
        case Surround_7_1:
            chv.push_back(FrontLeft);
            chv.push_back(FrontRight);
            chv.push_back(FrontCenter);
            chv.push_back(LowFrequency);
            chv.push_back(SideLeft);
            chv.push_back(SideRight);
            chv.push_back(BackLeft);
            chv.push_back(BackRight);
            break;
        case SDDS_7_1:
            chv.push_back(FrontLeft);
            chv.push_back(FrontRight);
            chv.push_back(FrontCenter);
            chv.push_back(LowFrequency);
            chv.push_back(SideLeft);
            chv.push_back(SideRight);
            chv.push_back(FrontLeftOfCenter);
            chv.push_back(FrontRightOfCenter);
            break;
        case Back_7_1:
            chv.push_back(FrontLeft);
            chv.push_back(FrontRight);
            chv.push_back(FrontCenter);
            chv.push_back(LowFrequency);
            chv.push_back(BackLeft);
            chv.push_back(BackRight);
            chv.push_back(SideLeft);
            chv.push_back(SideRight);
            break;
        case Surround_9_1:
            chv.push_back(FrontLeft);
            chv.push_back(FrontRight);
            chv.push_back(FrontCenter);
            chv.push_back(LowFrequency);
            chv.push_back(BackLeft);
            chv.push_back(BackRight);
            chv.push_back(SideLeft);
            chv.push_back(SideRight);
            chv.push_back(LeftHeight);
            chv.push_back(RightHeight);
            break;
        case Generic_16: // Not sure?
            chv.push_back(FrontLeft);
            chv.push_back(FrontRight);
            chv.push_back(FrontCenter);
            chv.push_back(LowFrequency);
            chv.push_back(BackLeft);
            chv.push_back(BackRight);
            chv.push_back(SideLeft);
            chv.push_back(SideRight);
            chv.push_back(LeftHeight);
            chv.push_back(RightHeight);
            chv.push_back(FrontLeftOfCenter);
            chv.push_back(FrontRightOfCenter);
            chv.push_back(BackCenter);
            chv.push_back(Channel14);
            chv.push_back(Channel15);
            chv.push_back(Channel16);
            break;

        case UnknownLayout:
        default:
            break;
        }
        return chv;
    }

    void initChannelsMap(const ChannelsVector& a, const ChannelsVector& b,
                         ChannelsMap& chmap)
    {
        chmap.clear();

        if (a == b)
        {
            // Just copy the input to the output.
            for (int ch = 0; ch < b.size(); ch++)
            {
                ChannelMixState mix;
                ChannelState state;
                state.index = ch;
                if ((a[ch] & Center) == Center)
                {
                    state.weight = 0.5f;
                    mix.lefts.push_back(state);
                    mix.rights.push_back(state);
                }
                else if (b[ch] & Left)
                {
                    state.weight = 1.0f;
                    mix.lefts.push_back(state);
                }
                else
                {
                    state.weight = 1.0f;
                    mix.rights.push_back(state);
                }
                chmap[b[ch]] = mix;
            }
        }
        else if (a.size() == b.size())
        {
            if (b.hasAllChannels(a))
            {
                // The order of channels are mismtached
                for (int bch = 0; bch < b.size(); bch++)
                {
                    for (int ach = 0; ach < a.size(); ach++)
                    {
                        if (a[ach] == b[bch])
                        {
                            ChannelMixState mix;
                            ChannelState state;
                            state.index = ach;
                            if ((a[ach] & Center) == Center)
                            {
                                state.weight = 0.5f;
                                mix.lefts.push_back(state);
                                mix.rights.push_back(state);
                            }
                            else if (a[ach] & Left)
                            {
                                state.weight = 1.0f;
                                mix.lefts.push_back(state);
                            }
                            else
                            {
                                state.weight = 1.0f;
                                mix.rights.push_back(state);
                            }
                            chmap[b[bch]] = mix;
                            break;
                        }
                    }
                }
            }
            else
            {
                // Same number of channels but not all channels
                // are the same. In this case we just copy the
                // input to the output.
                for (int ch = 0; ch < b.size(); ch++)
                {
                    ChannelMixState mix;
                    ChannelState state;
                    state.index = ch;
                    if ((a[ch] & Center) == Center)
                    {
                        state.weight = 0.5f;
                        mix.lefts.push_back(state);
                        mix.rights.push_back(state);
                    }
                    else if (a[ch] & Left)
                    {
                        state.weight = 1.0f;
                        mix.lefts.push_back(state);
                    }
                    else
                    {
                        state.weight = 1.0f;
                        mix.rights.push_back(state);
                    }
                    chmap[b[ch]] = mix;
                }
            }
        }
        else
        {
            // Mix up or down channel cases.
            //
            // Cases:
            // 1. output=stereo input=anything; use mix down weight
            // 2. output=mono input=anything; use mix down weight
            // 3. output=n  n > 2; matched channel assignment only

            if (b.size() == 2) // Stereo Output Case
            {
                float normalizeFactor = 0.0f;
                for (int ach = 0; ach < a.size(); ach++)
                {
                    normalizeFactor += LEFT_CHANNEL_WEIGHT(a[ach]);
                }

                for (int bch = 0; bch < b.size(); bch++)
                {
                    ChannelMixState mix;
                    if ((b[bch] & Center) == Center)
                    {
                        for (int ach = 0; ach < a.size(); ach++)
                        {
                            float weight = LEFT_CHANNEL_WEIGHT(a[ach]);
                            if (weight > 0)
                            {
                                ChannelState state;
                                state.index = ach;
                                state.weight = weight / normalizeFactor;
                                mix.lefts.push_back(state);
                            }

                            weight = RIGHT_CHANNEL_WEIGHT(a[ach]);
                            if (weight > 0)
                            {
                                ChannelState state;
                                state.index = ach;
                                state.weight = weight / normalizeFactor;
                                mix.rights.push_back(state);
                            }
                        }
                    }
                    else if (b[bch] & Left)
                    {
                        for (int ach = 0; ach < a.size(); ach++)
                        {
                            float weight = LEFT_CHANNEL_WEIGHT(a[ach]);
                            if (weight > 0)
                            {
                                ChannelState state;
                                state.index = ach;
                                state.weight = weight / normalizeFactor;
                                mix.lefts.push_back(state);
                            }
                        }
                    }
                    else
                    {
                        for (int ach = 0; ach < a.size(); ach++)
                        {
                            float weight = RIGHT_CHANNEL_WEIGHT(a[ach]);
                            if (weight > 0)
                            {
                                ChannelState state;
                                state.index = ach;
                                state.weight = weight / normalizeFactor;
                                mix.rights.push_back(state);
                            }
                        }
                    }

                    chmap[b[bch]] = mix;
                }
            }
            else if (b.size() == 1) // Mono Output Case
            {
                ChannelMixState mix;

                float normalizeFactor = 0.0f;
                for (int ach = 0; ach < a.size(); ach++)
                {
                    normalizeFactor += LEFT_CHANNEL_WEIGHT(a[ach])
                                       + RIGHT_CHANNEL_WEIGHT(a[ach]);
                }

                for (int ach = 0; ach < a.size(); ach++)
                {
                    float weight = LEFT_CHANNEL_WEIGHT(a[ach]);
                    if (weight > 0)
                    {
                        ChannelState state;
                        state.index = ach;
                        state.weight = weight / normalizeFactor;
                        mix.lefts.push_back(state);
                    }

                    weight = RIGHT_CHANNEL_WEIGHT(a[ach]);
                    if (weight > 0)
                    {
                        ChannelState state;
                        state.index = ach;
                        state.weight = weight / normalizeFactor;
                        mix.rights.push_back(state);
                    }
                }
                chmap[b[0]] = mix;
            }
            else
            {
                // N output channel mix down or mix up case. N > 2

                ChannelMixState mixFrontCenter;
                ChannelMixState mixFrontLeft;
                ChannelMixState mixFrontRight;

                for (int bch = 0; bch < b.size(); bch++)
                {
                    bool foundMatch = false;

                    for (int ach = 0; ach < a.size(); ach++)
                    {
                        if ((a[ach] == FrontLeft)
                            && mixFrontCenter.lefts.empty())
                        {
                            ChannelState state;
                            state.index = ach;
                            state.weight = 1.0f;
                            mixFrontCenter.lefts.push_back(state);
                        }
                        else if ((a[ach] == FrontRight)
                                 && mixFrontCenter.rights.empty())
                        {
                            ChannelState state;
                            state.index = ach;
                            state.weight = 1.0f;
                            mixFrontCenter.rights.push_back(state);
                        }
                        else if (a[ach] == FrontCenter)
                        {
                            if (mixFrontLeft.lefts.empty())
                            {
                                ChannelState state;
                                state.index = ach;
                                state.weight = 1.0f;
                                mixFrontLeft.lefts.push_back(state);
                            }
                            if (mixFrontRight.rights.empty())
                            {
                                ChannelState state;
                                state.index = ach;
                                state.weight = 1.0f;
                                mixFrontRight.rights.push_back(state);
                            }
                        }

                        if (a[ach] == b[bch])
                        {
                            ChannelMixState mix;
                            ChannelState state;
                            state.index = ach;
                            if ((a[ach] & Center) == Center)
                            {
                                state.weight = 0.5f;
                                mix.lefts.push_back(state);
                                mix.rights.push_back(state);
                            }
                            else if (a[ach] & Left)
                            {
                                state.weight = 1.0f;
                                mix.lefts.push_back(state);
                            }
                            else
                            {
                                state.weight = 1.0f;
                                mix.rights.push_back(state);
                            }
                            chmap[b[bch]] = mix;
                            foundMatch = true;
                            break;
                        }
                    }

                    if (!foundMatch)
                    {
                        if ((b[bch] == FrontCenter)
                            && (mixFrontCenter.lefts.size() == 1)
                            && (mixFrontCenter.rights.size() == 1))
                        {
                            //
                            // We define an unmatched output FrontCenter channel
                            // as a combination of the input FrontLeft channel
                            // and FrontRight iff there wasnt an input
                            // FrontCenter channel.
                            //
                            chmap[FrontCenter] = mixFrontCenter;
                            continue;
                        }

                        if (b[bch] == FrontLeft)
                        {
                            //
                            // We define an unmatched output FrontLeft channel
                            // as the left portion of an input FrontCenter
                            // channel iff there wasnt an input FrontLeft
                            // channel.
                            //
                            chmap[FrontLeft] = mixFrontLeft;
                            continue;
                        }

                        if (b[bch] == FrontRight)
                        {
                            //
                            // We define an unmatched output FrontRight channel
                            // as the right portion of an input FrontCenter
                            // iff there wasnt an input FrontRight channel.
                            //
                            chmap[FrontRight] = mixFrontRight;
                            continue;
                        }

                        ChannelMixState mixEmpty;
                        chmap[b[bch]] = mixEmpty;
                    }
                }
            }
        }
    }

} // namespace TwkAudio
