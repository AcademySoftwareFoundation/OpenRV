//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __TwkAudio__AudioFormats__h__
#define __TwkAudio__AudioFormats__h__

#include <TwkAudio/dll_defs.h>

#include <iostream>
#include <vector>
#include <map>

namespace TwkAudio
{

    ///  Audio Data Format Precision

    ///
    /// We really only want to deal with Float32Format when processing. At
    /// some point we convert for output to one of the others. The
    /// UInt32_SMPTE272M_20Format and UInt32_SMPTE299M_24Format are both
    /// used by the NVidia SDI ancillary data API. These aren't really
    /// 299M or 272M because those are constructed from 10 bit data words
    /// when streamed out along with other information. The nvidia
    /// hardware takes a much easier to deal with 32 bit word into which
    /// additional data is embedded.
    ///

    enum Format
    {
        Float32Format,
        Int32Format,
        Int24Format,
        Int16Format,
        Int8Format,
        UInt32_SMPTE272M_20Format, // 20 bit NVidia ANC output
        UInt32_SMPTE299M_24Format, // 24 bit NVidia ANC output
        UnknownFormat
    };

    ///
    ///  Named Channel Schemes
    ///

    enum Layout
    {
        UnknownLayout,

        Mono_1,       // FrontCenter,
        Stereo_2,     // FrontLeft      | FrontRight,
        Stereo_2_1,   // Stereo_2       | LowFrequency,
        Quad_4_0,     // Stereo_2       | BackLeft          | BackRight,
        Surround_4_1, // Stereo_2_1     | FrontCenter       | BackCenter,
        Generic_4_1,  // Quad_4_0       | LowFrequency,
        Surround_5_1, // Stereo_2       | FrontCenter       | LowFrequency, |
                      // SideLeft     | SideRight,
        Generic_5_1,  // Stereo_2       | BackLeft          | BackRight,    |
                      // FrontCenter, | LowFrequency,
        Back_5_1,     // Stereo_2       | FrontCenter       | LowFrequency, |
                      // BackLeft     | BackRight,
        Generic_6_1,  // Generic_5_1    | BackCenter,
        SDDS_7_1, // Surround_5_1   | FrontLeftOfCenter | FrontRightOfCenter,
        Surround_7_1, // Surround_5_1   | BackLeft          | BackRight,
        Back_7_1,     // Back_5_1       | SideLeft          | SideRight,
        Generic_9,    // Surround_7_1   | BackCenter,
        Surround_9_1, // Surround_7_1   | LeftHeight        | RightHeight,
        Generic_11,   // Base8+Heights  | Channel14,
        Generic_12,   // Base8+Heights  | FrontLeftOfCenter | Channel14,
        Generic_13,   // Base8+Heights  | FrontL/ROfCenter | Channel14,
        Generic_14, // Base8+Heights  | FrontL/ROfCenter+BackCenter | Channel14,
        Generic_15, // Base8+Heights  | FrontL/ROfCenter+BackCenter |
                    // Channel14+15,
        Generic_16, // Base8+Heights  | FrontL/ROfCenter+BackCenter |
                    // Channel14-16,
        Generic_17, // Generic_16     | Channel17,
        Generic_18, // Generic_17     | Channel18,
        Generic_19, // Generic_18     | Channel19,
        Generic_20, // Generic_19     | Channel20,
        Generic_21, // Generic_20     | Channel21,
        Generic_22, // Generic_21     | Channel22,
        Generic_23, // Generic_22     | Channel23,
        Generic_24, // Generic_23     | Channel24,
        Generic_25, // Generic_24     | Channel25,
        Generic_26, // Generic_25     | Channel26,
        Generic_27, // Generic_26     | Channel27,
        Generic_28, // Generic_27     | Channel28,
        Generic_29, // Generic_28     | Channel29,
        Generic_30, // Generic_29     | Channel30,
        Generic_31, // Generic_30     | Channel31,
        Generic_32, // Generic_31     | Channel32,

        AC3_5_1,  // FrontLeft   | FrontCenter | FrontRight | SideLeft |
                  // SideRight | LowFrequency
        DTS_5_1,  // FrontCenter | FrontLeft   | FrontRight | SideLeft |
                  // SideRight | LowFrequency
        AIFF_5_1, // FrontLeft   | BackLeft    | FrontCenter | FrontRight |
                  // BackRight | LowFrequency

        ///
        ///  Some Aliases
        ///
        AAC_5_1 = DTS_5_1,
        FLAC_5_1 = Back_5_1,
        WMA_5_1 = Back_5_1,
        WAV_5_1 = Back_5_1,
        DS_7_1 = Surround_7_1
    };

    typedef std::vector<Layout> LayoutsVector;

    ///
    /// These are right off of a wikipedia page:
    ///
    ///      http://en.wikipedia.org/wiki/Surround_sound
    ///

    enum ChannelMasks
    {
        Right = 1,
        Left = 2,
        Center = Left | Right
    };

    // We reserve the bottom 2 bits for the channel mask
    // so we can determine the left, right or center of a
    // channel from its enum value.
    // Next four bits determine the right weighting and the
    // following next four bits the left weighting.
    // This means all channel enums are shifted up by 10 bits.
    // For the weights, we divide by 10 so the bits store
    // a weight values as a multiple of 0.1.

#define SET_CHANNEL_VALUE(_ch, _lw, _rw) (_ch << 10 | _lw << 6 | _rw << 2)

#define LEFT_CHANNEL_WEIGHT(_ch) float((_ch >> 6) & 0xF) / 10.0f

#define RIGHT_CHANNEL_WEIGHT(_ch) float((_ch >> 2) & 0xF) / 10.0f

#define MIX_CHANNEL(_ch, _lvol, _rvol) \
    (_lvol * LEFT_CHANNEL_WEIGHT(_ch) + _rvol * RIGHT_CHANNEL_WEIGHT(_ch))

    enum Channels
    {
        UnknownChannel = 0,

        FrontLeft = SET_CHANNEL_VALUE(1, 10, 0) | Left,
        FrontRight = SET_CHANNEL_VALUE(2, 0, 10) | Right,
        FrontCenter = SET_CHANNEL_VALUE(3, 10, 10) | Center,
        LowFrequency = SET_CHANNEL_VALUE(4, 2, 2) | Center,
        BackLeft = SET_CHANNEL_VALUE(5, 10, 0) | Left,
        BackRight = SET_CHANNEL_VALUE(6, 0, 10) | Right,
        FrontLeftOfCenter = SET_CHANNEL_VALUE(7, 10, 5) | Center,
        FrontRightOfCenter = SET_CHANNEL_VALUE(8, 5, 10) | Center,
        BackCenter = SET_CHANNEL_VALUE(9, 10, 10) | Center,
        SideLeft = SET_CHANNEL_VALUE(10, 10, 0) | Left,
        SideRight = SET_CHANNEL_VALUE(11, 0, 10) | Right,
        LeftHeight = SET_CHANNEL_VALUE(12, 10, 0) | Left,
        RightHeight = SET_CHANNEL_VALUE(13, 0, 10) | Right,
        Channel14 = SET_CHANNEL_VALUE(14, 10, 10) | Center,
        Channel15 = SET_CHANNEL_VALUE(15, 10, 10) | Center,
        Channel16 =
            SET_CHANNEL_VALUE(16, 10, 10) | Center, // SDI has 16 channels
        Channel17 = SET_CHANNEL_VALUE(17, 10, 10) | Center,
        Channel18 = SET_CHANNEL_VALUE(18, 10, 10) | Center,
        Channel19 = SET_CHANNEL_VALUE(19, 10, 10) | Center,
        Channel20 = SET_CHANNEL_VALUE(20, 10, 10) | Center,
        Channel21 = SET_CHANNEL_VALUE(21, 10, 10) | Center,
        Channel22 = SET_CHANNEL_VALUE(22, 10, 10) | Center,
        Channel23 = SET_CHANNEL_VALUE(23, 10, 10) | Center,
        Channel24 = SET_CHANNEL_VALUE(24, 10, 10) | Center,
        Channel25 = SET_CHANNEL_VALUE(25, 10, 10) | Center,
        Channel26 = SET_CHANNEL_VALUE(26, 10, 10) | Center,
        Channel27 = SET_CHANNEL_VALUE(27, 10, 10) | Center,
        Channel28 = SET_CHANNEL_VALUE(28, 10, 10) | Center,
        Channel29 = SET_CHANNEL_VALUE(29, 10, 10) | Center,
        Channel30 = SET_CHANNEL_VALUE(30, 10, 10) | Center,
        Channel31 = SET_CHANNEL_VALUE(31, 10, 10) | Center,
        Channel32 = SET_CHANNEL_VALUE(32, 10, 10) | Center
    };

    class TWKAUDIO_EXPORT ChannelsVector : public std::vector<Channels>
    {
    public:
        bool operator==(const ChannelsVector& other) const
        {
            return identical(*this, other);
        }

        bool operator!=(const ChannelsVector& other) const
        {
            return !identical(*this, other);
        }

        bool hasAllChannels(const ChannelsVector& b) const;

    private:
        bool identical(const ChannelsVector& a, const ChannelsVector& b) const;
    };

    struct TWKAUDIO_EXPORT ChannelState
    {
        int index;
        float weight;
    };

    struct TWKAUDIO_EXPORT ChannelMixState
    {
        std::vector<ChannelState> lefts;
        std::vector<ChannelState> rights;
    };

    typedef std::map<Channels, ChannelMixState> ChannelsMap;

    ///  Helper functions

    TWKAUDIO_EXPORT LayoutsVector channelLayouts(int channelCount);
    TWKAUDIO_EXPORT Layout channelLayout(const ChannelsVector& cv);
    TWKAUDIO_EXPORT int channelsCount(Layout);
    TWKAUDIO_EXPORT ChannelsVector layoutChannels(Layout layout);
    TWKAUDIO_EXPORT std::string channelString(Channels channel);
    TWKAUDIO_EXPORT std::string layoutString(Layout layout);

    TWKAUDIO_EXPORT std::string formatString(Format f);
    TWKAUDIO_EXPORT size_t formatSizeInBytes(Format f);

    TWKAUDIO_EXPORT void initChannelsMap(const ChannelsVector& a,
                                         const ChannelsVector& b,
                                         ChannelsMap& chmap);

} // namespace TwkAudio

#endif // __TwkAudio__AudioFormats__h__
