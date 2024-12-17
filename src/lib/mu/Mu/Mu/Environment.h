#ifndef __Mu__Environment__h__
#define __Mu__Environment__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/config.h>
#include <string>
#include <vector>

namespace Mu
{
    namespace Environment
    {

        //
        //  Types
        //

        typedef STLVector<String>::Type SearchPath;
        typedef STLVector<String>::Type ProgramArguments;
        typedef STLVector<String>::Type PathComponents;

        //
        //  Location of Mu home directory
        //

        const String& muHomeLocation();

        //
        //  Module path for .so and .muc files
        //

        const SearchPath& modulePath();
        void setModulePath(const SearchPath& paths);

        const ProgramArguments& programArguments();
        void setProgramArguments(const ProgramArguments& args);

        void pathComponents(const String& path, PathComponents&);

    } // namespace Environment
} // namespace Mu

#endif // __Mu__Environment__h__
