//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
/* Minimal main program -- everything is loaded from the library */

#include <Python.h>
#ifdef PLATFORM_DARWIN
#include <DarwinBundle/DarwinBundle.h>
#else
#include <QTBundle/QTBundle.h>
#endif
#include <QtCore/QtCore>
#include <QtGui/QtGui>
#include <iostream>

#include <QtWidgets/QtWidgets>

using namespace std;
using namespace TwkApp;

#ifdef PLATFORM_DARWIN
DarwinBundle bundle("python", BUNDLE_MAJOR_VERSION, BUNDLE_MINOR_VERSION, BUNDLE_REVISION_NUMBER);
#else
QTBundle bundle("python", BUNDLE_MAJOR_VERSION, BUNDLE_MINOR_VERSION, BUNDLE_REVISION_NUMBER);
#endif

int
main(int argc, char **argv)
{
    //Sleep(10 * 1000);

    QApplication app(argc, argv);
#ifndef PLATFORM_DARWIN
    bundle.initializeAfterQApplication(); // cause init() to be called
#endif

    Py_SetProgramName( Py_DecodeLocale( argv[0], nullptr ) );
    // Current issue: sys.path doesn't hold the site-packages path and thus py-interp cannot import modules.
    // This is the same as calling py-interp with -S since this disables site.py on init which is the symptom of the current issue.

    // Py_InitializeEx(1);
    // Note about Py_Initilialize:
    // See here for issue about calling Py_Initialize and Py_Main after: https://bugs.python.org/issue34008
    // See here for which functions are allowed to be called before Py_Initialize: https://docs.python.org/3/c-api/init.html#pre-init-safe
    // And we can infer, based on the previous 2 links, that it also safe for Py_Main (since it calls Py_Initialize itself)

    static wchar_t delim = L'\0';

    wchar_t** w_argv = new wchar_t*[argc + 1];
    w_argv[argc] = &delim;

    for(int i = 0; i < argc; ++i)
    {
      w_argv[i] = Py_DecodeLocale(argv[i], nullptr);
    }

    // PySys_SetArgvEx is deprecated in 3.11 (and using PySys_SetArgvEx without Py_Initialize crashes on make)
    // PySys_SetArgvEx( argc, w_argv, 0 );
    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    PyConfig_SetArgv(&config, argc, w_argv);
    // config.safe_path is new in 3.11
    // config.safe_path = 0;

    // Not calling Py_InitializeFromConfig(&config); since it's the same problem than Py_Initialize
    // Argv is still passed; you can repro (cause the same problem) the current issue by calling py-interp with -S i.e.: "./py-interp -S"

#ifdef PLATFORM_WINDOWS
    //
    // On windows: force -i
    //
    wchar_t* w_nargv[] = {Py_DecodeLocale(argv[0], nullptr), L"-i", L'\0'};

    if (argc == 1)
        return Py_Main(2, w_nargv);
    else
        return Py_Main(argc, w_argv);
#else

    return Py_Main(argc, w_argv);
#endif
}
