//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef _TwkMathExc_h_
#define _TwkMathExc_h_
#include <TwkExc/TwkExcException.h>
#include <string>

namespace TwkMath
{

    TWK_EXC_DECLARE(MathExc, TwkExc::Exception, "math exception: ");
    TWK_EXC_DECLARE(SingularMatrixExc, MathExc, "singular matrix");
    TWK_EXC_DECLARE(BadFrustumExc, MathExc, "bad frustum");
    TWK_EXC_DECLARE(MatrixNotAffine, MathExc, "matrix not affine");
    TWK_EXC_DECLARE(EigenExc, MathExc, "Eigen Exc:");
    TWK_EXC_DECLARE(ZeroScaleExc, MathExc, "zero scale matrix");

} // namespace TwkMath

#endif
