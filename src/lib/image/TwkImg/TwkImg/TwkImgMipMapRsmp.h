//******************************************************************************
// Copyright (c) 2001-2003 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkImgMipMapRsmp_h_
#define _TwkImgMipMapRsmp_h_

#include <TwkImg/TwkImgImage.h>
#include <TwkImg/TwkImgMipMap.h>
#include <TwkMath/Math.h>
#include <TwkMath/Function.h>
#include <TwkMath/Vec2.h>
#include <TwkMath/Vec3.h>
#include <TwkMath/Box.h>
#include <math.h>
#include <assert.h>

namespace TwkImg
{

    //******************************************************************************
    // This mip map resampler uses two resamplers - one for
    // when the sample radius is less than or equal to one, and
    // one for when the sample radius is greater than one. The
    // resampler finds the lod below and above the fractional lod
    // given by the sample radius. This particular resampler
    // assumes that the samples are circular. More advanced resamplers
    // might treat samples as rotated ellipses - for example,
    // the Elliptical Weighted Average (EWA) technique by Heckbert,
    // or the Fast Elliptical Line (Feline) technique by Compaq.
    // I'll worry about this later.
    template <class COLOR, class MIN_RSMP, class MAX_RSMP> class MmLinRsmp
    {
    public:
        MmLinRsmp(MIN_RSMP& minRsmp, MAX_RSMP& maxRsmp);

        // Sample a point in relative coordinates.
        float sample(const MipMap<COLOR>& mip, const TwkMath::Vec2f& loc,
                     float minSampRad, float maxSampRad, float lodBias,
                     COLOR& samp) const;

        // Sample a point in absolute coordinates.
        float sampleAbs(const MipMap<COLOR>& mip, const TwkMath::Vec2f& loc,
                        float minSampRad, float maxSampRad, float lodBias,
                        COLOR& samp) const;

    protected:
        // Contain by reference, for now. This may change.
        MIN_RSMP& m_minRsmp;
        MAX_RSMP& m_maxRsmp;
    };

    // Use unnamed namespace to restrict coverage function to
    // this file.
    namespace
    {

        //******************************************************************************
        // Coverage function, which is very useful in all our classes.
        static inline float coverage(const TwkMath::Box2i& bnds,
                                     const TwkMath::Vec2f& loc, float rad)
        {
            // For now, we'll simply use linear coverage.
            TwkMath::Box2f bBox(
                TwkMath::Vec2f((float)(bnds.min[0]), (float)(bnds.min[1]))
                    - 0.5f,
                TwkMath::Vec2f((float)(bnds.max[0]), (float)(bnds.max[1]))
                    + 0.5f);

            // Check for point sample
            if (rad <= 0.0f)
            {
                // Point sample has either zero or one coverage.
                if (bBox.intersects(loc))
                {
                    return 1.0f;
                }
                else
                {
                    return 0.0f;
                }
            }

            TwkMath::Box2f lBox(loc - rad, loc + rad);
            const float larea = 4.0f * rad * rad;

            assert(larea > 0.0f);

            TwkMath::Box2f isect = TwkMath::intersection(lBox, bBox);
            TwkMath::Vec2f isize(isect.size());
            return (isize[0] * isize[1]) / larea;
        }

    } // End unnamed namespace

    //******************************************************************************
    // TEMPLATE AND INLINE FUNCTIONS
    //******************************************************************************
    template <class COLOR, class MIN_RSMP, class MAX_RSMP>
    inline MmLinRsmp<COLOR, MIN_RSMP, MAX_RSMP>::MmLinRsmp(MIN_RSMP& minRsmp,
                                                           MAX_RSMP& maxRsmp)
        : m_minRsmp(minRsmp)
        , m_maxRsmp(maxRsmp)
    {
        // Nothing
    }

    //******************************************************************************
    template <class COLOR, class MIN_RSMP, class MAX_RSMP>
    float MmLinRsmp<COLOR, MIN_RSMP, MAX_RSMP>::sample(
        const MipMap<COLOR>& mip, const TwkMath::Vec2f& loc, float minSampRad,
        float maxSampRad, float lodBias, COLOR& samp) const
    {
        // Determine the coverage.
        const float cov = coverage(mip.lod(0)->bounds(), loc,
                                   minSampRad * m_minRsmp.radius());

        // If zero radii, treat like a point sample
        if (minSampRad <= 0.0f || maxSampRad <= 0.0f)
        {
            const float locCov = m_maxRsmp.sample(*(mip.lod(0)), loc, samp);
            if (locCov != 0.0f)
            {
                samp *= cov / locCov;
                return cov;
            }
            else
            {
                return 0.0f;
            }
        }

        // Determine the lod.
        static const float ONE_OVER_LOG_2 = 1.4426950f;
        float lod = 0.0f;
        if (maxSampRad > 0.0f)
        {
            lod = ONE_OVER_LOG_2 * logf(maxSampRad);
        }
        lod += lodBias;

        // If the lod is less than or equal to 0.0f, just
        // use the lod0 image with the maximizing resampler.
        if (lod <= 0.0f || (mip.numLods() < 2))
        {
            // Even in this case, we need to adjust for coverage,
            // since the sampler will use a coverage of 1, which is
            // incorrect if the sampRad is smaller. This would become
            // relevant particularly if multiple samples were being
            // averaged to create a single pixel.
            const float lod0cov = m_maxRsmp.sample(*(mip.lod(0)), loc, samp);
            if (lod0cov > 0.0f)
            {
                samp /= lod0cov;
                samp *= cov;
                return cov;
            }
            else
            {
                return 0.0f;
            }
        }
        else
        {
            if (lod < 1.0f)
            {
                // The sample is somewhere between lod0 and lod1.
                // sample both and linearly interpolate.
                COLOR lod0samp;
                const float lod0cov =
                    m_maxRsmp.sample(*(mip.lod(0)), loc, lod0samp);
                if (lod0cov > 0.0f)
                {
                    lod0samp /= lod0cov;
                }

                // lods below 0 require coverage adjustment.
                COLOR lod1samp;
                const float lod1cov = m_minRsmp.sample(
                    *(mip.lod(1)), (loc + 0.5f) * 0.5f - 0.5f, lod1samp);

                if (lod1cov > 0.0f)
                {
                    lod1samp /= lod1cov;
                }

                // lod IS the interpolant
                // Linearly interpolate.
                samp = cov * TwkMath::lerp(lod0samp, lod1samp, lod);
                if (lod0cov > 0.0f || lod1cov > 0.0f)
                {
                    return cov;
                }
                else
                {
                    return 0.0f;
                }
            }
            else if (lod >= mip.numLods() - 1)
            {
                // Only one sample is necessary, since we're at the bottom.
                const int minLod = mip.numLods() - 1;
                const float minRad = (const float)(1 << minLod);
                const float minCov = m_minRsmp.sample(
                    *(mip.lod(minLod)), ((loc + 0.5f) / minRad) - 0.5f, samp);
                if (minCov > 0.0f)
                {
                    samp /= minCov;
                    samp *= cov;
                    return cov;
                }
                else
                {
                    return 0.0f;
                }
            }
            else
            {
                // The sample lies between two minimizing lods,
                // and both lods are valid.
                const int minLod = int(floorf(lod));
                const int maxLod = minLod + 1;

                assert(minLod >= 1 && minLod < mip.numLods());
                assert(maxLod >= 1 && maxLod < mip.numLods());

                const float t = lod - (const float)minLod;

                // Do the low sample
                const float minRad = (const float)(1 << minLod);
                COLOR minSamp;
                const float minCov =
                    m_minRsmp.sample(*(mip.lod(minLod)),
                                     ((loc + 0.5f) / minRad) - 0.5f, minSamp);
                if (minCov > 0.0f)
                {
                    minSamp /= minCov;
                }

                // Do the high sample
                const float maxRad = (const float)(1 << maxLod);
                COLOR maxSamp;
                const float maxCov =
                    m_minRsmp.sample(*(mip.lod(maxLod)),
                                     ((loc + 0.5f) / maxRad) - 0.5f, maxSamp);
                if (maxCov > 0.0f)
                {
                    maxSamp /= maxCov;
                }

                // Interpolate.
                samp = cov * TwkMath::lerp(minSamp, maxSamp, t);
                if (minCov > 0.0f || maxCov > 0.0f)
                {
                    return cov;
                }
                else
                {
                    return 0.0f;
                }
            }
        }
    }

    //******************************************************************************
    template <class COLOR, class MIN_RSMP, class MAX_RSMP>
    inline float MmLinRsmp<COLOR, MIN_RSMP, MAX_RSMP>::sampleAbs(
        const MipMap<COLOR>& mip, const TwkMath::Vec2f& loc, float minSampRad,
        float maxSampRad, float lodBias, COLOR& samp) const
    {
        return sample(mip,
                      loc - TwkMath::Vec2f(mip.origin()[0], mip.origin()[1]),
                      minSampRad, maxSampRad, lodBias, samp);
    }

} // End namespace TwkImg

#endif
