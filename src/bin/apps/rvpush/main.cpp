//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include "../../utf8Main.h"

#include <QtCore/QCoreApplication>
#include <iostream>
#include <RvPusher.h>

using namespace std;

const char* usage = "\n\
usage: rvpush [-tag <tag>] <command> <commandArgs>\n\
       rvpush set <mediaArgs>         \n\
       rvpush merge <mediaArgs>       \n\
       rvpush mu-eval <mu>            \n\
       rvpush mu-eval-return <mu>     \n\
       rvpush py-eval <python>        \n\
       rvpush py-eval-return <python> \n\
       rvpush py-exec <python>        \n\
       rvpush url rvlink://<rv-command-line>\n\
       \n\
       Examples:\n\
       \n\
       To set the media contents of the currently running RV:\n\
           rvpush set [ foo.mov -in 101 -out 120 ] \n\
       \n\
       To add to the media contents of the currently running RV with tag \"myrv\":\n\
           rvpush -tag myrv merge [ fooLeft.mov fooRight.mov ] \n\
       \n\
       To execute arbitrary Mu code in the currently running RV:\n\
           rvpush mu-eval 'play()'\n\
       \n\
       To execute arbitrary Mu code in the currently running RV, and print the result:\n\
           rvpush mu-eval-return 'frame()'\n\
       \n\
       To evaluate an arbitrary Python expression in the currently running RV:\n\
           rvpush py-eval 'rv.commands.play()'\n\
       \n\
       To evaluate an arbitrary Python expression in the currently running RV, and print the result:\n\
           rvpush py-eval-return 'rv.commands.frame()'\n\
       \n\
       To execute arbitrary Python statements in the currently running RV:\n\
           rvpush py-exec 'from rv import commands; commands.play()'\n\
       \n\
       To process an rvlink url in the currently running RV, loading a movie into the current session:\n\
           rvpush url 'rvlink:// -reuse 1 foo.mov'\n\
       \n\
       To process an rvlink url in the currently running RV, loading a movie into a new session:\n\
           rvpush url 'rvlink:// -reuse 0 foo.mov'\n\
       \n\
       Set environment variable RVPUSH_RV_EXECUTABLE_PATH if you want rvpush to \n\
       start something other than the default RV when it cannot find a running \n\
       RV.  Set to 'none' if you want no RV to be started. \n\
       \n\
       Exit status: \n\
            4: Connection to running RV failed \n\
           11: Could not connect to running RV, and could not start new RV \n\
           15: Cound not connect to running RV, started new one. \n\
";

#if 0
#define DB(x) cerr << dec << x << endl
#define DBL(level, x) cerr << dec << x << endl
#else
#define DB(x)
#define DBL(level, x)
#endif

int utf8Main(int argc, char* argv[])
{
    if (argc < 2 || strcmp(argv[1], "-help") == 0)
    {
        cerr << usage << endl;
        exit(1);
    }

    int commandPos = 1;
    string tag;
    DB("argv1 '" << argv[1] << "'");
    if (string(argv[1]) == "-tag")
    {
        if (argc < 3)
        {
            cerr << "ERROR: missing tag" << endl;
            cerr << usage << endl;
            exit(2);
        }
        tag = argv[2];
        commandPos = 3;
    }
    string command = argv[commandPos];
    DB("commandPos " << commandPos << " command " << command);

    if (command != "set" && command != "merge" && command != "mu-eval"
        && command != "mu-eval-return" && command != "py-eval"
        && command != "py-eval-return" && command != "py-exec"
        && command != "url")
    {
        cerr << usage << endl;
        exit(3);
    }

    vector<string> commandArgv;
    if (command == "url")
    {
        string url;

        for (int i = commandPos + 1; i < argc; ++i)
        {
            if (i > commandPos + 1)
                url += " ";
            url += argv[i];
        }
        commandArgv.push_back(url);
    }
    else
        for (int i = commandPos + 1; i < argc; ++i)
            commandArgv.push_back(argv[i]);
    DB("command " << command << " tag " << tag);

    QCoreApplication app(argc, argv);

    RvPusher pusher(app, tag, command, commandArgv);

    //
    //  Start the app event loop only if we neither got an error constructing
    //  the RvPusher nor started a new RV (IE we're going to communicate with a
    //  running RV over the network).
    //

    if (pusher.error() == 0 && !pusher.rvStarted())
        app.exec();

    DB("exiting, return " << pusher.error());

    exit(pusher.error());
}
