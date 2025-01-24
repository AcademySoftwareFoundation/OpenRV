//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <TwkApp/Action.h>
#include <TwkApp/Application.h>
#include <TwkApp/Event.h>
#include <TwkApp/Document.h>
#include <TwkApp/Menu.h>
#include <TwkApp/EventTable.h>
#include <stl_ext/stl_ext_algo.h>
#include <algorithm>
#include <functional>

namespace TwkApp
{
    using namespace TwkUtil;
    using namespace std;
    using namespace TwkMath;

    Document::Documents Document::m_documents;
    Document* Document::m_eventDocument = 0;

    NOTIFIER_MESSAGE_IMP(Document, modeChangedMessage, "document mode changed")
    NOTIFIER_MESSAGE_IMP(Document, menuChangedMessage, "document menu changed")
    NOTIFIER_MESSAGE_IMP(Document, selectionChangedMessage,
                         "document selection changed")
    NOTIFIER_MESSAGE_IMP(Document, activeMessage, "document active")
    NOTIFIER_MESSAGE_IMP(Document, bindingsChangedMessage,
                         "document bindings changed")
    NOTIFIER_MESSAGE_IMP(Document, modeAddedMessage, "document mode added")
    NOTIFIER_MESSAGE_IMP(Document, historyChangedMessage,
                         "document history changed")
    NOTIFIER_MESSAGE_IMP(Document, deleteMessage, "document deleted")
    NOTIFIER_MESSAGE_IMP(Document, filenameChangedMessage,
                         "document filename changed")

    Document::Document()
        : TwkUtil::Notifier()
        , EventNode("document")
        , m_majorMode(0)
        , m_menu(0)
        , m_eventTableDirty(true)
        , m_event(0)
        , m_opaquePointer(0)
        , m_focusTable(0)
        , m_fileName("Untitled")
    {
        m_documents.push_back(this);
        if (App())
            App()->add(this);
    }

    Document::~Document()
    {
        bool active = activeDocument() == this;

        deleteModes();

        stl_ext::remove(m_documents, this);

        if (active && !m_documents.empty())
        {
            m_documents.front()->makeActive();
        }

        m_deleteSignal(this);
        send(deleteMessage(), this);
    }

    void Document::deleteModes()
    {
        for (Modes::iterator i = m_modes.begin(); i != m_modes.end(); ++i)
        {
            delete (*i).second;
        }

        m_modes.clear();
        m_majorMode = 0;
        m_minorModes.clear();
        delete m_menu;
        m_menu = 0;
        m_menuChangedSignal(this);
        send(menuChangedMessage());
    }

    void Document::makeActive()
    {
        Documents::iterator i =
            find(m_documents.begin(), m_documents.end(), this);

        if (i != m_documents.end())
        {
            swap(m_documents.front(), *i);
            m_activeSignal(this);
            send(activeMessage(), this);
        }
    }

    void Document::setFileName(const std::string& newName)
    {
        m_fileName = newName;
        m_fileNameChangedSignal(this);
        send(filenameChangedMessage());
    }

    const std::string& Document::fileName() const { return m_fileName; }

    std::string Document::filePath() const { return m_fileName; }

    void Document::read(const string&, const ReadRequest&)
    {
        // nothing
    }

    void Document::write(const string&, const WriteRequest&)
    {
        // nothing
    }

    void Document::clear() {}

    void Document::doCommand(Command* c)
    {
        CommandHistory::doCommand(c);
        m_historyChangedSignal(this);
        send(historyChangedMessage(), this);
    }

    void Document::undoCommand()
    {
        CommandHistory::undoCommand();
        m_historyChangedSignal(this);
        send(historyChangedMessage(), this);
    }

    void Document::redoCommand()
    {
        CommandHistory::redoCommand();
        m_historyChangedSignal(this);
        send(historyChangedMessage(), this);
    }

    void Document::activateMode(const std::string& name)
    {
        if (Mode* m = findModeByName(name))
        {
            activateMode(m);
        }
        else
        {
            cerr << "WARNING: tried to activate non-existant mode " << name
                 << endl;
        }
    }

    void Document::deactivateMode(const std::string& name)
    {
        if (Mode* m = findModeByName(name))
        {
            deactivateMode(m);
        }
        else
        {
        }
    }

    bool Document::isModeActive(const std::string& name)
    {
        if (Mode* m = findModeByName(name))
        {
            return isModeActive(m);
        }
        else
        {
            return false;
        }
    }

    void Document::activateMode(Mode* mode)
    {
        if (MajorMode* majorMode = dynamic_cast<MajorMode*>(mode))
        {
            if (m_majorMode == majorMode)
                return;

            if (m_majorMode)
            {
                m_majorMode->deactivate();
            }

            if (m_majorMode = majorMode)
            {
                m_majorMode->activate();
            }
        }
        else
        {
            MinorMode* minorMode = static_cast<MinorMode*>(mode);
            MinorModes::iterator i =
                find(m_minorModes.begin(), m_minorModes.end(), minorMode);

            if (i == m_minorModes.end())
            {
                m_minorModes.insert(minorMode);
                minorMode->activate();
            }
            else
            {
                return;
            }
        }

        // see header file note
        invalidateEventTables();
        m_modeChangedSignal(this);
        send(modeChangedMessage());
        if (mode->menu())
            invalidateMenu();
    }

    void Document::deactivateMode(Mode* mode)
    {
        if (MajorMode* majorMode = dynamic_cast<MajorMode*>(mode))
        {
            return;
        }
        else
        {
            MinorMode* minorMode = static_cast<MinorMode*>(mode);
            MinorModes::iterator i =
                find(m_minorModes.begin(), m_minorModes.end(), minorMode);

            if (i != m_minorModes.end())
            {
                m_minorModes.erase(i);
                minorMode->deactivate();
            }
            else
            {
                return;
            }
        }

        // see header file note
        invalidateEventTables();
        m_modeChangedSignal(this);
        send(modeChangedMessage());
        if (mode->menu())
            invalidateMenu();
    }

    bool Document::isModeActive(Mode* m)
    {
        if (m == m_majorMode)
            return true;

        if (MinorMode* minorMode = dynamic_cast<MinorMode*>(m))
        {
            MinorModes::iterator i =
                find(m_minorModes.begin(), m_minorModes.end(), minorMode);
            return i != m_minorModes.end();
        }

        return false;
    }

    Mode* Document::findModeByName(const std::string& name) const
    {
        Modes::const_iterator i = m_modes.find(name);
        if (i == m_modes.end())
            return 0;
        return (*i).second;
    }

    void Document::addMode(Mode* mode)
    {
        if (findModeByName(mode->name()))
        {
            TWK_THROW_STREAM(DocumentException,
                             "Duplicate mode: " << mode->name());
        }

        m_modes.insert(Modes::value_type(mode->name(), mode));
        invalidateEventTables();
        m_modeAddedSignal(this);
        send(modeAddedMessage());
    }

    void Document::setSelection(const SelectionState& state)
    {
        m_selectionState = state;
        m_selectionChangedSignal(this);
        send(selectionChangedMessage());
    }

    void Document::invalidateMenu()
    {
        delete m_menu;
        m_menu = 0;
        m_menuChangedSignal(this);
        send(menuChangedMessage());
    }

    void Document::invalidateEventTables() { m_eventTableDirty = true; }

    Menu* Document::menu()
    {
        if (!m_menu && m_majorMode)
        {
            m_menu = new Menu(m_majorMode->menu());

            for (MinorModes::iterator i = m_minorModes.begin();
                 i != m_minorModes.end(); ++i)
            {
                m_menu->merge((*i)->menu());
            }
        }

        return m_menu;
    }

    void Document::buildEventTables()
    {
        m_eventTableStack.clear();

        if (m_eventTableNameStack.empty())
        {
            m_eventTableNameStack.push_back("global");
        }

        //
        //  Retain minormode ordering here, so that event tables are
        //  checked in the correct order.
        //

        for (EventTableNameStack::const_iterator ni =
                 m_eventTableNameStack.begin();
             ni != m_eventTableNameStack.end(); ++ni)
        {
            const string& tname = *ni;

            //
            //  do the major mode
            //

            EventTables::const_iterator ti =
                m_majorMode->eventTables().find(tname);

            if (ti != m_majorMode->eventTables().end())
            {
                m_eventTableStack.push_back(ti->second);
            }

            //
            //  do the minor modes
            //

            for (MinorModes::reverse_iterator mi = m_minorModes.rbegin();
                 mi != m_minorModes.rend(); ++mi)
            {
                const EventTables& tables = (*mi)->eventTables();
                EventTables::const_iterator ti = tables.find(tname);

                if (ti != tables.end())
                {
                    m_eventTableStack.push_back(ti->second);
                }
            }
        }

        setFocusTable(0);
        m_eventTableDirty = false;
        m_bindingsChangedSignal(this);
        send(bindingsChangedMessage());
    }

    void Document::setFocusTable(const EventTable* t) { m_focusTable = t; }

    Document::ActionTablePair Document::queryEvent(const Event& event,
                                                   const Action* after)
    {
        const EventTable* t = 0;

        bool foundAfter = after ? false : true;

        const PointerEvent* pe = dynamic_cast<const PointerEvent*>(&event);

        if (pe && m_focusTable)
        {
            if (pe->buttonStates() == 0)
            {
                setFocusTable(0);
            }
            else if (const Action* a = m_focusTable->query(event.name()))
            {
                if (foundAfter)
                {
                    return ActionTablePair(a, m_focusTable);
                }
                else
                {
                    return ActionTablePair((const Action*)0,
                                           (const EventTable*)0);
                }
            }
            else
            {
                return ActionTablePair((const Action*)0, (const EventTable*)0);
            }
        }

        for (int i = m_eventTableStack.size() - 1; t == 0 && i >= 0; i--)
        {
            t = m_eventTableStack[i];

            if (t && pe)
            {
                Vec2i start(pe->startX(), pe->startY());
                Vec2i current(pe->x(), pe->y());

                if (t->bbox().intersects(start)
                    || t->bbox().intersects(current))
                {
                    if (pe->buttonStates())
                    {
                        setFocusTable(t);
                    }
                }
                else
                {
                    t = 0;
                }
            }

            if (t)
            {
                const Action* a = t->query(event.name());

                if (foundAfter && a)
                {
                    return ActionTablePair(a, t);
                }
                else if (a == after)
                {
                    foundAfter = true;
                }

                t = 0;
            }
        }

        if (after && !foundAfter)
        {
            //
            //  This is a special case -- if an event action remapped the
            //  event and rejected it, you won't find after. So let's
            //  assume that happened and the intention is to find a taker
            //  in the remapped event. In other words -- start over
            //

            return queryEvent(event, 0);
        }
        else
        {
            return ActionTablePair((const Action*)0, (const EventTable*)0);
        }
    }

    static bool showEvents = false;

    void Document::executeAction(const Event& event)
    {
        const Action* after = 0;
        static TwkUtil::Timer* debugTimer = 0;
        float startTime;

        if (showEvents && !debugTimer)
        {
            debugTimer = new TwkUtil::Timer();
            debugTimer->start();
        }

        for (;;)
        {
            ActionTablePair atp = queryEvent(event, after);

            if (const Action* a = atp.first)
            {
                event.m_table = atp.second;

                bool outputEvent =
                    (showEvents && event.name() != "render"
                     && event.name() != "pre-render"
                     && event.name() != "post-render"
                     && event.name() != "per-render-event-processing");

                if (outputEvent)
                {
                    cerr << "Action begin on Event: '" << event.name() << "'"
                         << endl;
                    startTime = debugTimer->elapsed();
                }

                a->execute(this, event);

                if (outputEvent)
                {
                    /*
                    XXX For some reason the docString() below is sometimes so
                        corrupted that RV will crash.  Haven't been able to
                    figure out why.

                    cerr << "Action complete on Event: '" << event.name() << "'
                    " <<
                            ((event.handled) ? "handled" : "processed") <<
                            " by action '" << a->docString() << "' from table ";
                    */

                    cerr << "Action complete on Event: '" << event.name()
                         << "' " << ((event.handled) ? "handled" : "processed")
                         << " by table ";
                    if (atp.second->mode())
                        cerr << atp.second->mode()->name() << ":";
                    cerr << atp.second->name() << " ("
                         << 1000.0 * (debugTimer->elapsed() - startTime)
                         << "ms)" << endl;
                }

                if (event.handled)
                    return;
                else if (atp.second == m_focusTable)
                //
                //  At this point m_focusTable has rejected the event, so reset
                //  m_focusTable so that other tables have a chance at the event
                //  next time around.
                //
                {
                    setFocusTable(0);
                }
            }
            else
            {
                event.handled = false;
                return;
            }

            after = atp.first;
        }
    }

    void Document::pushTable(const string& name)
    {
        EventTableNameStack::iterator i = find(
            m_eventTableNameStack.begin(), m_eventTableNameStack.end(), name);

        if (i != m_eventTableNameStack.end())
        {
            m_eventTableNameStack.erase(i);
        }

        m_eventTableNameStack.push_back(name);
        m_eventTableDirty = true;
        setFocusTable(0);
    }

    void Document::popTable()
    {
        if (m_eventTableNameStack.empty())
        {
            TWK_THROW_STREAM(DocumentException, "event table empty; can't pop");
        }
        else
        {
            m_eventTableNameStack.resize(m_eventTableNameStack.size() - 1);

            if (m_eventTableNameStack.empty())
            {
                m_eventTableNameStack.push_back("global");
            }

            m_eventTableDirty = true;
        }
    }

    void Document::popTable(const string& name)
    {
        setFocusTable(0);

        EventTableNameStack::iterator i = find(
            m_eventTableNameStack.begin(), m_eventTableNameStack.end(), name);

        if (i != m_eventTableNameStack.end())
        {
            m_eventTableNameStack.erase(i);
        }
        else
        {
            TWK_THROW_STREAM(DocumentException,
                             "bad event table name: \"" << name << "\"");
        }

        m_eventTableDirty = true;
    }

    void Document::debugEvents() { showEvents = true; }

    const Document::EventTableStack& Document::eventTableStack() const
    {
        if (m_eventTableDirty)
            const_cast<Document*>(this)->buildEventTables();
        return m_eventTableStack;
    }

    EventNode::Result Document::receiveEvent(const Event& event)
    {
        const Event* lastEvent = m_event;
        Document* d = m_eventDocument;
        m_eventDocument = this;

        bool outputEvent =
            (showEvents && event.name() != "render"
             && event.name() != "pre-render" && event.name() != "post-render"
             && event.name() != "per-render-event-processing");

        if (outputEvent)
        {
            cerr << event;
            if (const RenderEvent* e = dynamic_cast<const RenderEvent*>(&event))
                cerr << " '" << e->stringContent() << "'";
            else if (const DragDropEvent* e =
                         dynamic_cast<const DragDropEvent*>(&event))
                cerr << " '" << e->stringContent() << "'";
            else if (const GenericStringEvent* e =
                         dynamic_cast<const GenericStringEvent*>(&event))
                cerr << " '" << e->stringContent() << "'";
            cerr << endl;
        }
        m_event = &event;
        if (m_eventTableDirty)
            buildEventTables();
        executeAction(event);
        m_event = lastEvent;
        m_eventDocument = d;
        return event.handled ? EventAccept : EventIgnored;
    }

} // namespace TwkApp
