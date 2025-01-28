//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkApp__Command__h__
#define __TwkApp__Command__h__
#include <boost/signals2.hpp>
#include <boost/thread/mutex.hpp>
#include <deque>
#include <map>
#include <string>

namespace TwkApp
{
    class Command;

    //
    //  class CommandInfo
    //
    //  Describes a command. Used to create instances of a particular type
    //  of command. In particular indicates the UndoType of the command:
    //
    //  * Undoable: after doit() is called the command will be entered
    //    into the redo/undo history and may have its undo() and redo()
    //    functions called sometime later if the user chooses.
    //
    //  * NotUndoable: the command changes state in some way. After doit()
    //    is called the command is deleted and does not go into the
    //    history. In addition, the history itself is cleared since the
    //    command made a state change that cannot be tracked.
    //
    //  * EditOnly: the command makes a state change which may not be
    //    necessary for the operation of undo/redo. The command is deleted
    //    after doit() is called and is not entered into the
    //    history. Unlike NotUndoable, the history is left untouched after
    //    doit() is called.
    //
    //  * Marker: the command marks a point in the application's
    //    state. The most useful case might be indicating when the state
    //    was saved to the filesystem. If the user calls undo() and the
    //    marker is on the top of the history stack, that means that the
    //    current state of the application is *also* represented as a file
    //    that was previously saved. In that case the application could
    //    exit safetly without asking for permission. A marker command
    //    does not have its doit(), undo(), or redo() commands called; it
    //    merely exists in the history and is shuffled from the redo to
    //    the undo (and back) stacks along with other commands.
    //

    class CommandInfo
    {
    public:
        typedef std::map<std::string, CommandInfo*> CommandInfos;

        enum UndoType
        {
            Undoable,
            NotUndoable,
            EditOnly,
            Marker
        };

        CommandInfo(const std::string& name, UndoType type = Undoable);
        virtual ~CommandInfo();

        static const CommandInfo* findByName(const std::string&);

        const std::string& name() const { return m_name; }

        //
        //	Create a command object for execution
        //

        virtual Command* newCommand() const = 0;

        bool isUndoable() const { return m_type == Undoable; }

        bool isEditOnly() const { return m_type == EditOnly; }

        bool isMarker() const { return m_type == Marker; }

    private:
        UndoType m_type;
        std::string m_name;
        static CommandInfos m_commands;
    };

    //----------------------------------------------------------------------
    //
    //  class Command
    //
    //  Subclass from this class creates a command. The doit() method is
    //  required. If the command has undo, it may also need to implement
    //  undo() and/or redo(). By default undo() and redo() call doit(), so
    //  if doit for example does a state swap (toggling the state) than
    //  you only need to implement that.
    //
    //  Limitations on what commands are allowed to do:
    //
    //  1. You cannot call queueCommand() on the CommandHistory from
    //     within a command.
    //
    //  2. You cannot touch the CommandHistory in any way from a command
    //     *unless* your command is EditOnly (not undoable, but queuable).
    //
    //  3. Commands are allowed to stash data between undo/redos, but
    //     should avoid stashing pointers at all costs. Objects may change
    //     identities between undo/redo invocations.
    //

    class Command
    {
    public:
        Command(const CommandInfo*);
        virtual ~Command();

        const CommandInfo* info() const { return m_commandInfo; }

        virtual void doit() = 0;
        virtual void undo();
        virtual void redo();

        virtual std::string name() const;

        bool isUndoable() const { return info()->isUndoable(); }

        bool isEditOnly() const { return info()->isEditOnly(); }

        bool isMarker() const { return info()->isMarker(); }

        void setDescription(const std::string& d) { m_description = d; }

        std::string description() const { return m_description; }

    private:
        const CommandInfo* m_commandInfo;
        std::string m_description;
    };

    //----------------------------------------------------------------------
    //
    //  class CommandHistory
    //
    //  An undo and redo stack as well as a facility to handle compound
    //  commands.
    //
    //  The CommandQueue makes it possible for the applciation to queue up
    //  commands without having them executed. This is useful in
    //  multithreaded sitations where you want all state changes to occur
    //  at one time in a controlled manner (e.g. you want a particular
    //  thread to do all the state changes). The CommandQueue is
    //  internally protected with a mutex but its advisable to allow only
    //  a single thread to fill it and the same or another to flush it.
    //

    class CompoundCommand;
    class CompoundCommandInfo;
    typedef std::deque<Command*> CommandStack;
    typedef CommandStack CommandQueue;

    class CommandHistory
    {
    public:
        typedef boost::signals2::signal<void()> VoidSignal;
        typedef boost::mutex Mutex;
        typedef boost::mutex::scoped_lock ScopedLock;

        enum HistoryState
        {
            EditingState,
            UndoState,
            RedoState
        };

        CommandHistory();
        ~CommandHistory();

        //
        //  Each beginCompoundCommand() must be bracketed by a corresponding
        //  endCompoundCommand(). Compound command trees can be created and are
        //  allowed.
        //

        void beginCompoundCommand(const std::string&);
        void endCompoundCommand();

        bool compoundActive() const { return m_compound != 0; }

        void clearUndoHistory();
        void clearRedoHistory();

        void clearHistory()
        {
            clearUndoHistory();
            clearRedoHistory();
        }

        //
        //  This is the main method of creating a command by its type
        //  name. This should be only way an application creates command
        //  instances.
        //

        template <class T> T* newCommand(const std::string&);

        //
        //  Causes immediate call to doit(), undo(), or redo() on the command
        //

        virtual void doCommand(Command*);
        virtual void undoCommand();
        virtual void redoCommand();

        void undoAllCommands();
        void redoAllCommands();

        //
        //  These two are protected by a mutex internally. Queuing a command
        //  does not result in a call to doit(). Flushing the command queue
        //  will causes repeated calls to doCommand() above on the queued
        //  commands and the queue will be empty after returning.
        //

        virtual void queueCommand(Command*);
        virtual void flushCommandQueue();

        //
        //  Direct access to the undo/redo stacks
        //

        const CommandStack& undoStack() const { return m_undoStack; }

        const CommandStack& redoStack() const { return m_redoStack; }

        static std::string topOfStackDescription(const CommandStack&);

        bool hasMarkerCommand(const std::string& name) const;
        bool isMarkerCommandRecent(const std::string& name) const;

        //
        //  Signals
        //

        VoidSignal& undoSizeChangedSignal() { return m_undoSizeChangedSignal; }

        VoidSignal& redoSizeChangedSignal() { return m_redoSizeChangedSignal; }

    protected:
        virtual void doCommandInternal(Command*);

    private:
        void undoCommandInternal();
        void redoCommandInternal();
        void flushCommandQueueInternal();

    protected:
        CompoundCommand* m_compound;
        CommandStack m_undoStack;
        CommandStack m_redoStack;
        VoidSignal m_undoSizeChangedSignal;
        VoidSignal m_redoSizeChangedSignal;
        HistoryState m_historyState;
        CommandQueue m_commandQueue;
        Mutex m_commandQueueMutex;
        static CompoundCommandInfo* m_ccinfo;
    };

    template <class T> T* CommandHistory::newCommand(const std::string& n)
    {
        const CommandInfo* i = CommandInfo::findByName(n);
        return i ? dynamic_cast<T*>(i->newCommand()) : 0;
    }

    //----------------------------------------------------------------------
    //
    //  class CompoundCommandInfo
    //

    class CompoundCommandInfo : public CommandInfo
    {
    public:
        CompoundCommandInfo();
        virtual ~CompoundCommandInfo();
        virtual Command* newCommand() const;
    };

    //----------------------------------------------------------------------
    //
    //  class CompoundCommand
    //
    //  Is its self a CommandHistory so that it can recursively hold
    //  commands for undo. When a compound command is undone, all of its
    //  children are undone. Redo is the same.
    //

    class CompoundCommand
        : public Command
        , public CommandHistory
    {
    public:
        CompoundCommand(const CompoundCommandInfo*);
        virtual ~CompoundCommand();
        virtual void doit();
        virtual void undo();
        virtual void redo();
        virtual std::string name() const;

        void setName(const std::string& n) { m_name = n; }

    private:
        std::string m_name;
    };

    //----------------------------------------------------------------------
    //
    //  class MarkerCommandInfo
    //

    class MarkerCommandInfo : public CommandInfo
    {
    public:
        MarkerCommandInfo();
        virtual ~MarkerCommandInfo();
        virtual Command* newCommand() const;
    };

    //----------------------------------------------------------------------
    //
    //  class MarkerCommand
    //
    //

    class MarkerCommand : public Command
    {
    public:
        MarkerCommand(const MarkerCommandInfo*);
        virtual ~MarkerCommand();
        virtual void doit();
        virtual void undo();
        virtual void redo();
        virtual std::string name() const;

        bool isUnique() const { return m_unique; }

        void setArgs(const std::string& n, bool unique = true)
        {
            m_name = n;
            m_unique = unique;
        }

    private:
        std::string m_name;
        bool m_unique;
    };

    //----------------------------------------------------------------------
    //
    //  class HistoryCommandInfo
    //

    class HistoryCommandInfo : public CommandInfo
    {
    public:
        HistoryCommandInfo();
        virtual ~HistoryCommandInfo();
        virtual Command* newCommand() const;
    };

    //----------------------------------------------------------------------
    //
    //  class HistoryCommand
    //
    //  This command operates on the history itself. This makes it
    //  possible to queue history commands like any other state change (so
    //  they occur interleaved with others). HistoryCommands are not
    //  themselves stored in the history since they are declared as
    //  EditOnly commands.
    //

    class HistoryCommand : public Command
    {
    public:
        enum CommandType
        {
            NoOpCommandType,
            UndoCommandType,
            RedoCommandType,
            ClearCommandType,
            BeginCompoundCommandType,
            EndCompoundCommandType
        };

        HistoryCommand(const HistoryCommandInfo*);
        virtual ~HistoryCommand();
        virtual void doit();
        virtual void undo();
        virtual void redo();
        void setArgs(CommandHistory* h, CommandType t,
                     const std::string& name = "");

    private:
        CommandHistory* m_history;
        CommandType m_commandType;
        std::string m_name;
    };

} // namespace TwkApp

#endif // __TwkApp__Command__h__
