//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkApp__Selection__h__
#define __TwkApp__Selection__h__
#include <TwkApp/SelectionType.h>

namespace TwkApp
{
    class SelectionType;

    //
    //  class Selection
    //
    //  Holds a "selection" which is composed of some unknown
    //  objects. Information regarding the selection is held in the
    //  SelectionType class used to initialize it.
    //

    class Selection
    {
    public:
        Selection(const SelectionType* type)
            : m_type(type)
        {
        }

        virtual ~Selection();

        const SelectionType* type() const { return m_type; }

        virtual Selection* copy() const = 0;

    private:
        const SelectionType* m_type;
    };

    //
    //  template class TypedSelection
    //
    //  Convenience class which assembles a selection out of a vector of
    //  some type.
    //

    template <class TContainer> class TypedSelection : public Selection
    {
    public:
        typedef TContainer Container;

        TypedSelection(const SelectionType* type)
            : Selection(type)
        {
        }

        ~TypedSelection();

        virtual Selection* copy() const;

        Container& container() { return m_container; }

        const Container& container() const { return m_container; }

    private:
        Container m_container;
    };

    template <class Container> TypedSelection<Container>::~TypedSelection()
    {
        // nothing
    }

    template <class Container>
    Selection* TypedSelection<Container>::copy() const
    {
        typedef TypedSelection<Container> ThisType;
        ThisType* c = new ThisType(type());
        ;
        c->container() = container();
        return c;
    }

} // namespace TwkApp

#endif // __TwkApp__Selection__h__
