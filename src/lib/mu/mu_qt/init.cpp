//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Context.h>
#include <Mu/MuProcess.h>
#include <Mu/Module.h>
#include <Mu/Node.h>
#include <Mu/Function.h>
#include <MuLang/MuLangContext.h>
#include <MuQt5/qtModule.h>

#ifdef WIN32
#include <windows.h>
#endif

using namespace Mu;

extern "C"
{

    MU_EXPORT_DYNAMIC
    Mu::Module* MuInitialize(const char* name, Context* c, Process* p)
    {
        Module* m = new qtModule(c, name);
        Symbol* s = c->globalScope();
        s->addSymbol(m);
        return m;
    }

#ifdef WIN32

    BOOL DLLMain(HINSTANCE hi, DWORD reason, LPVOID dummy) { return TRUE; }

#endif
};
