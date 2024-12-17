#ifndef __MuEncoding__EncodingModule__h__
#define __MuEncoding__EncodingModule__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Node.h>
#include <Mu/Module.h>

namespace Mu
{

    class EncodingModule : public Module
    {
    public:
        EncodingModule(Context* c, const char* name);
        virtual ~EncodingModule();

        virtual void load();

        static NODE_DECLARATION(to_base64, Pointer);
        static NODE_DECLARATION(from_base64, Pointer);
        static NODE_DECLARATION(string_to_utf8, Pointer);
        static NODE_DECLARATION(utf8_to_string, Pointer);
    };

} // namespace Mu

#endif // __MuEncoding__EncodingModule__h__
