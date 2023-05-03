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
    Py_InitializeEx( 1 );

    static wchar_t delim = L'\0';

    wchar_t** w_argv = new wchar_t*[argc + 1];
    w_argv[argc] = &delim;

    for (int i = 0; i < argc; ++i ){
      w_argv[i] = Py_DecodeLocale(argv[i], nullptr);
    }

    PySys_SetArgvEx( argc, w_argv, 0 );

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
