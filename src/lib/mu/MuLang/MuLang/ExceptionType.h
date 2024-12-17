#ifndef __MuLang__ExceptionType__h__
#define __MuLang__ExceptionType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Class.h>
#include <Mu/ClassInstance.h>
#include <Mu/Exception.h>
#include <Mu/Node.h>
#include <iosfwd>
#include <string>

namespace Mu
{

    //
    //  class ExceptionType
    //
    //  A non-mutable string type.
    //

    class ExceptionType : public Class
    {
    public:
        ExceptionType(Context*, Class* super = 0);
        ~ExceptionType();

        class Exception : public ClassInstance
        {
        public:
            Exception(const Class*);

            Mu::String& string() { return _string; }

            const Mu::String& string() const { return _string; }

            void setBackTrace(Mu::Exception& e) { _backtrace = e.backtrace(); }

            const Thread::BackTrace& backtrace() const { return _backtrace; }

            Thread::BackTrace& backtrace() { return _backtrace; }

            String backtraceAsString() const;

        private:
            Mu::String _string;
            Thread::BackTrace _backtrace;
        };

        //
        //	Return a new Object
        //

        virtual Object* newObject() const;
        virtual void deleteObject(Object*) const;

        //
        //	Output the symbol name
        //	Output the appropriate Value in human readable form
        //

        virtual void outputValueRecursive(std::ostream&, const ValuePointer,
                                          ValueOutputState&) const;

        //
        //	Load function is called when the symbol is added to the
        //	context.
        //

        virtual void load();

        virtual void freeze();

        //
        //	Nodes
        //

        static NODE_DECLARATION(construct, Pointer);
        static NODE_DECLARATION(stringCast, Pointer);
        static NODE_DECLARATION(dereference, Pointer);
        static NODE_DECLARATION(print, void);
        static NODE_DECLARATION(assign, Pointer);
        static NODE_DECLARATION(equals, bool);
        static NODE_DECLARATION(copy, Pointer);
        static NODE_DECLARATION(mu__try, void);
        static NODE_DECLARATION(mu__catch, bool);
        static NODE_DECLARATION(mu__catch_all, bool);
        static NODE_DECLARATION(mu__exception, Pointer);
        static NODE_DECLARATION(mu__throw, void);
        static NODE_DECLARATION(mu__throw_exception, void);
        static NODE_DECLARATION(mu__rethrow, void);
        static NODE_DECLARATION(backtrace, Pointer);
    };

} // namespace Mu

#endif // __MuLang__ExceptionType__h__
