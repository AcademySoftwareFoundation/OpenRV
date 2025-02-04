
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <MuEncoding/EncodingModule.h>
#include <Mu/Function.h>
#include <Mu/MuProcess.h>
#include <Mu/Thread.h>
#include <Mu/Exception.h>
#include <Mu/ParameterVariable.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/DynamicArrayType.h>
#include <MuLang/StringType.h>
#include <MuLang/DynamicArray.h>
#include <algorithm>
#include <sstream>
#include <Mu/UTF8.h>

namespace Mu
{
    using namespace std;

    EncodingModule::EncodingModule(Context* c, const char* name)
        : Module(c, name)
    {
    }

    EncodingModule::~EncodingModule() {}

    void EncodingModule::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        MuLangContext* c = (MuLangContext*)globalModule()->context();
        c->arrayType(c->byteType(), 1, 0);

        addSymbols(
            new Function(c, "to_base64", EncodingModule::to_base64, None,
                         Return, "byte[]", Parameters,
                         new ParameterVariable(c, "data", "byte[]"), End),

            new Function(c, "from_base64", EncodingModule::from_base64, None,
                         Return, "byte[]", Parameters,
                         new ParameterVariable(c, "data", "byte[]"), End),

            new Function(c, "string_to_utf8", EncodingModule::string_to_utf8,
                         None, Return, "byte[]", Parameters,
                         new ParameterVariable(c, "text", "string"), End),

            new Function(c, "utf8_to_string", EncodingModule::utf8_to_string,
                         None, Return, "string", Parameters,
                         new ParameterVariable(c, "data", "byte[]"), End),

            EndArguments);
    }

    NODE_IMPLEMENTATION(EncodingModule::to_base64, Pointer)
    {
        static const char* base64Table =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const DynamicArray* in = NODE_ARG_OBJECT(0, DynamicArray);
        DynamicArray* out = new DynamicArray(
            static_cast<const DynamicArrayType*>(in->type()), 1);

        const size_t insize = in->size();

        if (insize == 0)
            NODE_RETURN(out);

        const size_t rem = insize % 3;

        out->resize(insize / 3 * 4 + (rem ? 4 : 0));
        const size_t outsize = out->size();

        for (size_t i = 0, j = 0, s = insize - rem; i < s; i += 3, j += 4)
        {
            const byte a = in->element<byte>(i + 0);
            const byte b = in->element<byte>(i + 1);
            const byte c = in->element<byte>(i + 2);

            out->element<byte>(j + 0) = base64Table[a >> 2];
            out->element<byte>(j + 1) =
                base64Table[((a & 0x3) << 4) + (b >> 4)];
            out->element<byte>(j + 2) =
                base64Table[((b & 0xf) << 2) + (c >> 6)];
            out->element<byte>(j + 3) = base64Table[c & 0x3f];
        }

        switch (rem)
        {
        case 0:
            break;
        case 2:
        {
            const byte a = in->element<byte>(insize - 2);
            const byte b = in->element<byte>(insize - 1);
            const byte c = 0;
            out->element<byte>(outsize - 4) = base64Table[a >> 2];
            out->element<byte>(outsize - 3) =
                base64Table[((a & 0x3) << 4) + (b >> 4)];
            out->element<byte>(outsize - 2) =
                base64Table[((b & 0xf) << 2) + (c >> 6)];
            out->element<byte>(outsize - 1) = '=';
        }
        break;
        case 1:
        {
            const byte a = in->element<byte>(insize - 1);
            const byte b = 0;
            out->element<byte>(outsize - 4) = base64Table[a >> 2];
            out->element<byte>(outsize - 3) =
                base64Table[((a & 0x3) << 4) + (b >> 4)];
            out->element<byte>(outsize - 2) = '=';
            out->element<byte>(outsize - 1) = '=';
        }
        break;
        }

        NODE_RETURN(out);
    }

    static unsigned char base64Value(char c)
    {
        if (c >= 'A' && c <= 'Z')
            return c - 'A';
        if (c >= 'a' && c <= 'z')
            return c - 'a' + 26;
        if (c >= '0' && c <= '9')
            return c - '0' + 26 * 2;
        if (c == '+')
            return 62;
        if (c == '/')
            return 63;
        if (c == '=')
            return 0xff;
        abort();
        return 0xff;
    }

    NODE_IMPLEMENTATION(EncodingModule::from_base64, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const DynamicArray* in = NODE_ARG_OBJECT(0, DynamicArray);
        DynamicArray* out = new DynamicArray(
            static_cast<const DynamicArrayType*>(in->type()), 1);

        const size_t insize = in->size();
        const size_t rem = insize % 4;

        if (rem || insize == 0)
        {
            throw Mu::OutOfRangeException();
        }
        else
        {
            const byte c = base64Value(in->element<byte>(in->size() - 2));
            const byte d = base64Value(in->element<byte>(in->size() - 1));
            size_t s = insize / 4 * 3;
            if (d == (unsigned char)0xff)
                s--;
            if (c == (unsigned char)0xff)
                s--;
            out->resize(s);
        }

        const size_t outsize = out->size();

        for (size_t i = 0, j = 0; i < insize; i += 4, j += 3)
        {
            const byte a = base64Value(in->element<byte>(i + 0));
            const byte b = base64Value(in->element<byte>(i + 1));
            const byte c = base64Value(in->element<byte>(i + 2));
            const byte d = base64Value(in->element<byte>(i + 3));

            out->element<byte>(j + 0) = (a << 2) + (b >> 4);

            if (d == (unsigned char)0xff)
            {
                if (c != (unsigned char)0xff)
                {
                    out->element<byte>(j + 1) = (b << 4) + (c >> 2);
                }
            }
            else
            {
                out->element<byte>(j + 1) = (b << 4) + (c >> 2);
                out->element<byte>(j + 2) = (c << 6) + d;
            }
        }

        NODE_RETURN(out);
    }

    NODE_IMPLEMENTATION(EncodingModule::string_to_utf8, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const StringType::String* s = NODE_ARG_OBJECT(0, StringType::String);

        const DynamicArrayType* atype =
            static_cast<const DynamicArrayType*>(NODE_THIS.type());
        DynamicArray* array = new DynamicArray(atype, 1);

        //
        //  We're currently using UF8 internally so just copy
        //

        size_t size = s->size();
        array->resize(size);
        memcpy(array->data<byte>(), s->c_str(), size);

        NODE_RETURN(array);
    }

    NODE_IMPLEMENTATION(EncodingModule::utf8_to_string, Pointer)
    {
        Process* p = NODE_THREAD.process();
        MuLangContext* c = static_cast<MuLangContext*>(p->context());
        const DynamicArray* a = NODE_ARG_OBJECT(0, DynamicArray);

        size_t size = a->size();
        ostringstream str;

        for (size_t i = 0; i < size; i++)
        {
            str << a->element<char>(i);
        }

        NODE_RETURN(c->stringType()->allocate(str));
    }

} // namespace Mu
