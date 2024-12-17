#ifndef __MuLang__OStreamType__h__
#define __MuLang__OStreamType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Node.h>
#include <Mu/PrimitiveObject.h>
#include <Mu/PrimitiveType.h>
#include <Mu/Thread.h>
#include <Mu/Vector.h>
#include <MuIO/StreamType.h>
#include <iosfwd>
#include <iostream>
#include <string>

namespace Mu
{

    //
    //  class OStreamType
    //
    //
    //

    class OStreamType : public StreamType
    {
    public:
        OStreamType(Context* c, const char* name, Class* super);
        ~OStreamType();

        class OStream : public StreamType::Stream
        {
        public:
            OStream(const Class*);
            ~OStream();

            std::ostream* _ostream;

        private:
            friend class OStreamType;
        };

        //
        //	Return a new Object
        //

        virtual Object* newObject() const;
        virtual void deleteObject(Object*) const;

        //
        //	Load function is called when the symbol is added to the
        //	context.
        //

        virtual void load();

        //
        //  Functions
        //

        static NODE_DECLARATION(putc, Pointer);
        static NODE_DECLARATION(write, Pointer);
        static NODE_DECLARATION(writeBytes, Pointer);

        static NODE_DECLARATION(tell, int);
        static NODE_DECLARATION(seek, Pointer);
    };

} // namespace Mu

#endif // __MuLang__OStreamType__h__
