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
        case Generic_9:
            chstr = "9.0 (Generic)";
            break;
        case Surround_9_1:
            chstr = "9.1";
            break;
        case Generic_11:
            chstr = "11.0 (Generic)";
            break;
        case Generic_12:
            chstr = "12.0 (Generic)";
            break;
        case Generic_13:
            chstr = "13.0 (Generic)";
            break;
        case Generic_14:
            chstr = "14.0 (Generic)";
            break;
        case Generic_15:
            chstr = "15.0 (Generic)";
            break;
        case Generic_16:
            chstr = "16.0 (Generic)";
            break;
        case Generic_17:
            chstr = "17.0 (Generic)";
            break;
        case Generic_18:
            chstr = "18.0 (Generic)";
            break;
        case Generic_19:
            chstr = "19.0 (Generic)";
            break;
        case Generic_20:
            chstr = "20.0 (Generic)";
            break;
        case Generic_21:
            chstr = "21.0 (Generic)";
            break;
        case Generic_22:
            chstr = "22.0 (Generic)";
            break;
        case Generic_23:
            chstr = "23.0 (Generic)";
            break;
        case Generic_24:
            chstr = "24.0 (Generic)";
            break;
        case Generic_25:
            chstr = "25.0 (Generic)";
            break;
        case Generic_26:
            chstr = "26.0 (Generic)";
            break;
        case Generic_27:
            chstr = "27.0 (Generic)";
            break;
        case Generic_28:
            chstr = "28.0 (Generic)";
            break;
        case Generic_29:
            chstr = "29.0 (Generic)";
            break;
        case Generic_30:
            chstr = "30.0 (Generic)";
            break;
        case Generic_31:
            chstr = "31.0 (Generic)";
            break;
        case Generic_32:
            chstr = "32.0 (Generic)";
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
            lv.push_back(SDDS_7_1);
            break;
        case 9:
            lv.push_back(Generic_9);
            break;
        case 10:
            lv.push_back(Surround_9_1);
            break;
        case 11:
            lv.push_back(Generic_11);
            break;
        case 12:
            lv.push_back(Generic_12);
            break;
        case 13:
            lv.push_back(Generic_13);
            break;
        case 14:
            lv.push_back(Generic_14);
            break;
        case 15:
            lv.push_back(Generic_15);
            break;
        case 16:
            lv.push_back(Generic_16);
            break;
        case 17:
            lv.push_back(Generic_17);
            break;
        case 18:
            lv.push_back(Generic_18);
            break;
        case 19:
            lv.push_back(Generic_19);
            break;
        case 20:
            lv.push_back(Generic_20);
            break;
        case 21:
            lv.push_back(Generic_21);
            break;
        case 22:
            lv.push_back(Generic_22);
            break;
        case 23:
            lv.push_back(Generic_23);
            break;
        case 24:
            lv.push_back(Generic_24);
            break;
        case 25:
            lv.push_back(Generic_25);
            break;
        case 26:
            lv.push_back(Generic_26);
            break;
        case 27:
            lv.push_back(Generic_27);
            break;
        case 28:
            lv.push_back(Generic_28);
            break;
        case 29:
            lv.push_back(Generic_29);
            break;
        case 30:
            lv.push_back(Generic_30);
            break;
        case 31:
            lv.push_back(Generic_31);
            break;
        case 32:
            lv.push_back(Generic_32);
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
        else if (cv == layoutChannels(Generic_9))
        {
            return Generic_9;
        }
        else if (cv == layoutChannels(Generic_11))
        {
            return Generic_11;
        }
        else if (cv == layoutChannels(Generic_12))
        {
            return Generic_12;
        }
        else if (cv == layoutChannels(Generic_13))
        {
            return Generic_13;
        }
        else if (cv == layoutChannels(Generic_14))
        {
            return Generic_14;
        }
        else if (cv == layoutChannels(Generic_15))
        {
            return Generic_15;
        }
        else if (cv == layoutChannels(Generic_16))
        {
            return Generic_16;
        }
        else if (cv == layoutChannels(Generic_17))
        {
            return Generic_17;
        }
        else if (cv == layoutChannels(Generic_18))
        {
            return Generic_18;
        }
        else if (cv == layoutChannels(Generic_19))
        {
            return Generic_19;
        }
        else if (cv == layoutChannels(Generic_20))
        {
            return Generic_20;
        }
        else if (cv == layoutChannels(Generic_21))
        {
            return Generic_21;
        }
        else if (cv == layoutChannels(Generic_22))
        {
            return Generic_22;
        }
        else if (cv == layoutChannels(Generic_23))
        {
            return Generic_23;
        }
        else if (cv == layoutChannels(Generic_24))
        {
            return Generic_24;
        }
        else if (cv == layoutChannels(Generic_25))
        {
            return Generic_25;
        }
        else if (cv == layoutChannels(Generic_26))
        {
            return Generic_26;
        }
        else if (cv == layoutChannels(Generic_27))
        {
            return Generic_27;
        }
        else if (cv == layoutChannels(Generic_28))
        {
            return Generic_28;
        }
        else if (cv == layoutChannels(Generic_29))
        {
            return Generic_29;
        }
        else if (cv == layoutChannels(Generic_30))
        {
            return Generic_30;
        }
        else if (cv == layoutChannels(Generic_31))
        {
            return Generic_31;
        }
        else if (cv == layoutChannels(Generic_32))
        {
            return Generic_32;
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
        case Generic_9:
            count = 9;
            break;
        case Surround_9_1:
            count = 10;
            break;
        case Generic_11:
            count = 11;
            break;
        case Generic_12:
            count = 12;
            break;
        case Generic_13:
            count = 13;
            break;
        case Generic_14:
            count = 14;
            break;
        case Generic_15:
            count = 15;
            break;
        case Generic_16:
            count = 16;
            break;
        case Generic_17:
            count = 17;
            break;
        case Generic_18:
            count = 18;
            break;
        case Generic_19:
            count = 19;
            break;
        case Generic_20:
            count = 20;
            break;
        case Generic_21:
            count = 21;
            break;
        case Generic_22:
            count = 22;
            break;
        case Generic_23:
            count = 23;
            break;
        case Generic_24:
            count = 24;
            break;
        case Generic_25:
            count = 25;
            break;
        case Generic_26:
            count = 26;
            break;
        case Generic_27:
            count = 27;
            break;
        case Generic_28:
            count = 28;
            break;
        case Generic_29:
            count = 29;
            break;
        case Generic_30:
            count = 30;
            break;
        case Generic_31:
            count = 31;
            break;
        case Generic_32:
            count = 32;
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
        case Generic_9:
            // Surround_7_1 + BackCenter
            chv.push_back(FrontLeft);
            chv.push_back(FrontRight);
            chv.push_back(FrontCenter);
            chv.push_back(LowFrequency);
            chv.push_back(BackLeft);
            chv.push_back(BackRight);
            chv.push_back(SideLeft);
            chv.push_back(SideRight);
            chv.push_back(BackCenter);
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
        case Generic_11:
            // Surround_9_1 + Channel14
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
            chv.push_back(Channel14);
            break;
        case Generic_12:
            // Base 8 + Heights + FrontLeftOfCenter + Channel14
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
            chv.push_back(Channel14);
            break;
        case Generic_13:
            // Base 8 + Heights + FrontLeftOfCenter + FrontRightOfCenter +
            // Channel14
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
            chv.push_back(Channel14);
            break;
        case Generic_14:
            // Base 8 + Heights + FrontLeftOfCenter + FrontRightOfCenter +
            // BackCenter + Channel14
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
            break;
        case Generic_15:
            // Base 8 + Heights + FrontLeftOfCenter + FrontRightOfCenter +
            // BackCenter + Channel14 + Channel15
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
            break;
        case Generic_16:
            // Base 8 + Heights + FrontLeftOfCenter + FrontRightOfCenter +
            // BackCenter + Channel14 + Channel15 + Channel16
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
        case Generic_17:
            // Generic_16 + Channel17
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
            chv.push_back(Channel17);
            break;
        case Generic_18:
            // Generic_17 + Channel18
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
            chv.push_back(Channel17);
            chv.push_back(Channel18);
            break;
        case Generic_19:
            // Generic_18 + Channel19
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
            chv.push_back(Channel17);
            chv.push_back(Channel18);
            chv.push_back(Channel19);
            break;
        case Generic_20:
            // Generic_19 + Channel20
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
            chv.push_back(Channel17);
            chv.push_back(Channel18);
            chv.push_back(Channel19);
            chv.push_back(Channel20);
            break;
        case Generic_21:
            // Generic_20 + Channel21
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
            chv.push_back(Channel17);
            chv.push_back(Channel18);
            chv.push_back(Channel19);
            chv.push_back(Channel20);
            chv.push_back(Channel21);
            break;
        case Generic_22:
            // Generic_21 + Channel22
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
            chv.push_back(Channel17);
            chv.push_back(Channel18);
            chv.push_back(Channel19);
            chv.push_back(Channel20);
            chv.push_back(Channel21);
            chv.push_back(Channel22);
            break;
        case Generic_23:
            // Generic_22 + Channel23
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
            chv.push_back(Channel17);
            chv.push_back(Channel18);
            chv.push_back(Channel19);
            chv.push_back(Channel20);
            chv.push_back(Channel21);
            chv.push_back(Channel22);
            chv.push_back(Channel23);
            break;
        case Generic_24:
            // Generic_23 + Channel24
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
            chv.push_back(Channel17);
            chv.push_back(Channel18);
            chv.push_back(Channel19);
            chv.push_back(Channel20);
            chv.push_back(Channel21);
            chv.push_back(Channel22);
            chv.push_back(Channel23);
            chv.push_back(Channel24);
            break;
        case Generic_25:
            // Generic_24 + Channel25
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
            chv.push_back(Channel17);
            chv.push_back(Channel18);
            chv.push_back(Channel19);
            chv.push_back(Channel20);
            chv.push_back(Channel21);
            chv.push_back(Channel22);
            chv.push_back(Channel23);
            chv.push_back(Channel24);
            chv.push_back(Channel25);
            break;
        case Generic_26:
            // Generic_25 + Channel26
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
            chv.push_back(Channel17);
            chv.push_back(Channel18);
            chv.push_back(Channel19);
            chv.push_back(Channel20);
            chv.push_back(Channel21);
            chv.push_back(Channel22);
            chv.push_back(Channel23);
            chv.push_back(Channel24);
            chv.push_back(Channel25);
            chv.push_back(Channel26);
            break;
        case Generic_27:
            // Generic_26 + Channel27
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
            chv.push_back(Channel17);
            chv.push_back(Channel18);
            chv.push_back(Channel19);
            chv.push_back(Channel20);
            chv.push_back(Channel21);
            chv.push_back(Channel22);
            chv.push_back(Channel23);
            chv.push_back(Channel24);
            chv.push_back(Channel25);
            chv.push_back(Channel26);
            chv.push_back(Channel27);
            break;
        case Generic_28:
            // Generic_27 + Channel28
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
            chv.push_back(Channel17);
            chv.push_back(Channel18);
            chv.push_back(Channel19);
            chv.push_back(Channel20);
            chv.push_back(Channel21);
            chv.push_back(Channel22);
            chv.push_back(Channel23);
            chv.push_back(Channel24);
            chv.push_back(Channel25);
            chv.push_back(Channel26);
            chv.push_back(Channel27);
            chv.push_back(Channel28);
            break;
        case Generic_29:
            // Generic_28 + Channel29
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
            chv.push_back(Channel17);
            chv.push_back(Channel18);
            chv.push_back(Channel19);
            chv.push_back(Channel20);
            chv.push_back(Channel21);
            chv.push_back(Channel22);
            chv.push_back(Channel23);
            chv.push_back(Channel24);
            chv.push_back(Channel25);
            chv.push_back(Channel26);
            chv.push_back(Channel27);
            chv.push_back(Channel28);
            chv.push_back(Channel29);
            break;
        case Generic_30:
            // Generic_29 + Channel30
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
            chv.push_back(Channel17);
            chv.push_back(Channel18);
            chv.push_back(Channel19);
            chv.push_back(Channel20);
            chv.push_back(Channel21);
            chv.push_back(Channel22);
            chv.push_back(Channel23);
            chv.push_back(Channel24);
            chv.push_back(Channel25);
            chv.push_back(Channel26);
            chv.push_back(Channel27);
            chv.push_back(Channel28);
            chv.push_back(Channel29);
            chv.push_back(Channel30);
            break;
        case Generic_31:
            // Generic_30 + Channel31
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
            chv.push_back(Channel17);
            chv.push_back(Channel18);
            chv.push_back(Channel19);
            chv.push_back(Channel20);
            chv.push_back(Channel21);
            chv.push_back(Channel22);
            chv.push_back(Channel23);
            chv.push_back(Channel24);
            chv.push_back(Channel25);
            chv.push_back(Channel26);
            chv.push_back(Channel27);
            chv.push_back(Channel28);
            chv.push_back(Channel29);
            chv.push_back(Channel30);
            chv.push_back(Channel31);
            break;
        case Generic_32:
            // Generic_31 + Channel32
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
            chv.push_back(Channel17);
            chv.push_back(Channel18);
            chv.push_back(Channel19);
            chv.push_back(Channel20);
            chv.push_back(Channel21);
            chv.push_back(Channel22);
            chv.push_back(Channel23);
            chv.push_back(Channel24);
            chv.push_back(Channel25);
            chv.push_back(Channel26);
            chv.push_back(Channel27);
            chv.push_back(Channel28);
            chv.push_back(Channel29);
            chv.push_back(Channel30);
            chv.push_back(Channel31);
            chv.push_back(Channel32);
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
