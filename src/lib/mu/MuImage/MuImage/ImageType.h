#ifndef __MuLang__ImageType__h__
#define __MuLang__ImageType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Class.h>
#include <Mu/ClassInstance.h>
#include <Mu/Node.h>
#include <Mu/Thread.h>
#include <Mu/Vector.h>
#include <MuLang/StringType.h>
#include <MuLang/DynamicArray.h>
#include <iosfwd>
#include <string>

namespace Mu
{

    //
    //  class ImageType
    //
    //
    //

    class ImageType : public Class
    {
    public:
        ImageType(Context* c, Class* super = 0);
        ~ImageType();

        struct ImageStruct
        {
            const StringType::String* name;
            DynamicArray* data;
            int width;
            int height;

            Vector4f* row(int y)
            {
                return data->data<Vector4f>() + (y * width);
            }

            Vector4f& pixel(int x, int y)
            {
                return data->data<Vector4f>()[x + y * width];
            }

            Vector4f sample(float x, float y);
        };

        //
        //	Load function is called when the symbol is added to the
        //	context.
        //

        virtual void load();

        //
        //	Constant
        //

        static NODE_DECLARATION(construct, Pointer);
        static NODE_DECLARATION(dereference, Pointer);
        static NODE_DECLARATION(print, void);
        static NODE_DECLARATION(sample, Vector4f);
    };

} // namespace Mu

#endif // __MuLang__ImageType__h__
