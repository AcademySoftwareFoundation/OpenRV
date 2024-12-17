//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkContainer__Properties__h__
#define __TwkContainer__Properties__h__
#include <TwkContainer/Component.h>
#include <TwkContainer/Exception.h>
#include <TwkContainer/Property.h>
#include <TwkContainer/PropertyContainer.h>
#include <TwkMath/Mat33.h>
#include <TwkMath/Mat44.h>
#include <TwkMath/Quat.h>
#include <TwkMath/Vec2.h>
#include <TwkMath/Vec3.h>
#include <TwkMath/Vec4.h>
#include <TwkMath/Box.h>
#include <half.h>
#include <string>
#include <vector>

namespace TwkContainer
{

    //
    //  Templates specialized to return the proper type name (GTOism)
    //

    // template <class T>
    // class PropertyTraits
    // {
    //   public:
    //     typedef std::vector<T> Container;
    //     static const char* typeName() { abort(); return "failed"; }
    //     static Property::Layout layout() { return Property::IntLayout; }
    //     static size_t xsize() { return 0; }
    //     static size_t ysize() { return 0; }
    //     static size_t zsize() { return 0; }
    // };

    // #define PROPERTY_TRAITS(PTYPENAME, TYPE, TYPENAME, LAYOUT, WIDTH)       \
// template <> const char* PropertyTraits<TYPE>::typeName();               \
// template <> Property::Layout PropertyTraits<TYPE>::layout();            \
// template <> size_t PropertyTraits<TYPE>::xsize();                       \
// template <> size_t PropertyTraits<TYPE>::ysize();                       \
// template <> size_t PropertyTraits<TYPE>::zsize();                       \
// typedef TypedProperty<PropertyTraits<TYPE>::Container,                  \
//                 WIDTH, 0, 0, Property::LAYOUT> PTYPENAME;

#define PROPERTY_TRAITS(PTYPENAME, TYPE, TYPENAME, LAYOUT, WIDTH)              \
    typedef TypedProperty<std::vector<TYPE>, WIDTH, 0, 0, 0, Property::LAYOUT> \
        PTYPENAME;

#define PROPERTY_TRAITS2(PTYPENAME, TYPE, TYPENAME, LAYOUT, XSIZE, YSIZE)   \
    template <> const char* PropertyTraits<TYPE>::typeName();               \
    template <> Property::Layout PropertyTraits<TYPE>::layout();            \
    template <> size_t PropertyTraits<TYPE>::xsize();                       \
    template <> size_t PropertyTraits<TYPE>::ysize();                       \
    template <> size_t PropertyTraits<TYPE>::zsize();                       \
    typedef TypedProperty<PropertyTraits<TYPE>::Container, XSIZE, YSIZE, 0, \
                          Property::LAYOUT>                                 \
        PTYPENAME;

#define PROPERTY_TRAITS3(PTYPENAME, TYPE, TYPENAME, LAYOUT, XSIZE, YSIZE, \
                         ZSIZE)                                           \
    template <> const char* PropertyTraits<TYPE>::typeName();             \
    template <> Property::Layout PropertyTraits<TYPE>::layout();          \
    template <> size_t PropertyTraits<TYPE>::xsize();                     \
    template <> size_t PropertyTraits<TYPE>::ysize();                     \
    template <> size_t PropertyTraits<TYPE>::zsize();                     \
    typedef TypedProperty<PropertyTraits<TYPE>::Container, XSIZE, YSIZE,  \
                          ZSIZE, Property::LAYOUT>                        \
        PTYPENAME;

    PROPERTY_TRAITS(FloatProperty, float, "float", FloatLayout, 1)
    PROPERTY_TRAITS(HalfProperty, half, "half", HalfLayout, 1)
    PROPERTY_TRAITS(DoubleProperty, double, "double", DoubleLayout, 1)
    PROPERTY_TRAITS(IntProperty, int, "int", IntLayout, 1)
    PROPERTY_TRAITS(ShortProperty, unsigned short, "short", ShortLayout, 1)
    PROPERTY_TRAITS(ByteProperty, unsigned char, "byte", ByteLayout, 1)
    PROPERTY_TRAITS(Vec4fProperty, TwkMath::Vec4f, "float[4]", FloatLayout, 4)
    PROPERTY_TRAITS(Vec3fProperty, TwkMath::Vec3f, "float[3]", FloatLayout, 3)
    PROPERTY_TRAITS(Vec2fProperty, TwkMath::Vec2f, "float[2]", FloatLayout, 2)
    PROPERTY_TRAITS(Vec4hProperty, TwkMath::Vec4h, "half[4]", FloatLayout, 4)
    PROPERTY_TRAITS(Vec3hProperty, TwkMath::Vec3h, "half[3]", FloatLayout, 3)
    PROPERTY_TRAITS(Vec2hProperty, TwkMath::Vec2h, "half[2]", FloatLayout, 2)
    PROPERTY_TRAITS(Vec4ucProperty, TwkMath::Vec4uc, "byte[4]", ByteLayout, 4)
    PROPERTY_TRAITS(Vec3ucProperty, TwkMath::Vec3uc, "byte[3]", ByteLayout, 3)
    PROPERTY_TRAITS(Vec2ucProperty, TwkMath::Vec2uc, "byte[2]", ByteLayout, 2)
    PROPERTY_TRAITS(Vec4usProperty, TwkMath::Vec4us, "short[4]", ShortLayout, 4)
    PROPERTY_TRAITS(Vec3usProperty, TwkMath::Vec3us, "short[3]", ShortLayout, 3)
    PROPERTY_TRAITS(Vec2usProperty, TwkMath::Vec2us, "short[2]", ShortLayout, 2)
    PROPERTY_TRAITS(Vec4iProperty, TwkMath::Vec4i, "int[4]", IntLayout, 4)
    PROPERTY_TRAITS(Vec3iProperty, TwkMath::Vec3i, "int[3]", IntLayout, 3)
    PROPERTY_TRAITS(Vec2iProperty, TwkMath::Vec2i, "int[2]", IntLayout, 2)
    PROPERTY_TRAITS(QuatfProperty, TwkMath::Quatf, "float[4]", FloatLayout, 4)
    PROPERTY_TRAITS(Box3fProperty, TwkMath::Box3f, "float[6]", FloatLayout, 6)

    PROPERTY_TRAITS(StringProperty, std::string, "string", StringLayout, 1)
    PROPERTY_TRAITS(StringPairProperty, StringPair, "string[2]", StringLayout,
                    2)

    PROPERTY_TRAITS(Mat44fProperty, TwkMath::Mat44f, "float[16]", FloatLayout,
                    16)
    PROPERTY_TRAITS(Mat33fProperty, TwkMath::Mat33f, "float[9]", FloatLayout, 9)
    // PROPERTY_TRAITS(Mat22fProperty, TwkMath::Mat22f, "float[4]", FloatLayout,
    // 4) PROPERTY_TRAITS2(Mat44fProperty, TwkMath::Mat44f, "float[4,4]",
    // FloatLayout, 4, 4) PROPERTY_TRAITS2(Mat33fProperty, TwkMath::Mat33f,
    // "float[3,3]", FloatLayout, 2, 2)

#undef PROPERTY_TRAITS

} // namespace TwkContainer

#endif // __TwkContainer__Properties__h__
