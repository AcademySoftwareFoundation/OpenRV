//******************************************************************************
// Copyright (c) 2001 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkMathRand48_h_
#define _TwkMathRand48_h_

// State Carrying Implementation of Drand48.
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <TwkMath/dll_defs.h>

namespace TwkMath
{

    //*****************************************************************************
    // 48 bit Random Number Generator
    class TWKMATH_EXPORT Rand48
    {
    public:
        Rand48();
        Rand48(unsigned long seed);

        void reset();
        void reset(unsigned long seed);

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

        // Double precision as well
        double nextDouble();

        double nextDouble(double range);

        double nextDouble(double low, double high);

    private:
        void nextState();
        unsigned int m_x[3];
        unsigned int m_a[3];
        unsigned int m_c;
    };

    //**********************************************************************
    //  Rand48 as an STL Generator function.
    //

    template <typename T> class Rand48Generator : public Rand48
    {
    public:
        Rand48Generator()
            : Rand48()
        {
        }

        Rand48Generator(unsigned long seed)
            : Rand48(seed)
        {
        }

        T operator()() { return T(nextDouble()); }
    };

    typedef Rand48Generator<float> Rand48Generatorf;
    typedef Rand48Generator<double> Rand48Generatord;

    template <> inline int Rand48Generator<int>::operator()()
    {
        return nextLong();
    }

    template <> inline unsigned int Rand48Generator<unsigned int>::operator()()
    {
        return nextULong();
    }

    template <> inline char Rand48Generator<char>::operator()()
    {
        return char(nextULong() % sizeof(char));
    }

    template <> inline short Rand48Generator<short>::operator()()
    {
        return char(nextULong() % sizeof(short));
    }

    template <>
    inline unsigned short Rand48Generator<unsigned short>::operator()()
    {
        return char(nextULong() % sizeof(unsigned short));
    }

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

#define HI_BIT ((unsigned long)1 << (2 * N - 1))

    //*****************************************************************************
    // INLINE FUNCTIONS
    //*****************************************************************************
    inline unsigned long Rand48::nextULong()
    {
        nextState();
        return (((long)m_x[2] << (N - 1)) + (m_x[1] >> 1));
    }

    //*****************************************************************************
    inline long Rand48::nextLong()
    {
        long l;

        nextState();
        return ((l = ((long)m_x[2] << N) + m_x[1]) & HI_BIT ? l | -HI_BIT : l);
    }

    //*****************************************************************************
    inline double Rand48::nextDouble()
    {
        static const double two16m = 1.0 / (1L << N);

        nextState();
        return (two16m * (two16m * (two16m * m_x[0] + m_x[1]) + m_x[2]));
    }

    //*****************************************************************************
    inline double Rand48::nextDouble(double range)
    {
        return range * nextDouble();
    }

    //*****************************************************************************
    inline double Rand48::nextDouble(double lo, double hi)
    {
        return lo + (hi - lo) * nextDouble();
    }

    //*****************************************************************************
    inline float Rand48::nextFloat() { return (float)nextDouble(); }

    //*****************************************************************************
    inline float Rand48::next() { return nextFloat(); }

    //*****************************************************************************
    inline float Rand48::nextFloat(float range) { return range * nextFloat(); }

    //*****************************************************************************
    inline float Rand48::nextFloat(float start, float end)
    {
        return start + nextFloat(end - start);
    }

//*****************************************************************************
// UNDEF THINGS
#undef N
#undef MASK
#undef LOW
#undef HIGH
#undef MUL
#undef CARRY
#undef ADDEQU
#undef X0
#undef X1
#undef X2
#undef A0
#undef A1
#undef A2
#undef C
#undef SET3
#undef SETLOW
#undef SEED
#undef HI_BIT

} // End namespace TwkMath

#endif
