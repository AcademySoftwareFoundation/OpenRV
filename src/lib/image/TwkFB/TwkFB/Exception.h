//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkFB__Exception__h__
#define __TwkFB__Exception__h__
#include <TwkFB/dll_defs.h>
#include <TwkExc/Exception.h>
#include <string>

namespace TwkFB
{

    TWK_DERIVED_EXCEPTION_WITH_EXPORT(Exception, TWKFB_EXPORT)
    TWK_DERIVED_EXCEPTION_FROM_WITH_EXPORT(Exception, UnsupportedException,
                                           TWKFB_EXPORT)
    TWK_DERIVED_EXCEPTION_FROM_WITH_EXPORT(Exception, BitDepthTooLowException,
                                           TWKFB_EXPORT)
    TWK_DERIVED_EXCEPTION_FROM_WITH_EXPORT(Exception, BitDepthTooHighException,
                                           TWKFB_EXPORT)
    TWK_DERIVED_EXCEPTION_FROM_WITH_EXPORT(Exception, BadDataTypeForOperation,
                                           TWKFB_EXPORT)
    TWK_DERIVED_EXCEPTION_FROM_WITH_EXPORT(Exception, IOException, TWKFB_EXPORT)
    TWK_DERIVED_EXCEPTION_FROM_WITH_EXPORT(Exception, NullImageException,
                                           TWKFB_EXPORT)
    TWK_DERIVED_EXCEPTION_FROM_WITH_EXPORT(Exception, CacheFullException,
                                           TWKFB_EXPORT)
    TWK_DERIVED_EXCEPTION_FROM_WITH_EXPORT(Exception, CacheMismatchException,
                                           TWKFB_EXPORT)

} // namespace TwkFB

#endif // __TwkFB__Exception__h__
