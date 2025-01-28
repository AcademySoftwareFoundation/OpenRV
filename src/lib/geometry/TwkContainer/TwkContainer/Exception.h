//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkContainer__Exception__h__
#define __TwkContainer__Exception__h__
#include <TwkExc/TwkExcException.h>
#include <string>

namespace TwkContainer
{

    class Exception : public TwkExc::Exception
    {
    public:
        Exception() throw()
            : TwkExc::Exception()
        {
        }

        Exception(const char* str) throw()
            : TwkExc::Exception(str)
        {
        }

        virtual ~Exception() throw() {}
    };

#define TWK_GEOM_EXC_DECLARE(EXC_TYPE, BASE_TYPE, TAG) \
    class EXC_TYPE : public BASE_TYPE                  \
    {                                                  \
    public:                                            \
        EXC_TYPE()                                     \
            : BASE_TYPE(TAG)                           \
        {                                              \
        }                                              \
        EXC_TYPE(const char* str)                      \
            : BASE_TYPE(TAG)                           \
        {                                              \
            m_string.append(str);                      \
        }                                              \
    };

    TWK_GEOM_EXC_DECLARE(ReadFailedExc, Exception, "read failed")
    TWK_GEOM_EXC_DECLARE(BadPropertyTypeMatchExc, Exception,
                         "bad property match")
    TWK_GEOM_EXC_DECLARE(NoPropertyExc, Exception, "no such property")
    TWK_GEOM_EXC_DECLARE(UnexpectedExc, Exception, "unexpected program state")
    TWK_GEOM_EXC_DECLARE(TypeMismatchExc, Exception,
                         "PropertyContainer type mismatch")
    TWK_GEOM_EXC_DECLARE(IncompatibleGeometryExc, Exception,
                         "incompatible geometry")

} // namespace TwkContainer

#endif
