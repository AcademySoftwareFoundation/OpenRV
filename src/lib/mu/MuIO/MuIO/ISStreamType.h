#ifndef __MuLang__ISStreamType__h__
#define __MuLang__ISStreamType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <MuIO/IStreamType.h>

#ifndef TWK_STUB_IT_OUT
#if ((__GNUC__ == 2) && (__GNUC_MINOR__ == 95))
#define TWK_STUB_IT_OUT
#endif
#endif

#ifndef TWK_STUB_IT_OUT
namespace Mu
{

    //
    //  class ISStreamType
    //
    //
    //

    class ISStreamType : public IStreamType
    {
    public:
        ISStreamType(Context* c, const char* name, Class* super);
        ~ISStreamType();

        class ISStream : public IStreamType::IStream
        {
        public:
            ISStream(const Class*);
            ~ISStream();

        private:
            std::istringstream* _isstream;
            friend class ISStreamType;
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
    };

} // namespace Mu

#endif
#endif // __MuLang__ISStreamType__h__
