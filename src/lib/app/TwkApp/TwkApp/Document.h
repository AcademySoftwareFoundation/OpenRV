//*****************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************
#ifndef __TwkApp__Document__h__
#define __TwkApp__Document__h__
#include <TwkApp/Command.h>
#include <TwkApp/EventNode.h>
#include <TwkApp/Exception.h>
#include <TwkApp/Mode.h>
#include <TwkApp/Selection.h>
#include <TwkApp/SelectionState.h>
#include <TwkApp/SelectionType.h>
#include <TwkUtil/Notifier.h>
#include <boost/any.hpp>
#include <map>
#include <set>
#include <vector>
#include <boost/signals2.hpp>

namespace TwkApp
{
    class Menu;
    class EventTable;

    //
    //  class Document
    //
    //  Borrowing from the ideas in Cocoa, this is a base class for a
    //  thing being edited by the app. An example is the TwkModel::Scene
    //  object which is a document. This class provides API for reading
    //  and writing files, etc. The Application classes can work with this
    //  class without knowing about the implementation. Unlike Cocoa, you
    //  should certinaly inherit from this class and make your own
    //  document type.
    //
    //  The Document manages editing modes (Mode.h) and selection
    //  (SelectionState.h). Its assumed that most things you're likely to be
    //  editing need these operations.
    //

    class Document
        : public EventNode
        , public TwkUtil::Notifier
        , public CommandHistory
    {
    public:
        //
        //  Types
        //

        typedef std::map<std::string, boost::any> OptionMap;
        typedef boost::signals2::signal<void()> VoidSignal;
        typedef boost::signals2::signal<void(Document*)> DocumentSignal;
        typedef boost::signals2::signal<void(const std::string&)> StringSignal;
        typedef std::vector<Command*> CommandQueue;

        struct FileIORequest
        {
            FileIORequest() {}

            //
            //  request.setOption("flag", true);
            //  request.setOption("foo", string("string value"));
            //  request.setOption("bar", int(123));
            //  request.setOption("baz", float(123.321));
            //

            void setOption(const std::string& name, const boost::any& value)
            {
                options[name] = value;
            }

            //
            //  const bool b     = request.optionValue<bool>("flag", false);
            //  const string foo = request.optionValue<string>("foo",
            //  "defaultValue"); const int bar    =
            //  request.optionValue<int>("bar", 0); const float baz  =
            //  request.optionValue<float>("baz", 0.0f);
            //

            template <typename T>
            T optionValue(const std::string& name,
                          const T& defaultValue = T()) const
            {
                OptionMap::const_iterator i = options.find(name);
                if (i != options.end())
                    return boost::any_cast<T>(i->second);
                return defaultValue;
            }

            OptionMap options;
        };

        typedef FileIORequest ReadRequest;
        typedef FileIORequest WriteRequest;

        struct MinorModeComp
        {
            bool operator()(MinorMode* a, MinorMode* b) const
            {
                return a->order() != b->order() ? a->order() < b->order()
                       : a->sortKey() != b->sortKey()
                           ? a->sortKey() < b->sortKey()
                           : a < b;
            }
        };

        typedef std::map<std::string, TwkApp::Mode*> Modes;
        typedef std::set<TwkApp::MinorMode*, MinorModeComp> MinorModes;
        typedef std::map<std::string, EventTable*> EventTables;
        typedef EventTables::value_type EventTableEntry;
        typedef std::vector<const EventTable*> EventTableStack;
        typedef std::vector<std::string> EventTableNameStack;
        typedef std::vector<Document*> Documents;
        typedef std::pair<const Action*, const EventTable*> ActionTablePair;

        typedef bool (*TablePredicate)(const Document&, const EventTable&,
                                       const Event&);

        //
        //  Signals
        //

        DocumentSignal& documentDeleteSignal() { return m_deleteSignal; }

        DocumentSignal& documentActiveSignal() { return m_activeSignal; }

        DocumentSignal& fileNameChangedSignal()
        {
            return m_fileNameChangedSignal;
        }

        DocumentSignal& historyChangedSignal()
        {
            return m_historyChangedSignal;
        }

        DocumentSignal& modeChangedSignal() { return m_modeChangedSignal; }

        DocumentSignal& menuChangedSignal() { return m_menuChangedSignal; }

        DocumentSignal& selectionChangedSignal()
        {
            return m_selectionChangedSignal;
        }

        DocumentSignal& bindingsChangedSignal()
        {
            return m_bindingsChangedSignal;
        }

        DocumentSignal& modeAddedSignal() { return m_modeAddedSignal; }

        //
        //  Messages
        //

        NOTIFIER_MESSAGE(modeChangedMessage);
        NOTIFIER_MESSAGE(menuChangedMessage);
        NOTIFIER_MESSAGE(selectionChangedMessage);
        NOTIFIER_MESSAGE(activeMessage);
        NOTIFIER_MESSAGE(bindingsChangedMessage);
        NOTIFIER_MESSAGE(modeAddedMessage);
        NOTIFIER_MESSAGE(historyChangedMessage);
        NOTIFIER_MESSAGE(deleteMessage);
        NOTIFIER_MESSAGE(filenameChangedMessage);

        //
        //  Constructor/Destructor
        //

        Document();
        virtual ~Document();

        //
        //  Active / All documents
        //

        static Document* activeDocument()
        {
            return m_documents.empty() ? 0 : m_documents.front();
        }

        static const Documents& documents() { return m_documents; }

        static Document* eventDocument() { return m_eventDocument; }

        //
        //  You must call the base class makeActive() if you override
        //

        virtual void makeActive();

        //
        //  Full path to file represented by document (Deprecated)
        //

        virtual std::string filePath() const;

        //
        //  Change the current filename. Note: This API supersedes the
        //  filePath() API.
        //

        virtual void setFileName(const std::string& newName);
        virtual const std::string& fileName() const;

        //
        //  Replaces contents with filename or if addContents==true then
        //  add the contents of the file instead of replacing. You should
        //  throw out of here if the read failed.
        //
        //  The tags can be used for any purpose. In RV, this is used to
        //  indicate the circumstances under which the file is being
        //  read. For example, choosing a single file of a sequence in the
        //  media browser means "exclusively that file".
        //

        virtual void read(const std::string& filename, const ReadRequest&);

        //
        //  Write contents to filename or if partial==true, then write
        //  only some of the data. (This is interpreted by the document
        //  sub-class.) Normally this would mean: write selection
        //  only. you should throw out of here if the write fails.
        //
        //  The tag can be used for any purpose.
        //

        virtual void write(const std::string& filename, const WriteRequest&);

        //
        //  Deletes the contents of the document.
        //

        virtual void clear();

        //
        //  Deltes all modes in the document
        //

        virtual void deleteModes();

        //
        //  Command History
        //  ---------------
        //

        virtual void doCommand(Command*);
        virtual void undoCommand();
        virtual void redoCommand();

        //
        //  Editing modes.
        //  --------------
        //
        //  There is one MajorMode active at a time. There may be many
        //  MinorModes active at the same time.
        //
        //  activateMode() will turn a mode on. If the mode is a major
        //  mode, it will deactivate the current MajorMode and then active
        //  the specified mode which will become the current MajorMode. If
        //  the mode is a MinorMode it will make it active. If the mode is
        //  already active, it will do nothing.
        //
        //  deactivateMode() will turn a mode off. If the mode is a major
        //  mode, it will do nothing (there should always be an active
        //  major mode). if the mode is minor it will deactivate it unless
        //  its already inactive in which case it will do nothing.
        //

        virtual void addMode(Mode*);

        const Modes& modes() const { return m_modes; }

        MajorMode* majorMode() const { return m_majorMode; }

        const MinorModes& minorModes() const { return m_minorModes; }

        Mode* findModeByName(const std::string& name) const;

        virtual void activateMode(Mode*);
        virtual void deactivateMode(Mode*);
        bool isModeActive(Mode*);

        void activateMode(const std::string& name);
        void deactivateMode(const std::string& name);
        bool isModeActive(const std::string& name);

        //
        //  Query event finds the first action corresponding to the
        //  event. If after is non-nil queryEvent() will look for the
        //  first action *after* the passed in action.
        //

        ActionTablePair queryEvent(const Event& event, const Action* after = 0);

        void executeAction(const Event&);

        void pushTable(const std::string&);
        void popTable();
        void popTable(const std::string&);

        void invalidateEventTables();

        const Event* currentEvent() const { return m_event; }

        static void debugEvents();

        const EventTableStack& eventTableStack() const;

        //
        //  EventNode API
        //

        virtual Result receiveEvent(const Event&);

        //
        //  The Document menu: the base class will construct a menu from
        //  the currently active modes and cache it. When modes change,
        //  the menu is invalidated and reconstructed upon request. You
        //  can modify the menu by overriding this function.
        //
        //  invalidateMenu() will send a menuChangedMessage() as well as
        //  deleting the current m_menu -- this will force the menu to be
        //  rebuilt by the menu() function. This function is *NOT* called
        //  by activateMenu() and deactivateMenu() in order to prevent a
        //  race condition with order that modeChangedMessage() and
        //  menuChangedMessage() are received from some objects.
        //

        virtual Menu* menu();
        virtual void invalidateMenu();

        //
        //  Get/Set selection
        //

        const SelectionState& selection() const { return m_selectionState; }

        virtual void setSelection(const SelectionState&);

        //
        //  Opaque pointer
        //

        void* opaquePointer() const { return m_opaquePointer; }

        void setOpaquePointer(void* p) { m_opaquePointer = p; }

    protected:
        void buildEventTables();

    private:
        void setFocusTable(const EventTable*);

    private:
        SelectionState m_selectionState;
        Menu* m_menu;
        MajorMode* m_majorMode;
        MinorModes m_minorModes;
        Modes m_modes;
        EventTableStack m_eventTableStack;
        EventTableNameStack m_eventTableNameStack;
        const EventTable* m_focusTable;
        bool m_eventTableDirty;
        const Event* m_event;
        CommandHistory m_commandHistory;
        void* m_opaquePointer;
        std::string m_fileName;
        DocumentSignal m_deleteSignal;
        DocumentSignal m_activeSignal;
        DocumentSignal m_historyChangedSignal;
        DocumentSignal m_fileNameChangedSignal;
        DocumentSignal m_modeChangedSignal;
        DocumentSignal m_menuChangedSignal;
        DocumentSignal m_selectionChangedSignal;
        DocumentSignal m_bindingsChangedSignal;
        DocumentSignal m_modeAddedSignal;
        static Documents m_documents;
        static Document* m_eventDocument;
    };

} // namespace TwkApp

#endif // __TwkApp__Document__h__
