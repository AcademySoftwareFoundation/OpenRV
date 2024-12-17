//******************************************************************************
// Copyright (c) 2004 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkApp/EventTable.h>
#include <iostream>
#include <algorithm>

namespace TwkApp
{
    using namespace std;
    using namespace TwkUtil;

    EventTable::EventTable(const string& name)
        : m_name(name)
        , m_mode(0)
    {
        m_bbox.makeInfinite();
    }

    EventTable::EventTable(const char* name)
        : m_name(name)
        , m_mode(0)
    {
        m_bbox.makeInfinite();
    }

    EventTable::~EventTable() { clear(); }

    void EventTable::bind(const std::string& event, Action* action)
    {
        unbind(event);
        m_map[event] = action;
    }

    void EventTable::bindRegex(const std::string& eventRegex, Action* action)
    {
        unbindRegex(eventRegex);
        m_reBindings.push_back(RegExBinding(eventRegex, action));
    }

    void EventTable::unbind(const std::string& event)
    {
        BindingMap::iterator i = m_map.find(event);

        if (i != m_map.end())
        {
            delete (*i).second;
            m_map.erase(i);
        }
    }

    void EventTable::unbindRegex(const std::string& eventRegex)
    {
        for (int i = 0; i < m_reBindings.size(); i++)
        {
            if (m_reBindings[i].first.pattern() == eventRegex)
            {
                m_reBindings.erase(m_reBindings.begin() + i);
                i--;
            }
        }
    }

    static void deleteAction(EventTable::Binding& b) { delete b.second; }

    void EventTable::clear()
    {
        for_each(m_map.begin(), m_map.end(), deleteAction);
        m_map.clear();

        for (int i = 0; i < m_reBindings.size(); i++)
        {
            delete m_reBindings[i].second;
        }

        m_reBindings.clear();
    }

    const Action* EventTable::query(const string& event) const
    {
        BindingMap::const_iterator i = m_map.find(event);

        if (i != m_map.end())
        {
            return (*i).second;
        }
        else if (m_reBindings.empty())
        {
            return 0;
        }
        else
        {
            for (int i = 0; i < m_reBindings.size(); i++)
            {
                if (Match(m_reBindings[i].first, event))
                {
                    return m_reBindings[i].second;
                }
            }
        }

        return 0;
    }

} // namespace TwkApp
