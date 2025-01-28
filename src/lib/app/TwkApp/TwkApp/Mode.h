//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkApp__Mode__h__
#define __TwkApp__Mode__h__
#include <TwkApp/SelectionType.h>
#include <TwkApp/Menu.h>
#include <TwkUtil/Notifier.h>
#include <string>
#include <map>

namespace TwkApp
{
    class Document;
    class EventTable;

    /// A feature "unit" in an Document/Application

    ///
    /// An application mode. You should sub-class from MajorMode and Minor
    /// Mode -- not the base class.
    ///

    class Mode
    {
    public:
        //
        //  Types
        //

        typedef std::map<std::string, EventTable*> EventTables;
        typedef EventTables::value_type EventTableEntry;

        //
        //  Constructors
        //

        virtual ~Mode();

        //
        //  Given name
        //

        const std::string& name() const { return m_name; }

        //
        //  By default the sortKey() is the name()
        //

        virtual std::string sortKey() const;

        //
        //  Menu structure. Note: after a merge, the mode owns the passed
        //  in menu.
        //

        virtual Menu* menu();
        virtual void merge(Menu*);
        virtual void setMenu(Menu*);

        //
        //  Event Table(s). Derived classes should add event tables to the
        //  base class. Once you add an event table, its owned by the base
        //  class.
        //

        void addEventTable(EventTable*);
        void removeEventTable(const std::string&);
        void clearEventTables();
        EventTable* findTableByName(const std::string&);

        const EventTables& eventTables() { return m_eventTables; }

        //
        //  Called before mode is activated or before it is deactivated.
        //

        virtual void activate();
        virtual void deactivate();

        //
        //  SelectionMask
        //

        SelectionMask& selectionMask() { return m_selectionMask; }

        const SelectionMask& selectionMask() const { return m_selectionMask; }

        Document* document() const { return m_document; }

    protected:
        Mode(const char* name, Document* doc);

    private:
        std::string m_name;
        SelectionMask m_selectionMask;
        EventTables m_eventTables;
        Document* m_document;
        Menu* m_defaultMenu;
    };

    ///  There is only one active MajorMode in a document.

    class MajorMode : public Mode
    {
    public:
        MajorMode(const char* name, Document* doc);
        virtual ~MajorMode();
    };

    ///  There can be many minor modes active in a document.

    ///
    ///  The order() function can be overloaded to allow the mode to move
    ///  its position in the list of minor modes. By default the order is
    ///  0 in which case the sortkey is used to determine ordering.
    ///

    class MinorMode : public Mode
    {
    public:
        MinorMode(const char* name, Document* doc);
        virtual ~MinorMode();

        int order() const { return m_order; }

        virtual std::string sortKey() const;

        void setOrder(int o) { m_order = o; }

        void setSortKey(const std::string& s) { m_sortkey = s; }

    protected:
        std::string m_sortkey;
        int m_order;
    };

} // namespace TwkApp

#endif // __TwkApp__Mode__h__
