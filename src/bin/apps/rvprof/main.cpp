//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include "../../utf8Main.h"

#include "VisMainWindow.h"
#include <iostream>

using namespace std;

const string usage = "\
\n\
usage: rvprof <profileoutput.rvprof>\n\
\n\
mouse:\n\
    left-click drag to pan camera\n\
    right-click drag or scroll-wheel to zoom in/out\n\
    control-left-click to set start point of refresh rate computation\n\
    alt-left-click to set end point of refresh rate computation\n\
    shift-left-click to bracket start/end point of refresh rate computation on play range\n\
\n\
fields:\n\
    input frame or time range to comput 'real' refresh rate\n\
    input real target fps to generate 'ideal' frame transitions\n\
\n\
";

int utf8Main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    VisMainWindow mainWindow;

    if (argc == 2)
    {
        if (0 == strcmp(argv[1], "-help"))
        {
            cerr << usage;
            exit(-1);
        }
        else
            mainWindow.readFile(argv[1]);
    }

    mainWindow.show();
    mainWindow.raise();
    return app.exec();
}
