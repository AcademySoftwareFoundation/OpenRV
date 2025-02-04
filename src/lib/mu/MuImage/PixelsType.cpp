//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <MuImage/PixelsType.h>
#include <Mu/Function.h>
#include <MuLang/MuLangContext.h>
#include <Mu/Module.h>
#include <Mu/Function.h>
#include <Mu/VariantTagType.h>
#include <Mu/VariantInstance.h>

namespace Mu
{
    using namespace std;

    PixelsType::PixelsType(Context* c)
        : VariantType(c, "Pixels")
    {
    }

    PixelsType::~PixelsType() {}

    void PixelsType::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        MuLangContext* context = (MuLangContext*)globalModule()->context();
        Context* c = context;
        context->arrayType(context->floatType(), 1, 0);
        context->arrayType(context->halfType(), 1, 0);
        context->arrayType(context->shortType(), 1, 0);
        const Type* btype = context->arrayType(context->byteType(), 1, 0);

        VariantTagType* fp = new VariantTagType(c, "FloatPixels", "float[]");
        VariantTagType* hp = new VariantTagType(c, "HalfPixels", "half[]");
        VariantTagType* sp = new VariantTagType(c, "ShortPixels", "short[]");
        VariantTagType* bp = new VariantTagType(c, "BytePixels", "byte[]");

        addSymbols(fp, hp, sp, bp, EndArguments);

        const char* tn = fullyQualifiedName().c_str();

        //
        //  Add default constructors (w/o arguments).
        //

        fp->addSymbol(new Function(c, fp->name().c_str(),
                                   PixelsType::defaultPixelConstructor, None,
                                   Return, tn, End));

        hp->addSymbol(new Function(c, hp->name().c_str(),
                                   PixelsType::defaultPixelConstructor, None,
                                   Return, tn, End));

        sp->addSymbol(new Function(c, sp->name().c_str(),
                                   PixelsType::defaultPixelConstructor, None,
                                   Return, tn, End));

        bp->addSymbol(new Function(c, bp->name().c_str(),
                                   PixelsType::defaultPixelConstructor, None,
                                   Return, tn, End));
    }

    NODE_IMPLEMENTATION(PixelsType::defaultPixelConstructor, Pointer)
    {
        //
        //  Tricky -- get the scope of the constructor symbol -- that's
        //  the tag type.
        //

        const VariantTagType* t =
            static_cast<const VariantTagType*>(NODE_THIS.symbol()->scope());

        NODE_RETURN(VariantInstance::allocate(t));
    }

} // namespace Mu
