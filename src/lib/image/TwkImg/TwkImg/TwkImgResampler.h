//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkImgResampler_h_
#define _TwkImgResampler_h_

#include <TwkImg/TwkImgImage.h>
#include <TwkMath/Function.h>
#include <TwkMath/Vec2.h>
#include <TwkMath/Box.h>
#include <TwkMath/Math.h>
#include <assert.h>
#include <string.h>

namespace TwkImg
{

    //******************************************************************************
    template <class COLOR, bool CHECK = true> class NearNbRsmp
    {
    public:
        NearNbRsmp();

        // Return the radius of the kernel.
        // This is used by image processing utilities
        // to figure out how many pixels to enlarge
        // requests by.
        float radius() const;

        // Sample a point in relative coordinates
        // Return the percent coverage of the point
        // based on the given kernel.
        float sample(const Image<COLOR>& img, const TwkMath::Vec2f& loc,
                     COLOR& samp) const;

        // Sample a point in absolute coordinates
        float sampleAbs(const Image<COLOR>& img, const TwkMath::Vec2f& abs,
                        COLOR& samp) const;
    };

    //******************************************************************************
    template <class COLOR, bool CHECK = true> class BiLinearRsmp
    {
    public:
        BiLinearRsmp();

        // Return the radius of the kernel.
        // This is used by image processing utilities
        // to figure out how many pixels to enlarge
        // requests by.
        float radius() const;

        // Sample a point in relative coordinates
        float sample(const Image<COLOR>& img, const TwkMath::Vec2f& loc,
                     COLOR& samp) const;

        // Sample a point in absolute coordinates
        float sampleAbs(const Image<COLOR>& img, const TwkMath::Vec2f& abs,
                        COLOR& samp) const;
    };

    //******************************************************************************
    // TEMPLATE AND INLINE FUNCTIONS
    //******************************************************************************

    //******************************************************************************
    template <class COLOR, bool CHECK>
    inline NearNbRsmp<COLOR, CHECK>::NearNbRsmp()
    {
        // Nothing
    }

    //******************************************************************************
    template <class COLOR, bool CHECK>
    inline float NearNbRsmp<COLOR, CHECK>::radius() const
    {
        // Technically, the radius of a NearNbRsmp is
        // 0.5f pixels, because the furthest distance from
        // a pixel location that we'll ask for pixels is 0.5f.
        // However, because we know that image processing functions
        // round their requests up to whole pixels, we know that
        // our requests will always be included in the rectangles
        // they create when they round up. We don't want those rectangles
        // to be unnecessarily enlarged by a pixel, so we cheat
        // and say that our radius is really 0.0f;
        return 0.0f;
    }

    //******************************************************************************
    template <class COLOR, bool CHECK>
    float NearNbRsmp<COLOR, CHECK>::sample(const Image<COLOR>& img,
                                           const TwkMath::Vec2f& loc,
                                           COLOR& samp) const
    {
        // Create a static black pixel
        // Part of the COLOR specification is that a default
        // constructor produces a "black" color.
        static COLOR blackPixel;

        TwkMath::Vec2i rloc((int)(TwkMath::Math<float>::round(loc[0])),
                            (int)(TwkMath::Math<float>::round(loc[1])));

        if (CHECK)
        {
            if (!(img.bounds().intersects(rloc)))
            {
                samp = blackPixel;
                return 0.0f;
            }
            else
            {
                samp = img.pixel(rloc);
                return 1.0f;
            }
        }
        else
        {
            samp = img.pixel(rloc);
            return 1.0f;
        }
    }

    //******************************************************************************
    template <class COLOR, bool CHECK>
    inline float NearNbRsmp<COLOR, CHECK>::sampleAbs(const Image<COLOR>& img,
                                                     const TwkMath::Vec2f& loc,
                                                     COLOR& samp) const
    {
        return sample(img,
                      loc
                          - TwkMath::Vec2f((float)(img.origin()[0]),
                                           (float)(img.origin()[1])),
                      samp);
    }

    //******************************************************************************
    template <class COLOR, bool CHECK>
    inline BiLinearRsmp<COLOR, CHECK>::BiLinearRsmp()
    {
        // Nothing
    }

    //******************************************************************************
    template <class COLOR, bool CHECK>
    inline float BiLinearRsmp<COLOR, CHECK>::radius() const
    {
        return 0.5f;
    }

    //******************************************************************************
    template <class COLOR, bool CHECK>
    float BiLinearRsmp<COLOR, CHECK>::sample(const Image<COLOR>& img,
                                             const TwkMath::Vec2f& loc,
                                             COLOR& samp) const
    {
        // Allow for samples outside the range of this Image, or tile.
        const int minX = int(floorf(loc[0]));
        const int maxX = minX + 1;
        const int minY = int(floorf(loc[1]));
        const int maxY = minY + 1;

        // Interpolate top & bottom
        const float tx = loc[0] - (float)minX;
        const float ty = loc[1] - (float)minY;
        const float txty = tx * ty;

        if (CHECK)
        {
            // Do something like this.
            const bool validMinX = (minX >= 0 && minX < img.size()[0]);
            const bool validMaxX = (maxX >= 0 && maxX < img.size()[0]);
            const bool validMinY = (minY >= 0 && minY < img.size()[1]);
            const bool validMaxY = (maxY >= 0 && maxY < img.size()[1]);

            // Initialize the sample.
            memset((void*)(&samp), 0, sizeof(COLOR));
            float cov = 0.0f;

            if (validMinX)
            {
                if (validMinY)
                {
                    // Left bottom is good.
                    const float leftBottomFilt = (1.0f - tx - ty + txty);
                    cov += leftBottomFilt;
                    samp += img.pixel(minX, minY) * leftBottomFilt;
                }

                if (validMaxY)
                {
                    // Left top is good.
                    const float leftTopFilt = (ty - txty);
                    cov += leftTopFilt;
                    samp += img.pixel(minX, maxY) * leftTopFilt;
                }
            }

            if (validMaxX)
            {
                if (validMinY)
                {
                    // Right bottom is good.
                    const float rightBottomFilt = (tx - txty);
                    cov += rightBottomFilt;
                    samp += img.pixel(maxX, minY) * rightBottomFilt;
                }

                if (validMaxY)
                {
                    // Right top is good.
                    // Right top filt is exactly equal to txty,
                    // so in the interest of speed we avoid the intermediate
                    // variable.
                    cov += txty;
                    samp += img.pixel(maxX, maxY) * txty;
                }
            }

            return cov;
        }
        else
        {
            const COLOR* leftBottom = &(img.pixel(minX, minY));
            const COLOR* leftTop = leftBottom + img.stride();
            const COLOR* rightBottom = leftBottom + 1;
            const COLOR* rightTop = leftTop + 1;

            samp = (*leftBottom) * (1.0f - tx - ty + txty)
                   + (*leftTop) * (ty - txty) + (*rightBottom) * (tx - txty)
                   + (*rightTop) * txty;
            return 1.0f;
        }
    }

    //******************************************************************************
    template <class COLOR, bool CHECK>
    inline float BiLinearRsmp<COLOR, CHECK>::sampleAbs(
        const Image<COLOR>& img, const TwkMath::Vec2f& loc, COLOR& samp) const
    {
        return sample(img,
                      loc
                          - TwkMath::Vec2f((float)(img.origin()[0]),
                                           (float)(img.origin()[1])),
                      samp);
    }

} // End namespace TwkImg

#endif
