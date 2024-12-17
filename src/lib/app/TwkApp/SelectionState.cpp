//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <TwkApp/SelectionState.h>
#include <stl_ext/stl_ext_algo.h>

namespace TwkApp
{

    SelectionState::SelectionState() {}

    SelectionState::~SelectionState()
    {
        stl_ext::delete_contents(m_selections);
    }

    SelectionState::SelectionState(const SelectionState& state)
    {
        (*this) = state;
    }

    SelectionState& SelectionState::operator=(const SelectionState& state)
    {
        stl_ext::delete_contents(m_selections);
        m_selections.clear();

        for (int i = 0; i < state.m_selections.size(); i++)
        {
            m_selections.push_back(state.m_selections[i]->copy());
        }

        return *this;
    }

    Selection* SelectionState::selection(const std::string& name,
                                         bool createIfNotThere)
    {
        for (int i = 0; i < m_selections.size(); i++)
        {
            if (m_selections[i]->type()->name() == name)
            {
                return m_selections[i];
            }
        }

        if (createIfNotThere)
        {
            if (SelectionType* type = SelectionType::findByName(name))
            {
                m_selections.push_back(type->newSelection());
                return m_selections.back();
            }
        }

        return 0;
    }

    Selection* SelectionState::selection(const SelectionType* t,
                                         bool createIfNotThere)
    {
        for (int i = 0; i < m_selections.size(); i++)
        {
            if (m_selections[i]->type() == t)
            {
                return m_selections[i];
            }
        }

        if (createIfNotThere)
        {
            m_selections.push_back(t->newSelection());
            return m_selections.back();
        }

        return 0;
    }

    const Selection* SelectionState::selection(const std::string& name) const
    {
        return const_cast<SelectionState*>(this)->selection(name);
    }

    void SelectionState::set(Selection* sel)
    {
        if (sel)
        {
            for (int i = 0; i < m_selections.size(); i++)
            {
                if (m_selections[i]->type() == sel->type())
                {
                    delete m_selections[i];
                    m_selections[i] = sel;
                    return;
                }
            }

            m_selections.push_back(sel);
        }
    }

} // namespace TwkApp
