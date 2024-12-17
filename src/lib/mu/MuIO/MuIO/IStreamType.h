#ifndef __MuLang__IStreamType__h__
#define __MuLang__IStreamType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <MuIO/StreamType.h>
#include <iosfwd>
#include <iostream>
#include <string>

namespace Mu
{

    //
    //  class IStreamType
    //
    //
    //

    class IStreamType : public StreamType
    {
    public:
        IStreamType(Context* c, const char* name, Class* super);
        ~IStreamType();

        class IStream : public StreamType::Stream
        {
        public:
            IStream(const Class*);
            ~IStream();

            std::istream* _istream;

        private:
            friend class IStreamType;
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

        static NODE_DECLARATION(sgetc, char);
        static NODE_DECLARATION(gets, Pointer);
        static NODE_DECLARATION(read, char);
        static NODE_DECLARATION(readBytes, int);
        static NODE_DECLARATION(seek, Pointer);
        static NODE_DECLARATION(seek2, Pointer);
        static NODE_DECLARATION(tell, int);
        static NODE_DECLARATION(count, int);
        static NODE_DECLARATION(putback, Pointer);
        static NODE_DECLARATION(unget, Pointer);
        static NODE_DECLARATION(in_avail, int);
    };

} // namespace Mu

#endif // __MuLang__IStreamType__h__
