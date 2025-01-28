//
// Copyright (c) 2013 Tweak Software.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPMu__CommandsModule__h__
#define __IPMu__CommandsModule__h__
#include <vector>

namespace Mu
{
    class MuLangContext;
}

namespace IPMu
{

    struct PixelImage
    {
        float x;
        float y;
        float px;
        float py;
        bool inside;
        float edgeDistance;
    };

    typedef std::vector<PixelImage> PixelImageVector;

    void initCommands(Mu::MuLangContext*);
} // namespace IPMu

#endif // __IPMu__CommandsModule__h__
