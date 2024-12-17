//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/ShaderValues.h>

template <> void outputHashValue(std::ostream& o, const TwkMath::Vec2<float>& v)
{
    o << v.x << "," << v.y;
}

template <> void outputHashValue(std::ostream& o, const TwkMath::Vec2<int>& v)
{
    o << v.x << "," << v.y;
}

template <> void outputHashValue(std::ostream& o, const TwkMath::Vec2<bool>& v)
{
    o << v.x << "," << v.y;
}

template <> void outputHashValue(std::ostream& o, const TwkMath::Vec3<float>& v)
{
    o << v.x << "," << v.y << "," << v.z;
}

template <> void outputHashValue(std::ostream& o, const TwkMath::Vec3<int>& v)
{
    o << v.x << "," << v.y << "," << v.z;
}

template <> void outputHashValue(std::ostream& o, const TwkMath::Vec3<bool>& v)
{
    o << v.x << "," << v.y << "," << v.z;
}

template <> void outputHashValue(std::ostream& o, const TwkMath::Vec4<float>& v)
{
    o << v.x << "," << v.y << "," << v.z << "," << v.w;
}

template <> void outputHashValue(std::ostream& o, const TwkMath::Vec4<int>& v)
{
    o << v.x << "," << v.y << "," << v.z << "," << v.w;
}

template <> void outputHashValue(std::ostream& o, const TwkMath::Vec4<bool>& v)
{
    o << v.x << "," << v.y << "," << v.z << "," << v.w;
}

template <>
void outputHashValue(std::ostream& o, const TwkMath::Mat44<float>& M)
{
    for (size_t row = 0; row < 4; row++)
        for (size_t col = 0; col < 4; col++)
            o << "," << M.m[row][col];
}

template <>
void outputHashValue(std::ostream& o, const TwkMath::Mat33<float>& M)
{
    for (size_t row = 0; row < 3; row++)
        for (size_t col = 0; col < 3; col++)
            o << "," << M.m[row][col];
}

template <>
void outputHashValue(std::ostream& o, const TwkMath::Mat22<float>& M)
{
    for (size_t row = 0; row < 2; row++)
        for (size_t col = 0; col < 2; col++)
            o << "," << M.m[row][col];
}
