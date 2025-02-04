//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __MuTwkApp__CommandsModule__h__
#define __MuTwkApp__CommandsModule__h__
#include <Mu/Module.h>
#include <Mu/Node.h>
#include <Mu/Function.h>

namespace TwkApp
{

    class CommandsModule : public Mu::Module
    {
    public:
        CommandsModule(Mu::Context* c, const char* name);
        virtual ~CommandsModule();

        virtual void load();

        static NODE_DECLARATION(eval, Mu::Pointer);

        static NODE_DECLARATION(undo, void);
        static NODE_DECLARATION(redo, void);
        static NODE_DECLARATION(clearHistory, void);
        static NODE_DECLARATION(undoHistory, Mu::Pointer);
        static NODE_DECLARATION(redoHistory, Mu::Pointer);
        static NODE_DECLARATION(beginCompoundCommand, void);
        static NODE_DECLARATION(endCompoundCommand, void);
        static NODE_DECLARATION(readFile, void);
        static NODE_DECLARATION(writeFile, void);
        static NODE_DECLARATION(defineMinorMode, void);
        static NODE_DECLARATION(activateMode, void);
        static NODE_DECLARATION(isModeActive, bool);
        static NODE_DECLARATION(deactivateMode, void);
        static NODE_DECLARATION(bindDoc, Mu::Pointer);
        static NODE_DECLARATION(bindings, Mu::Pointer);
        static NODE_DECLARATION(bind, void);
        static NODE_DECLARATION(bindRegex, void);
        static NODE_DECLARATION(unbind, void);
        static NODE_DECLARATION(unbindRegex, void);
        static NODE_DECLARATION(setTableBBox, void);
        static NODE_DECLARATION(pushEventTable, void);
        static NODE_DECLARATION(popEventTable, void);
        static NODE_DECLARATION(popNamedEventTable, void);
        static NODE_DECLARATION(activeEventTables, Mu::Pointer);

        static NODE_DECLARATION(defineModeMenu, void);
        // static NODE_DECLARATION(isComputationInProgress, bool);
        // static NODE_DECLARATION(computationMessage, Mu::Pointer);
        // static NODE_DECLARATION(computationProgress, float);
        // static NODE_DECLARATION(computationElapsedTime, float);
        static NODE_DECLARATION(contractSeq, Mu::Pointer);

        static NODE_DECLARATION(activeModes, Mu::Pointer);
        static NODE_DECLARATION(sequenceOfFile, Mu::Pointer);
    };

} // namespace TwkApp

#endif // __MuTwkApp__CommandsModule__h__
