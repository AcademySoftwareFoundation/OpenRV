//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkApp__SelectionState__h__
#define __TwkApp__SelectionState__h__
#include <TwkApp/Selection.h>

namespace TwkApp
{

    //
    //  class SelectionState
    //
    //  Holds a bunch of selection objects.

    class SelectionState
    {
    public:
        typedef std::vector<Selection*> SelectionVector;

        //
        //  The SelectionState can be copied in which case a deep copy occurs.
        //

        SelectionState();
        SelectionState(const SelectionState&);
        ~SelectionState();

        SelectionState& operator=(const SelectionState&);

        //
        //  Returns a selection of the type named (it may create one if it
        //  does not yet exist.
        //

        Selection* selection(const std::string& name,
                             bool createIfNotThere = true);
        Selection* selection(const SelectionType*,
                             bool createIfNotThere = true);
        const Selection* selection(const std::string& name) const;

        //
        //  Set a particular selection. The passed in pointer will be
        //  owned by the SelectionState.
        //

        void set(Selection*);

        //
        //  Convenience: returns a pointer to a cast things.
        //

        template <class T> T* selectionOfType(const std::string& name);
        template <class T>
        const T* selectionOfType(const std::string& name) const;

        //
        //  All of the selection objects
        //

        const SelectionVector& selections() const { return m_selections; }

    private:
        SelectionVector m_selections;
    };

    //
    //  Convenient member template
    //

    template <class T>
    T* SelectionState::selectionOfType(const std::string& name)
    {
        return dynamic_cast<T*>(selection(name));
    }

    template <class T>
    const T* SelectionState::selectionOfType(const std::string& name) const
    {
        return dynamic_cast<const T*>(selection(name));
    }

} // namespace TwkApp

#endif // __TwkApp__SelectionState__h__
