//******************************************************************************
// Copyright (c) 2010 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <TwkFB/Exception.h>

namespace TwkFB
{

    TWK_DERIVED_EXCEPTION_IMP(Exception)
    TWK_DERIVED_EXCEPTION_FROM_IMP(Exception, UnsupportedException)
    TWK_DERIVED_EXCEPTION_FROM_IMP(Exception, BitDepthTooLowException)
    TWK_DERIVED_EXCEPTION_FROM_IMP(Exception, BitDepthTooHighException)
    TWK_DERIVED_EXCEPTION_FROM_IMP(Exception, BadDataTypeForOperation)
    TWK_DERIVED_EXCEPTION_FROM_IMP(Exception, IOException)
    TWK_DERIVED_EXCEPTION_FROM_IMP(Exception, NullImageException)
    TWK_DERIVED_EXCEPTION_FROM_IMP(Exception, CacheFullException)
    TWK_DERIVED_EXCEPTION_FROM_IMP(Exception, CacheMismatchException)

} // namespace TwkFB
