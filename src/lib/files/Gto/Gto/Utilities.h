//
// Copyright (c) 2009, Tweak Software
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#ifndef __Gto__Utilities__h__
#define __Gto__Utilities__h__
#include <sys/types.h>
#include <Gto/Header.h>
#include <vector>
#include <string>

namespace Gto
{

    struct DimStruct
    {
        uint32 x;
        uint32 y;
        uint32 z;
        uint32 w;
    };

    inline Dimensions makeDimensions(const DimStruct& ds)
    {
        return Dimensions(ds.x, ds.y, ds.y, ds.w);
    }

    inline DimStruct makeDimStruct(const Dimensions& d)
    {
        DimStruct ds;
        ds.x = d.x;
        ds.y = d.y;
        ds.z = d.z;
        ds.w = d.w;
        return ds;
    }

    struct TypeSpec
    {
        DataType type;
        uint32 size;
        DimStruct dims;
    };

    struct Number
    {
        union
        {
            int _int;
            double _double;
        };

        DataType type;
    };

    //
    //  NOTE: for bufferSize() if xsize, ysize, or zsize == 0 the
    //  dimension is not considered (i.e. think if it as being 1)
    //

    size_t dataSizeInBytes(Gto::uint32);
    size_t elementSize(const TypeSpec&);
    size_t elementSize(const Dimensions&);
    size_t bufferSizeInBytes(Gto::uint32, const Dimensions&);
    const char* typeName(Gto::DataType);
    bool isNumber(Gto::DataType);
    Number asNumber(void*, Gto::DataType);
    bool isGTOFile(const char*);
    void splitComponentName(const char* name, std::vector<std::string>& buffer);

} // namespace Gto

#endif // __Gto__Utilities__h__
