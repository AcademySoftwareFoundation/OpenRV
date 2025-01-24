#ifndef __MuLang__StreamType__h__
#define __MuLang__StreamType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Class.h>
#include <Mu/ClassInstance.h>
#include <Mu/Node.h>
#include <Mu/Thread.h>
#include <Mu/Vector.h>
#include <iosfwd>
#include <string>

namespace Mu
{

    //
    //  class StreamType
    //
    //
    //

    class StreamType : public Class
    {
    public:
        StreamType(Context* c, const char* name, Class* super = 0);
        ~StreamType();

        class Stream : public ClassInstance
        {
        public:
            Stream(const Class*);
            ~Stream();

            //
            //  The string could be an annotation (like a file name) or
            //  the contents of a stream ([oi]sstream).
            //

            const Mu::String& string() const { return _string; }

            void setString(const Mu::String& n) { _string = n; }

            std::ios* _ios;

        private:
            Mu::String _string;
            friend class StreamType;
        };

        //
        //	Return a new Object
        //

        virtual Object* newObject() const;
        virtual void deleteObject(Object*) const;

        //
        // Output the appropriate Value in human readable form (note: you
        // can call the static function if you already have a std::string.
        //

        virtual void outputValue(std::ostream&, const Value&,
                                 bool full = false) const;
        virtual void outputValueRecursive(std::ostream&, const ValuePointer,
                                          ValueOutputState&) const;

        //
        //	Load function is called when the symbol is added to the
        //	context.
        //

        virtual void load();

        //
        //	Constant
        //

        static NODE_DECLARATION(print, void);

        //
        //  Function
        //

        static NODE_DECLARATION(eval, bool);
        static NODE_DECLARATION(toBool, bool);
        static NODE_DECLARATION(good, bool);
        static NODE_DECLARATION(bad, bool);
        static NODE_DECLARATION(fail, bool);
        static NODE_DECLARATION(eof, bool);
        static NODE_DECLARATION(clear, void);
        static NODE_DECLARATION(clearflag, void);
        static NODE_DECLARATION(rdstate, int);
        static NODE_DECLARATION(setstate, void);
    };

} // namespace Mu

#endif // __MuLang__StreamType__h__
