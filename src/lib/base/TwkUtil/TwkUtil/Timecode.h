//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __TwkUtil__Timecode__h__
#define __TwkUtil__Timecode__h__
#include <TwkUtil/dll_defs.h>
#include <TwkMath/Time.h>
#include <iostream>
#include <vector>

namespace TwkUtil
{

    enum TCType
    {
        TCDropFrame,
        TCMonotonic, // Non-drop frame
        TCResidual   // Time + leftover in frames < round(fps)
    };

    struct TCSeparatedString
    {
        std::string hours;
        std::string minutes;
        std::string seconds;
        std::string frames;
    };

    typedef TwkMath::Time TCTime;
    typedef std::vector<std::string> StringVector;

    struct Timecode
    {
        Timecode(int rf = 0.0, TCTime rfps = 0.0, unsigned int f = 0,
                 unsigned int s = 0, unsigned int m = 0, unsigned int h = 0,
                 TCType t = TCResidual)
            : frames(f)
            , seconds(s)
            , minutes(m)
            , hours(h)
            , type(t)
            , padding(0)
            , rawFrame(rf)
            , rawFPS(rfps)
        {
        }

        int rawFrame;
        TCTime rawFPS;
        unsigned int frames;
        unsigned int seconds;
        unsigned int minutes;
        unsigned int hours;
        unsigned int padding;
        TCType type;
    };

    //
    //  Convert global frame number at fps to desired timecode type.
    //  NOTE: the first frame should be 0 not 1
    //

    TWKUTIL_EXPORT Timecode timecodeFromFrameNumber(int frame, TCTime fps,
                                                    TCType type = TCResidual);

    //
    //  Formatting
    //

    TWKUTIL_EXPORT std::string
    timecodeToString(const Timecode&,
                     int maxFrame = 0, // largest possible frame number or 0
                     char tsep = ':', char fsep = ':',
                     bool elideHours = true); // skip showing hours if possible

    TWKUTIL_EXPORT TCSeparatedString timecodeToSeparatedString(
        const Timecode&,
        int maxFrame = 0,        // largest possible frame number or 0
        bool elideHours = true); // skip hours if possible

} // namespace TwkUtil

#endif // __TwkUtil__Timecode__h__
