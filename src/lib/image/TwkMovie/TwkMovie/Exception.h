//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkMovie__Exception__h__
#define __TwkMovie__Exception__h__
#include <TwkExc/Exception.h>
#include <TwkMovie/dll_defs.h>

namespace TwkMovie
{

    TWK_DERIVED_EXCEPTION_WITH_EXPORT(Exception, TWKMOVIE_EXPORT)
    TWK_DERIVED_EXCEPTION_FROM_WITH_EXPORT(Exception,
                                           UnsupportedMovieTypeException,
                                           TWKMOVIE_EXPORT)
    TWK_DERIVED_EXCEPTION_FROM_WITH_EXPORT(Exception, IOException,
                                           TWKMOVIE_EXPORT)

} // namespace TwkMovie

#endif // __TwkMovie__Exception__h__
