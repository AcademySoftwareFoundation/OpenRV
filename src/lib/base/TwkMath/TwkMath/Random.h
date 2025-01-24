//******************************************************************************
// Copyright (c) 2001 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkMathRandom_h_
#define _TwkMathRandom_h_

#include <TwkMath/Function.h>
#include <TwkMath/Math.h>
#include <TwkMath/Rand48.h>
#include <assert.h>
#include <limits.h>

namespace TwkMath
{

    // Here are some random number generators
    // from Numerical Recipies in C.
    // They carry their seeds and states internally

    //*****************************************************************************
    // 32 bit random number generator
    class QuickRand
    {
    public:
        QuickRand(unsigned long seed = 0);
        void reset(unsigned long seed = 0);

        // RANGE: [0, ULONG_MAX]
        unsigned long nextULong();

        // RANGE: [0, high]
        unsigned long nextULong(unsigned long high);

        // RANGE: [low, high]
        unsigned long nextULong(unsigned long low, unsigned long high);

        // RANGE: [LONG_MIN, LONG_MAX]
        long nextLong();

        // RANGE: [0, high]
        long nextLong(long high);

        // RANGE: [low, high]
        long nextLong(long low, long high);

        // RANGE: [0.0f, 1.0f]
        float next(); // Synonym for next float.
        float nextFloat();

        // RANGE: [0.0f, range]
        float nextFloat(float range);

        // RANGE: [low, high]
        float nextFloat(float low, float high);

    private:
        unsigned long m_state;
    };

    //
    //  class QuickRandGenerator
    //
    //  QuickRand as an STL Generator function.
    //

    template <typename T> class QuickRandGenerator : public QuickRand
    {
    public:
        QuickRandGenerator()
            : QuickRand()
        {
        }

        QuickRandGenerator(unsigned long seed)
            : QuickRand(seed)
        {
        }

        T operator()() { return T(nextFloat()); }
    };

    template <> inline int QuickRandGenerator<int>::operator()()
    {
        return nextLong();
    }

    template <>
    inline unsigned int QuickRandGenerator<unsigned int>::operator()()
    {
        return nextULong();
    }

    template <> inline char QuickRandGenerator<char>::operator()()
    {
        return char(nextULong() % sizeof(char));
    }

    template <> inline short QuickRandGenerator<short>::operator()()
    {
        return char(nextULong() % sizeof(short));
    }

    template <>
    inline unsigned short QuickRandGenerator<unsigned short>::operator()()
    {
        return char(nextULong() % sizeof(unsigned short));
    }

    //*****************************************************************************
    // GAUSSIAN RANDOM NUMBER GENERATOR

    template <class RANDGEN> class GaussRand
    {
    public:
        GaussRand(RANDGEN& rng)
            : m_randGen(&rng)
            , m_savedGauss(0)
            , m_saved(false)
        {
        }

        GaussRand(RANDGEN* rng)
            : m_randGen(rng)
            , m_savedGauss(0)
            , m_saved(false)
        {
        }

        // Returns the next gaussian with a mean of 0
        // and a standard deviation of 1.
        float next();
        float next(float mean, float stddev);

    private:
        RANDGEN* m_randGen;
        float m_savedGauss;
        bool m_saved;
    };

    //**********************************************************************
    //  GaussRand as an STL Generator function.
    //

    template <typename T, class RANDGEN>
    class GaussRandGenerator : public GaussRand<RANDGEN>
    {
    public:
        GaussRandGenerator(RANDGEN& r)
            : GaussRand<RANDGEN>(&r)
        {
        }

        GaussRandGenerator(RANDGEN* r)
            : GaussRand<RANDGEN>(r)
        {
        }

        T operator()() { return T(double(GaussRand<RANDGEN>::next())); }
    };

    //*****************************************************************************
    // TYPEDEFS
    //*****************************************************************************
    typedef GaussRand<QuickRand> QuickGaussRand;
    typedef GaussRand<Rand48> GaussRand48;
    typedef GaussRandGenerator<float, QuickRand> GaussQuickGeneratorf;
    typedef GaussRandGenerator<float, Rand48> GaussRand48Generatorf;

    //*****************************************************************************
    // TEMPLATE AND INLINE FUNCTIONS
    //*****************************************************************************
    inline QuickRand::QuickRand(unsigned long state)
        : m_state(state)
    {
        // Nothing
    }

    //*****************************************************************************
    inline void QuickRand::reset(unsigned long seed) { m_state = seed; }

    //*****************************************************************************
    inline unsigned long QuickRand::nextULong()
    {
        // Increment state
        m_state = 1664525L * m_state + 1013904223L;
        return m_state;
    }

    //*****************************************************************************
    inline unsigned long QuickRand::nextULong(unsigned long high)
    {
        // This is actually not that easy. We can't just do a straight
        // remainder calculation, because that would produce a slightly
        // uneven distribution towards zero, "stepped".
        // We can't throw away anything outside the range 0-high, because
        // if high is small, this calculation will take forever.
        // Therefore we have to divide the range up into as high+1
        // equally sized buckets and choose from those buckets.

        if (high == 0)
        {
            return 0;
        }
        else if (high == ULONG_MAX)
        {
            return nextULong();
        }
        else
        {
            const unsigned long numBuckets = high + 1;
            const unsigned long bucketSize = ULONG_MAX / numBuckets;
            const unsigned long maxVal = numBuckets * bucketSize;

            assert(bucketSize > 0);

            unsigned long ret = nextULong();
            while (ret >= maxVal)
            {
                ret = nextULong();
            }

            return ret / bucketSize;
        }
    }

    //*****************************************************************************
    inline unsigned long QuickRand::nextULong(unsigned long low,
                                              unsigned long high)
    {
        if (low < high)
        {
            if (low == 0 && high == ULONG_MAX)
            {
                return nextULong();
            }
            else
            {
                return low + nextULong(high - low + 1);
            }
        }
        else
        {
            if (high == 0 && low == ULONG_MAX)
            {
                return nextULong();
            }
            else
            {
                return high + nextULong(low - high + 1);
            }
        }
    }

    //*****************************************************************************
    inline long QuickRand::nextLong() { return (long)nextULong(); }

    //*****************************************************************************
    inline long QuickRand::nextLong(long high)
    {
        if (high < 0)
        {
            return -((long)nextULong((unsigned long)(-high)));
        }
        else
        {
            return nextULong(high);
        }
    }

    //*****************************************************************************
    inline long QuickRand::nextLong(long low, long high)
    {
        if (low < high)
        {
            if (low == LONG_MIN && high == LONG_MAX)
            {
                return nextLong();
            }
            else
            {
                return low + nextULong(high - low + 1);
            }
        }
        else
        {
            if (high == LONG_MIN && low == LONG_MAX)
            {
                return nextLong();
            }
            else
            {
                return high + nextULong(low - high + 1);
            }
        }
    }

    //*****************************************************************************
    inline float QuickRand::nextFloat()
    {
        return ((float)nextULong()) / ((float)ULONG_MAX);
    }

    //*****************************************************************************
    inline float QuickRand::next() { return nextFloat(); }

    //*****************************************************************************
    inline float QuickRand::nextFloat(float range)
    {
        return range * nextFloat();
    }

    //*****************************************************************************
    inline float QuickRand::nextFloat(float start, float end)
    {
        return start + nextFloat(end - start);
    }

    //*****************************************************************************
    template <class RANDGEN> float GaussRand<RANDGEN>::next()
    {
        if (m_saved)
        {
            m_saved = false;
            return m_savedGauss;
        }
        else
        {
            // Pick two uniform random numbers
            // that lie within the -1, 1 square
            // and discard them if they do not lie within
            // the unit circle.
            float x = 0.0f;
            float y = 0.0f;
            float r2 = 0.0f;
            do
            {
                x = m_randGen->nextFloat(-1.0f, 1.0f);
                y = m_randGen->nextFloat(-1.0f, 1.0f);
                r2 = (x * x) + (y * y);
            } while (r2 > 1.0f || r2 == 0.0f);

            const float fac = sqrtf(-2.0f * logf(r2) / r2);

            // This Box-Muller
            // (or Box-Mueller, depending on where you read)
            // generates two normal deviates, not just one.
            // Save the other for later.
            m_savedGauss = x * fac;
            m_saved = true;

            return y * fac;
        }
    }

    //*****************************************************************************
    template <class RANDGEN>
    inline float GaussRand<RANDGEN>::next(float mean, float stddev)
    {
        return mean + (next() * stddev);
    }

} // End namespace TwkMath

#endif
