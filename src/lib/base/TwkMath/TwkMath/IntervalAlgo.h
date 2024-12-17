//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkMathIntervalAlgo_h_
#define _TwkMathIntervalAlgo_h_

#include <TwkMath/Interval.h>
#include <TwkMath/Math.h>
#include <assert.h>

namespace TwkMath
{

    //******************************************************************************
    // COSINE
    template <typename T> Interval<T> cos(const Interval<T>& a)
    {
        assert(a.isValid());

        if (a.isInfinite())
        {
            return Interval<T>((T)-1, (T)1);
        }
        else
        {
            const T AoverPi = a.min / Math<T>::pi();
            const T ceilAoverPi = Math<T>::ceil(AoverPi);
            const T BoverPi = a.max / Math<T>::pi();

            if ((((T)1) + ceilAoverPi) <= BoverPi)
            {
                return Interval<T>((T)-1, (T)1);
            }
            else
            {
                const T cosa = Math<T>::cos(a.min);
                const T cosb = Math<T>::cos(a.max);

                if (ceilAoverPi <= BoverPi)
                {
                    if (((int)ceilAoverPi) % 2 == 0)
                    {
                        return Interval<T>(std::min(cosa, cosb), (T)1);
                    }
                    else
                    {
                        return Interval<T>((T)-1, std::max(cosa, cosb));
                    }
                }
                else
                {
                    // The constructor will flip the values if they're
                    // in the wrong order.
                    return Interval<T>(cosa, cosb);
                }
            }
        }
    }

    //******************************************************************************
    template <typename T> inline Interval<T> sin(const Interval<T>& a)
    {
        assert(a.isValid());

        if (a.isInfinite())
        {
            return Interval<T>((T)-1, (T)1);
        }
        else
        {
            static const T piOver2 = Math<T>::pi() / (T)2;
            return cos(Interval<T>(a.min - piOver2, a.max - piOver2));
        }
    }

    //******************************************************************************
    template <typename T> inline Interval<T> abs(const Interval<T>& a)
    {
        assert(a.isValid());

        if (a.isInfinite())
        {
            return a;
        }
        else if (a.intersects((T)0))
        {
            return Interval<T>(
                (T)0, std::max(Math<T>::abs(a.min), Math<T>::abs(a.max)));
        }
        else
        {
            // The constructor will flip them around if necessary
            return Interval<T>(Math<T>::abs(a.min), Math<T>::abs(a.max));
        }
    }

    //******************************************************************************
    template <typename T> inline Interval<T> sqr(const Interval<T>& a)
    {
        assert(a.isValid());

        if (a.isInfinite())
        {
            return a;
        }
        else
        {
            Interval<T> ret = abs(a);
            ret.min *= ret.min;
            ret.max *= ret.max;

            assert(a.isValid());

            return ret;
        }
    }

} // End namespace TwkMath

#endif
