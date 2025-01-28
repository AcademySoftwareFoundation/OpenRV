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
#include <MuImage/ImageModule.h>

using namespace Mu;

extern "C"
{

    Mu::Module* MuInitialize(const char* name, Context* c, Process* p)
    {
        Module* m = new ImageModule(c, name);
        Symbol* s = c->globalScope();
        s->addSymbol(m);
        return m;
    }
};
