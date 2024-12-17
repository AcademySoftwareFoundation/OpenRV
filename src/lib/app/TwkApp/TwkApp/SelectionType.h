//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkApp__SelectionType__h__
#define __TwkApp__SelectionType__h__
#include <string>
#include <vector>

namespace TwkApp
{
    class Selection;

    //
    //  class SelectionType
    //
    //  SelectionType names a kind of selection and implements the actual
    //  selection method. Kinds of SelectionTypes are things like
    //  "objects" or "polygon edges" or "vertices", etc.
    //
    //  It is necessary to sub-class from this class to implement a new
    //  SelectionType. There is no inherent method of selection, just that
    //  a new Selection object can be returned.
    //

    class SelectionType
    {
    public:
        typedef std::vector<SelectionType*> TypeVector;

        enum Modifier
        {
            Replace,
            Add,
            Subtract
        };

        //
        //  Constructors
        //

        SelectionType(const std::string& name);
        virtual ~SelectionType();

        const std::string& name() const { return m_name; }

        //
        //  Look up in the collection of all SelectionTypes.
        //

        static const TypeVector& allTypes() { return m_allTypes; }

        static SelectionType* findByName(const std::string& name);

        //
        //  Return a selection object for this type
        //

        virtual Selection* newSelection() const = 0;

    private:
        static TypeVector m_allTypes;
        std::string m_name;
    };

    //
    //  A selection mask
    //

    typedef std::vector<SelectionType*> SelectionMask;

} // namespace TwkApp

#endif // __TwkApp__SelectionType__h__
