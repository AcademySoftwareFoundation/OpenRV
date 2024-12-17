//******************************************************************************
// Copyright (c) 2011 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __PyTwkApp__PyInterface__h__
#define __PyTwkApp__PyInterface__h__
#include <string>

//
//  NOTE: be careful not to input Python.h in this file. Its possible that
//  another project will accidently include installed Python.h of some
//  version we don't support.
//

namespace TwkApp
{
    class Menu;
    class Document;

    TwkApp::Menu* pyListToMenu(const char* name, void* pyobj);
    void* callPythonFunction(const char* name, const char* module = 0);
    void disposeOfPythonObject(void*);
    void evalPython(const char* text);
    void initPython(int argc = 0, char** argv = NULL);
    void finalizePython();
    void pyInitWithFile(const char* rcfile, void* commands0, void* commands1);

} // namespace TwkApp

#endif // __PyTwkApp__PyInterface__h__
