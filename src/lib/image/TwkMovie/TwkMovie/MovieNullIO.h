//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkMovie__MovieNullIO__h__
#define __TwkMovie__MovieNullIO__h__
#include <TwkMovie/MovieIO.h>
#include <TwkMovie/dll_defs.h>

namespace TwkMovie
{

    /// The plugin interface to NullWriter

    class TWKMOVIE_EXPORT MovieNullIO : public MovieIO
    {
    public:
        MovieNullIO();
        virtual ~MovieNullIO();

        virtual std::string about() const;

        virtual MovieReader* movieReader() const;
        virtual MovieWriter* movieWriter() const;
    };

} // namespace TwkMovie

#endif // __TwkMovie__MovieNullIO__h__
