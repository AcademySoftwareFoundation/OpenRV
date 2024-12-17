//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __TwkMath__Chromaticities__h__
#define __TwkMath__Chromaticities__h__
#include <iostream>
#include <TwkMath/Vec2.h>

namespace TwkMath
{

#if 0
Illuminant standardIlluminants[] = { 
    {"A",   0.44757, 0.40745 },
    {"B",   0.34842, 0.35161},
    {"C",   0.31006, 0.31616},
    {"D50",  0.34567, 0.35850},
    {"D55",  0.33242, 0.34743},
    {"D65",  0.31271, 0.32902},
    {"D75",  0.29902, 0.31485},
    {"E",   1.0/3.0, 1.0/3.0},
    {"F1",  0.31310, 0.33727},
    {"F2",  0.37208, 0.37529},
    {"F3",  0.40910, 0.39430},
    {"F4",  0.44018, 0.40329},
    {"F5",  0.31379, 0.34531},
    {"F6",  0.37790, 0.38835},
    {"F7",  0.31292, 0.32933},
    {"F8",  0.34588, 0.35875},
    {"F9",  0.37417, 0.37281},
    {"F10",  0.34609, 0.35986},
    {"F11",  0.38052, 0.37713},
    {"F12",  0.43695, 0.40441},
#endif

    template <class T> class Chromaticities
    {
    public:
        typedef Vec2<T> Vec;

        Chromaticities(const Vec& r = Vec(0.0f), const Vec& g = Vec(0.0f),
                       const Vec& b = Vec(0.0f), const Vec& w = Vec(0.0f))
            : red(r)
            , green(g)
            , blue(b)
            , white(w)
        {
        }

        Vec red;
        Vec green;
        Vec blue;
        Vec white;

        //
        //  601 525 lines from BT.601-7
        //

        static Chromaticities<T> Rec601_525()
        {
            return Chromaticities<T>(
                Vec2<T>(0.640, 0.340), Vec2<T>(0.310, 0.595),
                Vec2<T>(0.155, 0.070), Vec2<T>(0.3127, 0.3290)); // 6500K
        }

        //
        //  601 625 lines from BT.601-7
        //

        static Chromaticities<T> Rec601_625()
        {
            return Chromaticities<T>(
                Vec2<T>(0.640, 0.330), Vec2<T>(0.290, 0.600),
                Vec2<T>(0.150, 0.060), Vec2<T>(0.3127, 0.3290)); // 6500K
        }

        //
        //  from BT.709-5
        //

        static Chromaticities<T> Rec709()
        {
            return Chromaticities<T>(
                Vec2<T>(0.640, 0.330), Vec2<T>(0.3, 0.6), Vec2<T>(0.15, 0.06),
                Vec2<T>(
                    0.3127,
                    0.3290)); // 6500K Rec709 D65 is truncated form of CIE1931.
        }

        //
        //  from BT.2020 (Aug 2012)
        //

        static Chromaticities<T> Rec2020()
        {

            return Chromaticities<T>(
                Vec2<T>(0.708f, 0.292f), Vec2<T>(0.170f, 0.797f),
                Vec2<T>(0.131f, 0.046f), Vec2<T>(0.3127f, 0.3290f));
        }

        static Chromaticities<T> sRGB() // same as Rec709.
        {
            return Chromaticities<T>::Rec709();
        }

        static Chromaticities<T> P3()
        {
            return Chromaticities<T>(
                Vec2<T>(0.680, 0.320), Vec2<T>(0.265, 0.690),
                Vec2<T>(0.150, 0.06), Vec2<T>(0.314, 0.315)); // ~6300K
        }

        static Chromaticities<T> XYZ()
        {
            return Chromaticities<T>(
                Vec2<T>(1.0, 0.0), Vec2<T>(0.0, 1.0), Vec2<T>(0.0, 0.0),
                Vec2<T>(0.33333, 0.33333)); // 5454K i.e. equal energy
        }

        static Chromaticities<T> AdobeRGB()
        {
            return Chromaticities<T>(
                Vec2<T>(0.640, 0.330), Vec2<T>(0.210, 0.710),
                Vec2<T>(0.150, 0.06), Vec2<T>(0.3127, 0.3290));
        }

        static Chromaticities<T> SMPTE_C()
        {
            return Chromaticities<T>(
                Vec2<T>(0.630, 0.340), Vec2<T>(0.310, 0.595),
                Vec2<T>(0.155, 0.070), Vec2<T>(0.3127, 0.3290));
        }

        static Chromaticities<T> SMPTE_240M()
        {
            return Chromaticities<T>(Vec2<T>(0.67, 0.33), Vec2<T>(0.21, 0.71),
                                     Vec2<T>(0.15, 0.06),
                                     Vec2<T>(0.312713, 0.329016));
        }

        static Chromaticities<T> ACES()
        {
            return Chromaticities<T>(
                Vec2<T>(0.73470, 0.26530), Vec2<T>(0.00000, 1.00000),
                Vec2<T>(0.00010, -0.07700),
                Vec2<T>(0.32168, 0.33767)); // 6000K -- i.e. D60
        }

        static Chromaticities<T> DreamcolorFull()
        {
            return Chromaticities<T>(
                Vec2<T>(0.690, 0.300), Vec2<T>(0.205, 0.715),
                Vec2<T>(0.150, 0.045), Vec2<T>(0.3127, 0.3290)); // 6500K
        }

        //
        // From ALEXA LogC Curve - Usage in VFX doc.
        //
        // NOTE: not to be confused with ARRI "wide gamut" which includes
        // cross over and therefor is not scene referred (although its
        // linear).
        //

        static Chromaticities<T> ArriSceneReferred()
        {
            return Chromaticities<T>(
                Vec2<T>(0.6840, 0.3130), Vec2<T>(0.2210, 0.8480),
                Vec2<T>(0.0861, -0.1020), Vec2<T>(0.3127, 0.3290)); // 6500K
        }

        //
        // These Red colorspace chromaticities were obtain from
        // http://colour-science.org/posts/red-colourspaces-derivation/
        // which is based off the ACES distribution.
        //

        static Chromaticities<T> RedSpace()
        {
            // Unknown so assume rec709.
            return Chromaticities<T>::Rec709();
        }

        static Chromaticities<T> RedColor()
        {
            return Chromaticities<T>(Vec2<T>(0.699747001291, 0.329046930313),
                                     Vec2<T>(0.304264039024, 0.623641145129),
                                     Vec2<T>(0.134913961296, 0.0347174412813),
                                     Vec2<T>(0.321683289449, 0.337673447208));
        }

        static Chromaticities<T> RedColor2()
        {
            return Chromaticities<T>(Vec2<T>(0.878682510476, 0.32496400741),
                                     Vec2<T>(0.300888714367, 0.679054755791),
                                     Vec2<T>(0.0953986946056, -0.0293793268343),
                                     Vec2<T>(0.321683289449, 0.337673447208));
        }

        static Chromaticities<T> RedColor3()
        {
            return Chromaticities<T>(Vec2<T>(0.701181035906, 0.329014155583),
                                     Vec2<T>(0.300600304652, 0.683788834269),
                                     Vec2<T>(0.108154455624, -0.00868817578666),
                                     Vec2<T>(0.321683210353, 0.337673610062));
        }

        static Chromaticities<T> RedColor4()
        {
            return Chromaticities<T>(Vec2<T>(0.701180591892, 0.329013699116),
                                     Vec2<T>(0.300600395529, 0.683788824257),
                                     Vec2<T>(0.145331946229, 0.0516168036226),
                                     Vec2<T>(0.321683289449, 0.337673447208));
        }

        static Chromaticities<T> DragonColor()
        {
            return Chromaticities<T>(Vec2<T>(0.753044222785, 0.327830576682),
                                     Vec2<T>(0.299570228481, 0.700699321956),
                                     Vec2<T>(0.079642066735, -0.0549379510888),
                                     Vec2<T>(0.321683187724, 0.337673316035));
        }

        static Chromaticities<T> DragonColor2()
        {
            return Chromaticities<T>(Vec2<T>(0.753044491143, 0.327831029513),
                                     Vec2<T>(0.299570490451, 0.700699415614),
                                     Vec2<T>(0.145011584278, 0.0510971250879),
                                     Vec2<T>(0.321683210353, 0.337673610062));
        }
    };

    template <class T>
    bool operator==(const Chromaticities<T>& a, const Chromaticities<T>& b)
    {
        return a.red == b.red && a.green == b.green && a.blue == b.blue
               && a.white == b.white;
    }

    template <class T>
    bool operator!=(const Chromaticities<T>& a, const Chromaticities<T>& b)
    {
        return !(a == b);
    }

} // namespace TwkMath

#endif // __TwkMath__Chromaticities__h__
