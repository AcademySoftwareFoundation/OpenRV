#ifndef __Mu__Language__h__
#define __Mu__Language__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/config.h>
#include <vector>

namespace Mu
{

    class Symbol;
    class Type;
    class Function;
    class Thread;
    class Class;

    //
    //  class Language
    //
    //  This class describes characteristics of a language implemented on
    //  top of Mu.
    //

    class Language
    {
    public:
        MU_GC_NEW_DELETE

        Language(const String& name, const String& nameSpaceSeparator);
        virtual ~Language();

        //
        //	Types
        //

        typedef const String& StringRef;
        typedef STLVector<const Function*>::Type FunctionVector;
        typedef STLVector<const Type*>::Type TypeVector;

        //
        //	Public API
        //

        const String& name() const { return _name; }

        const String& nameSpaceSeparator() const { return _nsSeparator; }

        void verbose(bool b) { _verbose = b; }

    private:
        String _name;
        String _nsSeparator;
        bool _verbose;
    };

} // namespace Mu

#endif // __Mu__Language__h__
