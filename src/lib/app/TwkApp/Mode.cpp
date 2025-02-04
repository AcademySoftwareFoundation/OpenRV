//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkApp/Mode.h>
#include <TwkApp/Document.h>
#include <TwkApp/EventTable.h>

namespace TwkApp
{

    Mode::Mode(const char* name, Document* doc)
        : m_name(name)
        , m_document(doc)
        , m_defaultMenu(0)
    {
    }

    Mode::~Mode() { clearEventTables(); }

    void Mode::activate() {}

    void Mode::deactivate() {}

    Menu* Mode::menu() { return m_defaultMenu; }

    std::string Mode::sortKey() const { return m_name; }

    void Mode::addEventTable(EventTable* table)
    {
        removeEventTable(table->name());
        m_eventTables[table->name()] = table;
        table->m_mode = this;

        //
        //  m_document has a copy of this table, so invalidate it.
        //
        m_document->invalidateEventTables();
    }

    void Mode::removeEventTable(const std::string& name)
    {
        EventTables::iterator i = m_eventTables.find(name);

        if (i != m_eventTables.end())
        {
            delete (*i).second;
            m_eventTables.erase(i);
            //
            //  m_document has a copy of this table, so invalidate it.
            //
            m_document->invalidateEventTables();
        }
    }

    static void deleteTable(Mode::EventTableEntry& e) { delete e.second; }

    void Mode::clearEventTables()
    {
        for_each(m_eventTables.begin(), m_eventTables.end(), deleteTable);
        m_eventTables.clear();
        //
        //  m_document has a copy of this table, so invalidate it.
        //
        m_document->invalidateEventTables();
    }

    EventTable* Mode::findTableByName(const std::string& name)
    {
        EventTables::iterator i = m_eventTables.find(name);
        return i == m_eventTables.end() ? 0 : (*i).second;
    }

    void Mode::merge(Menu* m)
    {
        if (Menu* mm = menu())
        {
            mm->merge(m);
        }
        else
        {
            delete m_defaultMenu;
            m_defaultMenu = m;
        }
    }

    void Mode::setMenu(Menu* m)
    {
        delete m_defaultMenu;
        m_defaultMenu = m;
    }

    MajorMode::MajorMode(const char* name, Document* doc)
        : Mode(name, doc)
    {
    }

    MinorMode::MinorMode(const char* name, Document* doc)
        : Mode(name, doc)
        , m_sortkey(name)
        , m_order(0)
    {
    }

    MajorMode::~MajorMode() {}

    MinorMode::~MinorMode() {}

    std::string MinorMode::sortKey() const { return m_sortkey; }

} // namespace TwkApp
