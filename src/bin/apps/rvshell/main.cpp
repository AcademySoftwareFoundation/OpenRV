//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include "../../utf8Main.h"

#include <QtCore/QtCore>
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>
#include <QtNetwork/QHostInfo>
#include <iostream>

#include "ShellDialog.h"

using namespace std;

int utf8Main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    ShellDialog dialog;

    if (argc >= 3)
    {
        cout << endl;
        QHostInfo info = QHostInfo::fromName(argv[2]);
        int port = 45124;
        if (argc == 4)
            port = atoi(argv[3]);
        QHostAddress addr;

        for (int i = 0; i < info.addresses().size(); i++)
        {
            if (info.addresses()[i].protocol() == QAbstractSocket::IPv4Protocol)
            {
                addr = info.addresses()[i];
                cout << "found: "
                     << info.addresses()[i].toString().toUtf8().data() << endl;
                break;
            }
        }

        cout << "Connecting to " << argv[1] << " at "
             << addr.toString().toUtf8().data() << ":" << port << endl;

        dialog.client()->connectTo(argv[1], addr, port);
    }
    else
    {
        cerr << "usage: rvshell <name> <host> [port]" << endl;
        exit(1);
    }

    dialog.show();
    dialog.raise();
    return app.exec();
}
