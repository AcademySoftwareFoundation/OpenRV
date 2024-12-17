//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkContainer__DefaultValues__h__
#define __TwkContainer__DefaultValues__h__
#include <TwkMath/Mat33.h>
#include <TwkMath/Mat44.h>
#include <TwkMath/Quat.h>
#include <TwkMath/Vec2.h>
#include <TwkMath/Vec3.h>
#include <TwkMath/Vec4.h>
#include <TwkMath/Iostream.h>
#include <string>
#include <iostream>

namespace TwkContainer
{

    typedef std::pair<std::string, std::string> StringPair;

    std::ostream& operator<<(std::ostream& o,
                             const TwkContainer::StringPair& v);

    template <class T> inline T defaultValue() { return T(0); }

    template <> inline TwkMath::Vec4f defaultValue<TwkMath::Vec4f>()
    {
        return TwkMath::Vec4f(0.f, 0.f, 0.f, 0.f);
    }

    template <> inline TwkMath::Vec3f defaultValue<TwkMath::Vec3f>()
    {
        return TwkMath::Vec3f(0.f, 0.f, 0.f);
    }

    template <> inline TwkMath::Vec2uc defaultValue<TwkMath::Vec2uc>()
    {
        return TwkMath::Vec2uc(0, 0);
    }

    template <> inline TwkMath::Vec3uc defaultValue<TwkMath::Vec3uc>()
    {
        return TwkMath::Vec3uc(0, 0, 0);
    }

    template <> inline TwkMath::Vec4uc defaultValue<TwkMath::Vec4uc>()
    {
        return TwkMath::Vec4uc(0, 0, 0, 0);
    }

    template <> inline TwkMath::Vec2us defaultValue<TwkMath::Vec2us>()
    {
        return TwkMath::Vec2us(0, 0);
    }

    template <> inline TwkMath::Vec3us defaultValue<TwkMath::Vec3us>()
    {
        return TwkMath::Vec3us(0, 0, 0);
    }

    template <> inline TwkMath::Vec4us defaultValue<TwkMath::Vec4us>()
    {
        return TwkMath::Vec4us(0, 0, 0, 0);
    }

    template <> inline TwkMath::Vec2f defaultValue<TwkMath::Vec2f>()
    {
        return TwkMath::Vec2f(0.f, 0.f);
    }

    template <> inline TwkMath::Mat44f defaultValue<TwkMath::Mat44f>()
    {
        return TwkMath::Mat44f();
    }

    template <> inline TwkMath::Mat33f defaultValue<TwkMath::Mat33f>()
    {
        return TwkMath::Mat33f();
    }

    template <> inline TwkMath::Quatf defaultValue<TwkMath::Quatf>()
    {
        return TwkMath::Quatf(1.0f);
    }

    template <> inline std::string defaultValue<std::string>()
    {
        return std::string("");
    }

    template <> inline StringPair defaultValue<StringPair>()
    {
        return StringPair("", "");
    }

} // namespace TwkContainer

#endif // __TwkContainer__DefaultValues__h__
