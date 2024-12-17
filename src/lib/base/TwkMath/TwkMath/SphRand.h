//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkMath__TwkMathSphRand__h__
#define __TwkMath__TwkMathSphRand__h__
#include <TwkMath/Math.h>

namespace TwkMath
{

    //
    //  class SphRand<>
    //
    //  Random vectors with length <= 1.0. RANDGEN is a random number generator
    //  class. VEC is a Vec{2,3,4} class.
    //

    template <class RANDGEN, class VEC> class SphRand
    {
    public:
        typedef typename VEC::value_type Scalar;

        SphRand(RANDGEN& rng)
            : m_rng(&rng)
        {
        }

        SphRand(RANDGEN* rng)
            : m_rng(rng)
        {
        }

        //
        //	Returns then next point. Will return a unit vector if
        //	nextNormalized() is called.
        //

        VEC next();
        VEC nextNormalized();

    private:
        RANDGEN* m_rng;
    };

    //
    //  These are generator versions of the above.
    //  The first one is not normalized the other is.
    //

    template <class RANDGEN, class VEC>
    class SphRandGenerator : public SphRand<RANDGEN, VEC>
    {
    public:
        SphRandGenerator(RANDGEN& rng)
            : SphRand<RANDGEN, VEC>(rng)
        {
        }

        SphRandGenerator(RANDGEN* rng)
            : SphRand<RANDGEN, VEC>(rng)
        {
        }

        VEC operator()() { return SphRand<RANDGEN, VEC>::next(); }
    };

    template <class RANDGEN, class VEC>
    class NormalizedSphRandGenerator : public SphRand<RANDGEN, VEC>
    {
    public:
        NormalizedSphRandGenerator(RANDGEN& rng)
            : SphRand<RANDGEN, VEC>(rng)
        {
        }

        NormalizedSphRandGenerator(RANDGEN* rng)
            : SphRand<RANDGEN, VEC>(rng)
        {
        }

        VEC operator()() { return SphRand<RANDGEN, VEC>::nextNormalized(); }
    };

    //----------------------------------------------------------------------

    template <class RANDGEN, class VEC> VEC SphRand<RANDGEN, VEC>::next()
    {
        VEC v;
        const Scalar one = Scalar(1.0);
        const Scalar two = Scalar(2.0);
        const int dim = VEC::dimension();
        Scalar m2;

        do
        {
            for (int i = 0; i < dim; i++)
            {
                v[i] = Scalar(m_rng->nextFloat() * two - one);
            }

            m2 = v.magnitudeSquared();
        } while (m2 > one);

        return v;
    }

    template <class RANDGEN, class VEC>
    VEC SphRand<RANDGEN, VEC>::nextNormalized()
    {
        VEC v;
        const Scalar one = Scalar(1.0);
        const Scalar two = Scalar(2.0);
        const int dim = VEC::dimension();
        Scalar m2;

        do
        {
            for (int i = 0; i < dim; i++)
            {
                v[i] = Scalar(m_rng->nextFloat() * two - one);
            }

            m2 = v.magnitudeSquared();
        } while (m2 > one);

        return v / Math<Scalar>::sqrt(m2);
    }

} // namespace TwkMath

#endif // __TwkMath__TwkMathSphRand__h__
