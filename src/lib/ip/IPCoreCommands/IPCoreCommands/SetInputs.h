//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCoreCommands__SetInputs__h__
#define __IPCoreCommands__SetInputs__h__
#include <TwkApp/Command.h>
#include <IPCore/IPGraph.h>
#include <iostream>

namespace IPCore
{
    namespace Commands
    {

        //
        //  SetInputs
        //
        //  Old inputs are stored and new inputs are assigned. This operation
        //  is a toggle so undo() and redo() just call doit().
        //

        class SetInputsInfo : public TwkApp::CommandInfo
        {
        public:
            typedef TwkApp::Command Command;

            SetInputsInfo(const std::string& name,
                          TwkApp::CommandInfo::UndoType type);
            virtual ~SetInputsInfo();
            virtual Command* newCommand() const;
        };

        class SetInputs : public TwkApp::Command
        {
        public:
            typedef std::vector<std::string> StringVector;

            SetInputs(const SetInputsInfo*);
            virtual ~SetInputs();

            void setArgs(IPGraph* graph, const std::string& name,
                         const StringVector& inputs);

            virtual void doit();
            virtual void undo();

        private:
            IPGraph* m_graph;
            std::string m_name;
            StringVector m_inputs;
        };

    } // namespace Commands
} // namespace IPCore

#endif // __IPCoreCommands__SetInputs__h__
