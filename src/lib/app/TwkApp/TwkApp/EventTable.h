//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkApp__EventTable__h__
#define __TwkApp__EventTable__h__
#include <map>
#include <string>
#include <vector>
#include <TwkApp/Action.h>
#include <TwkUtil/TwkRegEx.h>
#include <TwkMath/Vec2.h>
#include <TwkMath/Box.h>

namespace TwkApp
{
    class Mode;

    ///
    /// class EventTable
    ///
    /// Maps string event names with actions.
    ///

    class EventTable
    {
    public:
        //
        //  Types
        //

        typedef std::map<std::string, Action*> BindingMap;
        typedef BindingMap::value_type Binding;
        typedef TwkUtil::RegEx RegEx;
        typedef std::pair<RegEx, Action*> RegExBinding;
        typedef std::vector<RegExBinding> RegExBindings;
        typedef TwkMath::Vec2i Vec2i;
        typedef TwkMath::Box2i Box2i;

        //
        //  Constructors
        //

        EventTable(const std::string&);
        EventTable(const char* name);
        ~EventTable();

        const std::string& name() const { return m_name; }

        //
        //  Bind an event name to an action. Once you bind the action, it
        //  is owned by the EventTable. If the binding conflicts with an
        //  existing binding, it will replace it.
        //
        //  While you can use regular expressions with bindRegex(), it can
        //  become a performance impairment if too many of them are
        //  needed. In addition, the order in which the regular
        //  expressions are added is the order in which they are matched; so
        //

        void bind(const std::string& event, Action* action);
        void bindRegex(const std::string& eventRegex, Action* action);

        //
        //  Completely remove the binding for event -- deletes the actions
        //

        void unbind(const std::string& event);
        void unbindRegex(const std::string& eventRegex);

        //
        //  ROI
        //

        void setBBox(const Box2i& box) { m_bbox = box; }

        const Box2i& bbox() const { return m_bbox; }

        //
        //  Unbinds all events -- deleting the actions
        //

        void clear();

        //
        //  Find the action for the event (if it exists)
        //

        const Action* query(const std::string&) const;

        //
        //  Iterators
        //

        BindingMap::const_iterator begin() const { return m_map.begin(); }

        BindingMap::const_iterator end() const { return m_map.end(); }

        RegExBindings::const_iterator beginRegex() const
        {
            return m_reBindings.begin();
        }

        RegExBindings::const_iterator endRegex() const
        {
            return m_reBindings.end();
        }

        //
        //  Mode access
        //

        const Mode* mode() const { return m_mode; }

    private:
        std::string m_name;
        BindingMap m_map;
        RegExBindings m_reBindings;
        Box2i m_bbox;
        Mode* m_mode;

        friend class Mode;
    };

} // namespace TwkApp

#endif // __TwkApp__EventTable__h__
