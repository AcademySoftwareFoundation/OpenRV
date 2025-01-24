#ifndef __MuLang__MuLangParse__h__
#define __MuLang__MuLangParse__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <Mu/Module.h>
#include <Mu/MuProcess.h>
#include <Mu/Value.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/MuLangLanguage.h>
#include <vector>

namespace Mu
{
    class NodeAssembler;
    class Module;

    //
    //  Returns 0 on failure.
    //

    Process* Parse(const char* fileName, NodeAssembler*);

} // namespace Mu

#endif // __MuLang__MuLangParse__h__
