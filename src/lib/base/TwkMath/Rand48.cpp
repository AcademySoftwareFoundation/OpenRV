//******************************************************************************
// Copyright (c) 2001 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkMath/Rand48.h>

//*****************************************************************************
// DEFINITIONS USED BY FUNCTIONS
//*****************************************************************************

#define N 16
#define MASK ((unsigned)(1 << (N - 1)) + (1 << (N - 1)) - 1)
#define LOW(x) ((unsigned)(x) & MASK)

#define HIGH(x) LOW((x) >> N)

#define MUL(x, y, z)                    \
    {                                   \
        long l = (long)(x) * (long)(y); \
        (z)[0] = LOW(l);                \
        (z)[1] = HIGH(l);               \
    }

#define CARRY(x, y) ((long)(x) + (long)(y) > MASK)

#define ADDEQU(x, y, z) (z = CARRY(x, (y)), x = LOW(x + (y)))

#define X0 0x330E
#define X1 0xABCD
#define X2 0x1234
#define A0 0xE66D
#define A1 0xDEEC
#define A2 0x5
#define C 0xB
#define SET3(x, x0, x1, x2) ((x)[0] = (x0), (x)[1] = (x1), (x)[2] = (x2))
#define SETLOW(x, y, n) \
    SET3(x, LOW((y)[n]), LOW((y)[(n) + 1]), LOW((y)[(n) + 2]))
#define SEED(x0, x1, x2) (SET3(m_x, x0, x1, x2), SET3(m_a, A0, A1, A2), m_c = C)

#define HI_BIT (1L << (2 * N - 1))

namespace TwkMath
{

    //*****************************************************************************
    Rand48::Rand48()
    {
        m_x[0] = X0;
        m_x[1] = X1;
        m_x[2] = X2;
        m_a[0] = A0;
        m_a[1] = A1;
        m_a[2] = A2;
        m_c = C;
    }

    //*****************************************************************************
    Rand48::Rand48(unsigned long seed) { reset(seed); }

    //*****************************************************************************
    void Rand48::reset()
    {
        m_x[0] = X0;
        m_x[1] = X1;
        m_x[2] = X2;
        m_a[0] = A0;
        m_a[1] = A1;
        m_a[2] = A2;
        m_c = C;
    }

    //*****************************************************************************
    void Rand48::reset(unsigned long seedval)
    {
        SEED(X0, LOW(seedval), HIGH(seedval));
    }

    //*****************************************************************************
    void Rand48::nextState()
    {
        unsigned int p[2], q[2], r[2], carry0, carry1;

        MUL(m_a[0], m_x[0], p);
        ADDEQU(p[0], m_c, carry0);
        ADDEQU(p[1], carry0, carry1);
        MUL(m_a[0], m_x[1], q);
        ADDEQU(p[1], q[0], carry0);
        MUL(m_a[1], m_x[0], r);
        m_x[2] =
            LOW(carry0 + carry1 + CARRY(p[1], r[0]) + q[1] + r[1]
                + (m_a[0] * m_x[2]) + (m_a[1] * m_x[1]) + (m_a[2] * m_x[0]));
        m_x[1] = LOW(p[1] + r[0]);
        m_x[0] = LOW(p[0]);
    }

    //*****************************************************************************
    unsigned long Rand48::nextULong(unsigned long high)
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
#ifdef DEBUG
            assert(bucketSize > 0);
#endif
            unsigned long ret = nextULong();
            while (ret >= maxVal)
            {
                ret = nextULong();
            }

            return ret / bucketSize;
        }
    }

    //*****************************************************************************
    unsigned long Rand48::nextULong(unsigned long low, unsigned long high)
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
    long Rand48::nextLong(long high)
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
    long Rand48::nextLong(long low, long high)
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

} // End namespace TwkMath
