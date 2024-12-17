//
//  Copyright (c) 2013 Tweak Software
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __RvApp__CommandsModule__h__
#define __RvApp__CommandsModule__h__
#include <Mu/Node.h>
#include <MuLang/MuLangContext.h>
#include <Mu/Function.h>
#include <Mu/Vector.h>

namespace Rv
{

    void initCommands(Mu::MuLangContext* c = 0);
    Mu::Function* sessionFunction(const char*);
    void makeSessionDataFunction(Mu::Function*);

} // namespace Rv

#endif // __RvApp__CommandsModule__h__
