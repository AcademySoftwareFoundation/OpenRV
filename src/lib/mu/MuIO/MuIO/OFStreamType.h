#ifndef __MuLang__OFStreamType__h__
#define __MuLang__OFStreamType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <MuIO/OStreamType.h>
#include <iosfwd>
#include <iostream>
#include <fstream>

namespace Mu
{

    //
    //  class OFStreamType
    //
    //
    //

    class OFStreamType : public OStreamType
    {
    public:
        OFStreamType(Context* c, const char* name, Class* super);
        ~OFStreamType();

        class OFStream : public OStreamType::OStream
        {
        public:
            OFStream(const Class*);
            ~OFStream();

        private:
            std::ofstream* _ofstream;
            friend class OFStreamType;
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

        static NODE_DECLARATION(construct, Pointer);
        static NODE_DECLARATION(construct0, Pointer);
        static NODE_DECLARATION(construct1, Pointer);
        static NODE_DECLARATION(close, void);
        static NODE_DECLARATION(open, void);
        static NODE_DECLARATION(is_open, bool);
    };

} // namespace Mu

#endif // __MuLang__OFStreamType__h__
