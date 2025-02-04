#ifndef __MuLang__IFStreamType__h__
#define __MuLang__IFStreamType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <MuIO/IStreamType.h>
#include <iosfwd>
#include <iostream>
#include <fstream>

namespace Mu
{

    //
    //  class IFStreamType
    //
    //
    //

    class IFStreamType : public IStreamType
    {
    public:
        IFStreamType(Context* c, const char* name, Class* super);
        ~IFStreamType();

        class IFStream : public IStreamType::IStream
        {
        public:
            IFStream(const Class*);
            ~IFStream();

        private:
            std::ifstream* _ifstream;
            friend class IFStreamType;
        };

        static void finalizer(void*, void*);

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
        //  Nodes
        //

        static NODE_DECLARATION(construct0, Pointer);
        static NODE_DECLARATION(construct, Pointer);
        static NODE_DECLARATION(construct1, Pointer);
        static NODE_DECLARATION(close, void);
        static NODE_DECLARATION(open, void);
        static NODE_DECLARATION(is_open, bool);
    };

} // namespace Mu

#endif // __MuLang__IFStreamType__h__
