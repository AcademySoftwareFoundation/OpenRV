//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <TwkMovie/Exception.h>

namespace TwkMovie
{

    TWK_DERIVED_EXCEPTION_IMP(Exception)
    TWK_DERIVED_EXCEPTION_FROM_IMP(Exception, UnsupportedMovieTypeException)
    TWK_DERIVED_EXCEPTION_FROM_IMP(Exception, IOException)

} // namespace TwkMovie
