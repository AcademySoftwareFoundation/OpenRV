//******************************************************************************
// Copyright (c) 2001-2004 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _Tex_Tex_h_
#define _Tex_Tex_h_

#include <TwkMath/Mat44.h>
#include <TwkExc/TwkExcException.h>
#include <iostream>

namespace Tex
{

    //******************************************************************************
    TWK_EXC_DECLARE(Exception, TwkExc::Exception, "Tex::Exception: ");

    //******************************************************************************
    // This function will read the two matrices OUTOF the tex file
    void GetMatrices(const char* fileName, TwkMath::Mat44f& camera,
                     TwkMath::Mat44f& screen, bool doTransposed);

    void GetMatrices(const char* fileName, const char* texPath,
                     TwkMath::Mat44f& camera, TwkMath::Mat44f& screen,
                     bool doTransposed);

} // End namespace Tex

#endif
