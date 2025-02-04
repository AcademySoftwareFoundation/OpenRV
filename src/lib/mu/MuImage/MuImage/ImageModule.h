#ifndef __runtime__ImageModule__h__
#define __runtime__ImageModule__h__
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

    class ImageModule : public Module
    {
    public:
        ImageModule(Context* c, const char* name);
        virtual ~ImageModule();
        static Module* init(const char* name, Context* c);

        virtual void load();
    };

} // namespace Mu

#endif // __runtime__ImageModule__h__
