//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__ShaderValues__h__
#define __IPCore__ShaderValues__h__
#include <iostream>
#include <TwkMath/Vec2.h>
#include <TwkMath/Vec3.h>
#include <TwkMath/Vec4.h>
#include <TwkMath/Mat44.h>
#include <TwkMath/Mat33.h>
#include <TwkMath/Iostream.h>

template <typename T> void outputHashValue(std::ostream& o, const T& value)
{
    o << value;
}

template <>
void outputHashValue(std::ostream& o, const TwkMath::Vec2<float>& v);
template <> void outputHashValue(std::ostream& o, const TwkMath::Vec2<int>& v);
template <> void outputHashValue(std::ostream& o, const TwkMath::Vec2<bool>& v);
template <>
void outputHashValue(std::ostream& o, const TwkMath::Vec3<float>& v);
template <> void outputHashValue(std::ostream& o, const TwkMath::Vec3<int>& v);
template <> void outputHashValue(std::ostream& o, const TwkMath::Vec3<bool>& v);
template <>
void outputHashValue(std::ostream& o, const TwkMath::Vec4<float>& v);
template <> void outputHashValue(std::ostream& o, const TwkMath::Vec4<int>& v);
template <> void outputHashValue(std::ostream& o, const TwkMath::Vec4<bool>& v);
template <>
void outputHashValue(std::ostream& o, const TwkMath::Mat44<float>& M);
template <>
void outputHashValue(std::ostream& o, const TwkMath::Mat33<float>& M);
template <>
void outputHashValue(std::ostream& o, const TwkMath::Mat22<float>& M);

#endif // __IPCore__ShaderValues__h__
