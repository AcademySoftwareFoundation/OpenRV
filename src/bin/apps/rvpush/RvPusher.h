//
//  Copyright (c) 2008 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __rvpush__RvPusher__h__
#define __rvpush__RvPusher__h__

#if defined(_WIN32)
#include <windows.h>
#endif
#include <iostream>
#include <TwkQtChat/Client.h>
#include <QtCore/QFileInfo>

using namespace std;

class QCoreApplication;

//
//  RvPusher reads port number for the most recently started RV out of a tmp
//  file and trys to communicate with it.  Either setting or adding to the
//  media, or running arbitrary mu code, according to the "command".  If it
//  can't find an RV it can talk to, it'll start a new one.

class RvPusher : public QObject
{
    Q_OBJECT

public:
    RvPusher(QCoreApplication&, string, string, vector<string>,
             QObject* parent = 0);

    //
    //  Errors can be encountered during construction, or later due to async
    //  network communication.
    //
    int error() { return m_error; };

    //
    //  Did we start a new RV process ?
    //
    bool rvStarted() { return m_rvStarted; };

private slots:
    void newContact(const QString&);
    void connectionFailed(QAbstractSocket::SocketError);
    void contactLeft(const QString&);
    void newMessage(const QString&, const QString&);

private:
    void buildFileList();
    void attemptConnection();
    void disconnect();

    QCoreApplication& m_app;
    TwkQtChat::Client* m_client;
    QString m_contact;
    string m_command;
    string m_tag;
    vector<string> m_argv;
    int m_error;
    int m_currentFile;
    QFileInfoList m_fileList;
    bool m_rvStarted;
};

#endif // __rvshell__ShellDialog__h__
