//******************************************************************************
// Copyright (c) 2004 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkContainer/Properties.h>

namespace TwkContainer
{

#if 0

#define PROPERTY_TRAITS_IMP(PTYPENAME, TYPE, TYPENAME, LAYOUT, XS)  \
    template <> const char* PropertyTraits<TYPE>::typeName()        \
    {                                                               \
        return TYPENAME;                                            \
    }                                                               \
                                                                    \
    template <> Property::Layout PropertyTraits<TYPE>::layout()     \
    {                                                               \
        return Property::LAYOUT;                                    \
    }                                                               \
                                                                    \
    template <> size_t PropertyTraits<TYPE>::xsize() { return XS; } \
                                                                    \
    template <> size_t PropertyTraits<TYPE>::ysize() { return 0; }  \
                                                                    \
    template <> size_t PropertyTraits<TYPE>::zsize() { return 0; }

#define PROPERTY_TRAITS_IMP2(PTYPENAME, TYPE, TYPENAME, LAYOUT, XS, YS) \
    template <> const char* PropertyTraits<TYPE>::typeName()            \
    {                                                                   \
        return TYPENAME;                                                \
    }                                                                   \
                                                                        \
    template <> Property::Layout PropertyTraits<TYPE>::layout()         \
    {                                                                   \
        return Property::LAYOUT;                                        \
    }                                                                   \
                                                                        \
    template <> size_t PropertyTraits<TYPE>::xsize() { return XS; }     \
                                                                        \
    template <> size_t PropertyTraits<TYPE>::ysize() { return YS; }     \
                                                                        \
    template <> size_t PropertyTraits<TYPE>::zsize() { return 0; }

#define PROPERTY_TRAITS_IMP3(PTYPENAME, TYPE, TYPENAME, LAYOUT, XS, YS, ZS) \
    template <> const char* PropertyTraits<TYPE>::typeName()                \
    {                                                                       \
        return TYPENAME;                                                    \
    }                                                                       \
                                                                            \
    template <> Property::Layout PropertyTraits<TYPE>::layout()             \
    {                                                                       \
        return Property::LAYOUT;                                            \
    }                                                                       \
                                                                            \
    template <> size_t PropertyTraits<TYPE>::xsize() { return XS; }         \
                                                                            \
    template <> size_t PropertyTraits<TYPE>::ysize() { return YS; }         \
                                                                            \
    template <> size_t PropertyTraits<TYPE>::zsize() { return ZS; }
                                                                        
PROPERTY_TRAITS_IMP(FloatProperty, float, "float", FloatLayout, 1)
PROPERTY_TRAITS_IMP(FloatProperty, half, "half", HalfLayout, 1)
PROPERTY_TRAITS_IMP(DoubleProperty, double, "double", DoubleLayout, 1)
PROPERTY_TRAITS_IMP(IntProperty, int, "int", IntLayout, 1)
PROPERTY_TRAITS_IMP(ShortProperty, unsigned short, "short", ShortLayout, 1)
PROPERTY_TRAITS_IMP(ByteProperty, unsigned char, "byte", ByteLayout, 1)
PROPERTY_TRAITS_IMP(Vec4fProperty, TwkMath::Vec4f, "float[4]", FloatLayout, 4)
PROPERTY_TRAITS_IMP(Vec3fProperty, TwkMath::Vec3f, "float[3]", FloatLayout, 3)
PROPERTY_TRAITS_IMP(Vec2fProperty, TwkMath::Vec2f, "float[2]", FloatLayout, 2)
PROPERTY_TRAITS_IMP(Vec4iProperty, TwkMath::Vec4i, "int[4]", IntLayout, 4)
PROPERTY_TRAITS_IMP(Vec3iProperty, TwkMath::Vec3i, "int[3]", IntLayout, 3)
PROPERTY_TRAITS_IMP(Vec2iProperty, TwkMath::Vec2i, "int[2]", IntLayout, 2)
PROPERTY_TRAITS_IMP(QuatfProperty, TwkMath::Quatf, "float[4]", FloatLayout, 4)
PROPERTY_TRAITS_IMP(Box3fProperty, TwkMath::Box3f, "float[6]", FloatLayout, 6)
PROPERTY_TRAITS_IMP(StringProperty, std::string, "string", StringLayout, 1)
PROPERTY_TRAITS_IMP(StringPairProperty, std::string, "string[2]", StringLayout, 2)
PROPERTY_TRAITS_IMP(Mat44fProperty, TwkMath::Mat44f, "float[4,4]", FloatLayout, 16)
PROPERTY_TRAITS_IMP(Mat33fProperty, TwkMath::Mat33f, "float[3,3]", FloatLayout, 9)

#endif

    // PROPERTY_TRAITS_IMP2(Mat44fProperty, TwkMath::Mat44f, "float[4,4]",
    // FloatLayout, 4, 4) PROPERTY_TRAITS_IMP2(Mat33fProperty, TwkMath::Mat33f,
    // "float[3,3]", FloatLayout, 3, 3)

} // namespace TwkContainer
