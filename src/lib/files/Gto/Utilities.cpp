//
// Copyright (c) 2009, Tweak Software
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Gto/Utilities.h>
#include <assert.h>
#include <fstream>
#ifdef GTO_SUPPORT_ZIP
#include <zlib.h>
// zlib might define Byte which clashes with the Gto::Byte enum
#ifdef Byte
#undef Byte
#endif
#endif
#ifdef GTO_SUPPORT_HALF
#include <half.h>
#endif
#include <stdlib.h>

// Unicode filename conversion
#ifdef _MSC_VER
#include <string>
#include <codecvt>
#include <wchar.h>
#endif

namespace Gto
{
    using namespace std;

    static unsigned int Csizes[] = {
        Int,    sizeof(int32),   Float,   sizeof(float32),
        Double, sizeof(float64),
#ifdef GTO_SUPPORT_HALF
        Half,   sizeof(half),
#else
        Half,   sizeof(float32) / 2,
#endif
        String, sizeof(uint32),  Boolean, sizeof(uint8),
        Short,  sizeof(uint16),  Byte,    sizeof(uint8)};

    size_t dataSizeInBytes(Gto::uint32 type) { return Csizes[type * 2 + 1]; }

    size_t bufferSizeInBytes(Gto::uint32 type, const Dimensions& dims)
    {
        return dataSizeInBytes(type) * elementSize(dims);
    }

    size_t elementSize(const Dimensions& dims)
    {
        size_t xs = dims.x ? dims.x : 1;
        size_t ys = dims.y ? dims.y : 1;
        size_t zs = dims.z ? dims.z : 1;
        size_t ws = dims.w ? dims.w : 1;

        return xs * ys * zs * ws;
    }

    size_t elementSize(const TypeSpec& spec)
    {
        return elementSize(
            Dimensions(spec.dims.x, spec.dims.y, spec.dims.z, spec.dims.w));
    }

    const char* typeName(Gto::DataType t)
    {
        switch (t)
        {
        case Float:
            return "float";
        case Boolean:
            return "bool";
        case Double:
            return "double";
        case Int:
            return "int";
        case Short:
            return "short";
        case Byte:
            return "byte";
        case String:
            return "string";
        case Half:
            return "half";
        default:
            abort();
            return "";
        }
    }

    bool isNumber(Gto::DataType t) { return t != String && t != ErrorType; }

    Number asNumber(void* data, Gto::DataType t)
    {
        Number n;
        n.type = t;

        switch (t)
        {
        case Float:
            n._double = *reinterpret_cast<float*>(data);
            n.type = Float;
            break;
        case Double:
            n._double = *reinterpret_cast<double*>(data);
            n.type = Double;
            break;
#ifdef GTO_SUPPORT_HALF
        case Half:
            n._double = double(*reinterpret_cast<half*>(data));
            n.type = Float;
            break;
#endif
        case Int:
            n._int = int(*reinterpret_cast<int*>(data));
            n.type = Int;
            break;
        case Short:
            n._int = int(*reinterpret_cast<short*>(data));
            n.type = Int;
            break;
        case Byte:
            n._int = int(*reinterpret_cast<unsigned char*>(data));
            n.type = Int;
            break;
        default:
            n.type = ErrorType;
        }

        return n;
    }

    bool isGTOFile(const char* infile)
    {
        Header header;

#ifdef _MSC_VER
        wstring_convert<codecvt_utf8<wchar_t>> wstr;
        wstring wstr_filename = wstr.from_bytes(infile);
        const wchar_t* w_infile = wstr_filename.c_str();
#endif

#ifdef GTO_SUPPORT_ZIP

#ifdef _MSC_VER
        if (gzFile file = gzopen_w(w_infile, "rb"))
#else
        if (gzFile file = gzopen(infile, "rb"))
#endif
        {
            if (gzread(file, &header, sizeof(header)) != sizeof(Header))
            {
                gzclose(file);
                return false;
            }
        }
#else
#ifdef _MSC_VER
        ifstream file(w_infile);
#else
        ifstream file(infile);
#endif
        if (!file)
            return false;
        if (file.readsome((char*)&header, sizeof(Header)) != sizeof(Header))
            return false;
        if (file.fail())
            return false;
#endif

        return header.magic == GTO_MAGIC || header.magic == GTO_MAGICl
               || header.magic == GTO_MAGIC_TEXT
               || header.magic == GTO_MAGIC_TEXTl;
    }

    void splitComponentName(const char* name, vector<string>& buffer) {}

} // namespace Gto
