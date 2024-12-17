//******************************************************************************
// Copyright (c) 2006 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkMath__TwkMathKernelFunctions__h__
#define __TwkMath__TwkMathKernelFunctions__h__
#include <TwkMath/Math.h>
#include <limits>

namespace TwkMath
{

    namespace SPH
    {

        template <typename Scalar> inline Scalar W_spiky(Scalar r, Scalar h)
        {
            const Scalar x = r / h;
            const Scalar h4 = h * Scalar(4);
            const Scalar hcube = h4 * h4 * h4;
            const Scalar xm2 = Scalar(2) - x;
            const Scalar W0 = Scalar(15) / (Math<Scalar>::pi() * hcube);
            return (xm2 * xm2 * xm2) * W0;
        }

        template <typename Scalar> inline Scalar dW_spiky(Scalar r, Scalar h)
        {
            const Scalar h4 = h * Scalar(4);
            const Scalar hcube = h4 * h4 * h4;
            const Scalar x = h - r;
            const Scalar W0 = Scalar(15) / (Math<Scalar>::pi() * hcube);
            return x * x * -Scalar(3);
        }

        template <typename Scalar> inline Scalar W_beta(Scalar r, Scalar h)
        {
            if (h <= std::numeric_limits<Scalar>::epsilon())
                return Scalar(0);
            const Scalar x = r / h;
            const Scalar W0 = Scalar(4) * Math<Scalar>::pi() * h * h * h;

            //
            //  Do the > 1 case *first* because the likelyhood is much greater
            //  for this case.
            //

            if (x >= Scalar(1))
            {
                const Scalar xm2 = Scalar(2) - x;
                return (xm2 * xm2 * xm2) / W0;
            }
            else
            {
                const Scalar x2 = x * x;
                return (Scalar(4) - Scalar(6) * x2 + Scalar(3) * x * x2) / W0;
            }
        }

        template <typename Scalar> inline Scalar dW_beta(Scalar r, Scalar h)
        {
            if (h <= std::numeric_limits<Scalar>::epsilon())
                return Scalar(0);
            const Scalar x = r / h;
            const Scalar W0 = Scalar(4) * Math<Scalar>::pi() * h * h * h;

            //
            //  NOTE: 2/3 modification to Monaghan kernel (Thomas & Couchman)
            //  is used because we're *always* going to have a chaotic initial
            //  state or at least an initial state which is far from being in
            //  thermoequalibrium (which almost always causes clumping)
            //

            //
            //  Do the > 1 case *first* because the likelyhood is much greater
            //  for this case.
            //

            // if (x < (Scalar(2) / Scalar(3)))
            //{
            // return -Scalar(4) / W0;
            //}
            // else
            if (x >= Scalar(1))
            {
                const Scalar xm2 = Scalar(2) - x;
                return (-Scalar(3) * xm2 * xm2) / W0;
            }
            else
            {
                const Scalar x2 = x * x;
                return (-Scalar(12) * x + Scalar(9) * x2) / W0;
            }
        }

    } // namespace SPH

} // namespace TwkMath

#endif // __TwkMath__TwkMathKernelFunctions__h__
