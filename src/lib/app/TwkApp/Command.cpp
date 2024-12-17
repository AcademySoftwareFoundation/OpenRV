//******************************************************************************
// Copyright (c) 2004 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkApp/Command.h>
#include <stl_ext/stl_ext_algo.h>

namespace TwkApp
{
    using namespace std;

    CommandInfo::CommandInfos CommandInfo::m_commands;
    CompoundCommandInfo* CommandHistory::m_ccinfo = 0;

    CommandInfo::CommandInfo(const string& name, UndoType type)
        : m_name(name)
        , m_type(type)
    {
        m_commands[name] = this;
    }

    CommandInfo::~CommandInfo() {}

    const CommandInfo* CommandInfo::findByName(const std::string& name)
    {
        return m_commands[name];
    }

    //----------------------------------------------------------------------

    Command::Command(const CommandInfo* info)
        : m_commandInfo(info)
    {
    }

    Command::~Command() {}

    void Command::undo() { doit(); }

    void Command::redo() { doit(); }

    std::string Command::name() const { return info()->name(); }

    //----------------------------------------------------------------------

    CommandHistory::CommandHistory()
        : m_compound(0)
        , m_historyState(EditingState)
    {
        if (!m_ccinfo)
        {
            m_ccinfo = new CompoundCommandInfo();
        }
    }

    CommandHistory::~CommandHistory()
    {
        assert(m_historyState == EditingState);
        if (m_compound)
            delete m_compound;
        clearHistory();
    }

    void CommandHistory::beginCompoundCommand(const std::string& name)
    {
        assert(m_historyState == EditingState);
        if (m_historyState != EditingState)
            return;

        if (compoundActive())
        {
            m_compound->beginCompoundCommand(name);
        }
        else
        {
            m_compound = static_cast<CompoundCommand*>(m_ccinfo->newCommand());
            m_compound->setName(name);
            m_compound->setDescription(name);
        }
    }

    void CommandHistory::endCompoundCommand()
    {
        assert(m_historyState == EditingState);
        if (m_historyState != EditingState)
            return;

        if (compoundActive())
        {
            if (m_compound->compoundActive())
            {
                m_compound->endCompoundCommand();
            }
            else
            {
                m_undoStack.push_front(m_compound);
                m_compound = 0;
                m_undoSizeChangedSignal();
                clearRedoHistory();
            }
        }
    }

    void CommandHistory::clearUndoHistory()
    {
        assert(m_historyState == EditingState);

        if (m_compound)
        {
            m_compound->clearUndoHistory();
        }
        else
        {
            stl_ext::delete_contents(m_undoStack);
            m_undoStack.clear();
        }

        m_undoSizeChangedSignal();
    }

    void CommandHistory::clearRedoHistory()
    {
        assert(m_historyState == EditingState);

        if (m_compound)
        {
            m_compound->clearRedoHistory();
        }
        else
        {
            stl_ext::delete_contents(m_redoStack);
            m_redoStack.clear();
        }

        redoSizeChangedSignal();
    }

    namespace
    {

        void removeUniqueMarkers(MarkerCommand* m, CommandStack& commandStack)
        {
            if (!m->isUnique())
                return;

            for (int i = 0; i < commandStack.size(); i++)
            {
                if (commandStack[i]->isMarker())
                {
                    MarkerCommand* cm =
                        static_cast<MarkerCommand*>(commandStack[i]);

                    if (cm->name() == m->name())
                    {
                        commandStack.erase(commandStack.begin() + i);
                        i--;
                        delete cm;
                    }
                }
            }
        }

        bool hasMarkerInCommandStack(const string& name,
                                     const CommandStack& commandStack)
        {
            for (int i = 0; i < commandStack.size(); i++)
            {
                if (commandStack[i]->isMarker())
                {
                    MarkerCommand* cm =
                        static_cast<MarkerCommand*>(commandStack[i]);
                    if (cm->name() == name)
                        return true;
                }
            }

            return false;
        }

    } // namespace

    string CommandHistory::topOfStackDescription(const CommandStack& s)
    {
        if (!s.empty())
        {
            if (s[0]->isMarker() && s.size() > 1)
            {
                return s[1]->description();
            }
            else if (!s.empty())
            {
                return s[0]->description();
            }
        }

        return "";
    }

    bool CommandHistory::hasMarkerCommand(const string& name) const
    {
        return hasMarkerInCommandStack(name, m_undoStack)
               || hasMarkerInCommandStack(name, m_redoStack);
    }

    bool CommandHistory::isMarkerCommandRecent(const string& name) const
    {
        if (!m_undoStack.empty() && m_undoStack[0]->isMarker())
        {
            MarkerCommand* cm = static_cast<MarkerCommand*>(m_undoStack[0]);
            return cm->name() == name;
        }

        return false;
    }

    void CommandHistory::doCommand(Command* c)
    {
        ScopedLock lock(m_commandQueueMutex);
        if (!m_commandQueue.empty())
            flushCommandQueueInternal();
        doCommandInternal(c);
    }

    void CommandHistory::doCommandInternal(Command* c)
    {
        assert(m_historyState == EditingState);

        if (m_historyState != EditingState)
        {
            delete c;
            return;
        }

        if (m_compound)
        {
            m_compound->doCommand(c);
        }
        else
        {
            if (c->isUndoable())
            {
                clearRedoHistory();
                c->doit();
                m_undoStack.push_front(c);
                m_undoSizeChangedSignal();
            }
            else if (c->isMarker())
            {
                MarkerCommand* m = static_cast<MarkerCommand*>(c);

                //
                //  Remove all markers with the same name in the history
                //  if its a "unqiue" marker
                //

                removeUniqueMarkers(m, m_undoStack);
                removeUniqueMarkers(m, m_redoStack);
                m_undoStack.push_front(c);
            }
            else
            {
                if (!c->isEditOnly())
                    clearHistory();
                c->doit();
                delete c;
            }
        }
    }

    void CommandHistory::undoCommand()
    {
        assert(!m_compound);
        if (m_compound)
            return;

        assert(m_historyState == EditingState);
        m_historyState = UndoState;

        try
        {
            undoCommandInternal();
        }
        catch (...)
        {
        }

        m_historyState = EditingState;
    }

    void CommandHistory::redoCommand()
    {
        assert(!m_compound);
        if (m_compound)
            return;

        assert(m_historyState == EditingState);
        m_historyState = RedoState;

        try
        {
            redoCommandInternal();
        }
        catch (...)
        {
        }

        m_historyState = EditingState;
    }

    void CommandHistory::undoCommandInternal()
    {
        if (!m_undoStack.empty())
        {
            Command* c = m_undoStack.front();
            m_undoStack.pop_front();
            c->undo();
            m_redoStack.push_front(c);

            if (!m_undoStack.empty() && c->isMarker())
                undoCommandInternal();
        }

        m_redoSizeChangedSignal();
        m_undoSizeChangedSignal();
    }

    void CommandHistory::redoCommandInternal()
    {
        if (!m_redoStack.empty())
        {
            Command* c = m_redoStack.front();
            m_redoStack.pop_front();
            c->redo();
            m_undoStack.push_front(c);
            if (!m_redoStack.empty() && m_redoStack.front()->isMarker())
                redoCommandInternal();
        }

        m_redoSizeChangedSignal();
        m_undoSizeChangedSignal();
    }

    void CommandHistory::undoAllCommands()
    {
        while (!m_undoStack.empty())
            undoCommand();
    }

    void CommandHistory::redoAllCommands()
    {
        while (!m_redoStack.empty())
            redoCommand();
    }

    void CommandHistory::queueCommand(Command* c)
    {
        ScopedLock lock(m_commandQueueMutex);
        m_commandQueue.push_back(c);
    }

    void CommandHistory::flushCommandQueue()
    {
        ScopedLock lock(m_commandQueueMutex);
        flushCommandQueueInternal();
    }

    void CommandHistory::flushCommandQueueInternal()
    {
        for (size_t i = 0; i < m_commandQueue.size(); i++)
        {
            doCommandInternal(m_commandQueue[i]);
        }

        m_commandQueue.clear();
    }

    //------------------------------------------------------------------------------

    CompoundCommand::CompoundCommand(const CompoundCommandInfo* info)
        : Command(info)
    {
    }

    CompoundCommand::~CompoundCommand() {}

    void CompoundCommand::doit() {}

    void CompoundCommand::undo() { undoAllCommands(); }

    void CompoundCommand::redo() { redoAllCommands(); }

    std::string CompoundCommand::name() const { return m_name; }

    //----------------------------------------------------------------------

    CompoundCommandInfo::CompoundCommandInfo()
        : CommandInfo("compound")
    {
    }

    CompoundCommandInfo::~CompoundCommandInfo() {}

    Command* CompoundCommandInfo::newCommand() const
    {
        return new CompoundCommand(this);
    }

    //------------------------------------------------------------------------------

    MarkerCommand::MarkerCommand(const MarkerCommandInfo* info)
        : Command(info)
        , m_unique(true)
    {
    }

    MarkerCommand::~MarkerCommand() {}

    void MarkerCommand::doit() {}

    void MarkerCommand::undo() {}

    void MarkerCommand::redo() {}

    std::string MarkerCommand::name() const { return m_name; }

    //----------------------------------------------------------------------

    MarkerCommandInfo::MarkerCommandInfo()
        : CommandInfo("marker", Marker)
    {
    }

    MarkerCommandInfo::~MarkerCommandInfo() {}

    Command* MarkerCommandInfo::newCommand() const
    {
        return new MarkerCommand(this);
    }

    //------------------------------------------------------------------------------

    HistoryCommand::HistoryCommand(const HistoryCommandInfo* info)
        : Command(info)
        , m_commandType(NoOpCommandType)
        , m_history(0)
    {
    }

    HistoryCommand::~HistoryCommand() {}

    void HistoryCommand::doit()
    {
        if (m_history)
        {
            switch (m_commandType)
            {
            case UndoCommandType:
                m_history->undoCommand();
                break;
            case RedoCommandType:
                m_history->redoCommand();
                break;
            case ClearCommandType:
                m_history->clearHistory();
                break;
            case BeginCompoundCommandType:
                m_history->beginCompoundCommand(m_name);
                break;
            case EndCompoundCommandType:
                m_history->endCompoundCommand();
                break;
            default:
                break;
            }

            m_history = 0;
            m_commandType = NoOpCommandType;
        }
    }

    void HistoryCommand::undo() { abort(); } // should never get here

    void HistoryCommand::redo() { abort(); } // should never get here

    void HistoryCommand::setArgs(CommandHistory* h, CommandType t,
                                 const string& name)
    {
        m_history = h;
        m_commandType = t;
        m_name = name;
    }

    //----------------------------------------------------------------------

    HistoryCommandInfo::HistoryCommandInfo()
        : CommandInfo("history", EditOnly)
    {
    }

    HistoryCommandInfo::~HistoryCommandInfo() {}

    Command* HistoryCommandInfo::newCommand() const
    {
        return new HistoryCommand(this);
    }

    //------------------------------------------------------------------------------

} // namespace TwkApp
