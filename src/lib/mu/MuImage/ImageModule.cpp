//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <MuImage/ImageModule.h>
#include <MuImage/ImageType.h>
#include <MuImage/PixelsType.h>
#include <Mu/Function.h>
#include <Mu/Exception.h>
#include <Mu/MuProcess.h>
#include <Mu/Thread.h>
#include <Mu/ParameterVariable.h>
#include <MuLang/StringType.h>
#include <MuLang/MuLangContext.h>
#include <cv.h>
#include <sstream>
#include <stdlib.h>

namespace Mu
{
    using namespace std;

    typedef ImageType::ImageStruct ImageStruct;

    ImageModule::ImageModule(Context* c, const char* name)
        : Module(c, name)
    {
    }

    Module* ImageModule::init(const char* name, Context* context)
    {
        Module* m = new ImageModule(context, name);
        MuLangContext* c = static_cast<MuLangContext*>(context);
        Symbol* s = c->globalScope();
        s->addSymbol(m);
        return m;
    }

    ImageModule::~ImageModule() {}

    static ClassInstance* makeImage(MuLangContext* c, const Class* imageType,
                                    int w, int h)
    {
        DynamicArrayType* dataType =
            static_cast<DynamicArrayType*>(c->arrayType(c->vec4fType(), 1, 0));
        ClassInstance* o = ClassInstance::allocate(imageType);
        ImageStruct* im = o->data<ImageStruct>();

        im->data = new DynamicArray(dataType, 1);
        im->data->resize(w * h);
        im->width = w;
        im->height = h;
        im->name = 0;

        return o;
    }

    static NODE_IMPLEMENTATION(resize, Pointer)
    {
        MuLangContext* context =
            static_cast<MuLangContext*>(NODE_THREAD.context());
        const Class* c = static_cast<const ImageType*>(NODE_THIS.type());
        ClassInstance* inObj = NODE_ARG_OBJECT(0, ClassInstance);
        int width = NODE_ARG(1, int);
        int height = NODE_ARG(2, int);
        ClassInstance* outObj = makeImage(context, c, width, height);
        ImageStruct* inIm = inObj->data<ImageStruct>();
        ImageStruct* outIm = outObj->data<ImageStruct>();

        CvMat inMat;
        CvMat outMat;

        cvInitMatHeader(&inMat, inIm->height, inIm->width, CV_32FC(4),
                        inIm->data->data<float>(), 0);

        cvInitMatHeader(&outMat, outIm->height, outIm->width, CV_32FC(4),
                        outIm->data->data<float>(), 0);

        cvResize(&inMat, &outMat, CV_INTER_AREA);

        NODE_RETURN(outObj);
    }

    void ImageModule::load()
    {
        USING_MU_FUNCTION_SYMBOLS

        MuLangContext* c = (MuLangContext*)globalModule()->context();

        addSymbols(new ImageType(c), new PixelsType(c),

                   new Function(c, "resize", resize, None, Return,
                                "image.image", Parameters,
                                new Param(c, "img", "image.image"),
                                new Param(c, "newWidth", "int"),
                                new Param(c, "newHeight", "int"), End),

                   EndArguments);
    }

} // namespace Mu
