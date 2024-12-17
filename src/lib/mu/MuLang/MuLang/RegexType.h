#ifndef __MuLang__RegexType__h__
#define __MuLang__RegexType__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Class.h>
#include <Mu/ClassInstance.h>
#include <Mu/Node.h>
#include <iosfwd>
#ifdef MU_USE_PCRE
#include <pcre2.h>
#include <pcre2posix.h>
#else
#include <regex.h>
#endif
#include <string>
#include <sys/types.h>

namespace Mu
{

    //
    //  class RegexType
    //
    //  A non-mutable string type.
    //

    class RegexType : public Class
    {
    public:
        RegexType(Context* c, Class* super = 0);
        ~RegexType();

        class Regex : public ClassInstance
        {
        public:
            Regex(const Class*);
            Regex(const Class*, Thread&, const char* s = 0, int flags = 0);
            ~Regex();

            Mu::String& string() { return _std_string; }

            const Mu::String& string() const { return _std_string; }

            int flags() const { return _flags; }

            int maxMatches() const { return _regex.re_nsub; }

            void throwError(Thread& m, int);
            void compile(Thread&, int flags);
            bool matches(Thread&, const Mu::String&, int flags);
            bool smatch(Thread&, const Mu::String&, int flags,
                        regmatch_t* matches, size_t num);

        private:
            Mu::String _std_string;
            regex_t _regex;
            int _flags;
            friend class RegexType;
        };

        //
        //	Return a new Object
        //

        virtual Object* newObject() const;
        virtual void deleteObject(Object*) const;
        virtual void freeze();

        //
        // Output the appropriate Value in human readable form (note: you
        // can call the static function if you already have a std::string.
        //

        virtual void outputValue(std::ostream&, const Value&,
                                 bool full = false) const;
        virtual void outputValueRecursive(std::ostream&, const ValuePointer,
                                          ValueOutputState&) const;
        static void outputQuotedRegex(std::ostream&, const std::string&);

        //
        //	Load function is called when the symbol is added to the
        //	context.
        //

        virtual void load();

        //
        //	Constant
        //

        static NODE_DECLARATION(from_string, Pointer);
        static NODE_DECLARATION(construct, Pointer);
        static NODE_DECLARATION(dereference, Pointer);
        static NODE_DECLARATION(print, void);
        static NODE_DECLARATION(match, bool);
        static NODE_DECLARATION(smatch, Pointer);
        static NODE_DECLARATION(replace, Pointer);
    };

} // namespace Mu

#endif // __MuLang__RegexType__h__
