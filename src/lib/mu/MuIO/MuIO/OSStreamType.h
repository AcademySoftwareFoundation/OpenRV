#ifndef __MuLang__OSStreamType__h__
#define __MuLang__OSStreamType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <MuIO/OStreamType.h>

#ifndef TWK_STUB_IT_OUT
#if ((__GNUC__ == 2) && (__GNUC_MINOR__ == 95))
#define TWK_STUB_IT_OUT
#endif
#endif

#ifndef TWK_STUB_IT_OUT
namespace Mu
{

    //
    //  class OSStreamType
    //
    //
    //

    class OSStreamType : public OStreamType
    {
    public:
        OSStreamType(Context* c, const char* name, Class* super);
        ~OSStreamType();

        class OSStream : public OStreamType::OStream
        {
        public:
            OSStream(const Class*);
            ~OSStream();

        private:
            std::ostringstream* _osstream;
            friend class OSStreamType;
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
        //  Nodes
        //

        static NODE_DECLARATION(construct, Pointer);
        static NODE_DECLARATION(tostring, Pointer);
    };

} // namespace Mu

#endif
#endif // __MuLang__OSStreamType__h__
