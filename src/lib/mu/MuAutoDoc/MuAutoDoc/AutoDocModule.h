#ifndef __MuAutoDoc__AutoDocModule__h__
#define __MuAutoDoc__AutoDocModule__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Module.h>
#include <Mu/Node.h>

namespace Mu
{

    class AutoDocModule : public Module
    {
    public:
        AutoDocModule(Context* c, const char* name);
        virtual ~AutoDocModule();

        virtual void load();

        static NODE_DECLARAION(document_symbol, Pointer);
        static NODE_DECLARAION(document_modules, Pointer);
    };

} // namespace Mu

#endif // __MuAutoDoc__AutoDocModule__h__
