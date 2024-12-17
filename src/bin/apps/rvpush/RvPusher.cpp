//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <RvPusher.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtNetwork/QHostAddress>
#include <iostream>
#include <algorithm>

#ifdef PLATFORM_WINDOWS
#include <synchapi.h> // For Sleep
#else
#include <unistd.h> // For usleep
#endif

#if 0
#define DB(x) cerr << dec << x << endl
#define DBL(level, x) cerr << dec << x << endl
#else
#define DB(x)
#define DBL(level, x)
#endif

using namespace std;

RvPusher::RvPusher(QCoreApplication& app, string tag, string cmd,
                   vector<string> argv, QObject* parent)
    : QObject(parent)
    , m_app(app)
    , m_client(0)
    , m_command(cmd)
    , m_tag(tag)
    , m_argv(argv)
    , m_error(0)
    , m_rvStarted(false)
{
    //
    //  Make network client.  If we can find a port number for a running rv,
    //  we'll connect to it later.
    //
    m_client = new TwkQtChat::Client("rvpush-1", "rvpush", 0, false);

    connect(m_client, SIGNAL(newContact(const QString&)), this,
            SLOT(newContact(const QString&)));

    connect(m_client, SIGNAL(connectionFailed(QAbstractSocket::SocketError)),
            this, SLOT(connectionFailed(QAbstractSocket::SocketError)));

    connect(m_client, SIGNAL(contactLeft(const QString&)), this,
            SLOT(contactLeft(const QString&)));

    connect(m_client, SIGNAL(newMessage(const QString&, const QString&)), this,
            SLOT(newMessage(const QString&, const QString&)));

    //
    //  Build list of possible tmp files holding rv port numbers.
    //
    buildFileList();

    //
    //  Try the first possible port file.
    //
    m_currentFile = 0;
    attemptConnection();
}

void RvPusher::buildFileList()
{
    DB("buildFileList");

    QDir tmp = QDir::temp();

    if (!tmp.exists("tweak_rv_proc"))
        return;

    tmp.cd("tweak_rv_proc");

    //
    //  Get files in this dir (not subdirs), sorted by modification time.
    //
    QString tagString(m_tag.c_str());

    QFileInfoList rawList = tmp.entryInfoList(QDir::Files, QDir::Time);

    for (int i = 0; i < rawList.size(); ++i)
    {
        QStringList parts = rawList[i].fileName().split("_");
        QString name = parts.mid(1).join("_");

        if ((parts.size() == 1 && tagString == "")
            || (parts.size() >= 2 && name == tagString))
        {
            m_fileList.push_back(rawList[i]);
        }
    }

    for (int i = 0; i < m_fileList.size(); ++i)
    {
        DB("    file " << i << " '"
                       << m_fileList[i].absoluteFilePath().toStdString()
                       << "'");
    }
}

void RvPusher::attemptConnection()
{
    DB("attemptConnection currentFile " << m_currentFile << " fileList:");
    for (int i = 0; i < m_fileList.size(); ++i)
    {
        DB("    file " << i << " '"
                       << m_fileList[i].absoluteFilePath().toStdString()
                       << "'");
    }

    //
    //  Attempt to read port number from currentFile, try other files if this
    //  fails.  When we have a candidate port, attempt a connection.  This is
    //  an async operation, so if it fails, contactLeft() will send us back here
    //  (with a new currentFile).
    //

    int port = 0;

    while (!port && m_currentFile < m_fileList.size())
    {
        port = 0;
        QFile portFile(m_fileList[m_currentFile].absoluteFilePath());

        if (portFile.open(QIODevice::ReadOnly))
        {
            //
            //  Read port number
            //
            char buf[256];

            int status = portFile.readLine(buf, 256);
            portFile.close();

            if (-1 != status)
            {
                QString str(buf);
                bool ok;
                port = str.toInt(&ok);
                if (!ok)
                    port = 0;

                DB("    '" << portFile.fileName().toStdString()
                           << "' gives port " << port);
            }
        }

        if (!port)
        {
            //
            //  This file gives no port (is unreadable or mal-formed), so
            //  remove it and try the next one.
            //
            DB("    removing " << portFile.fileName().toStdString());
            QFile::remove(portFile.fileName());
            ++m_currentFile;
        }
    }

    if (!port)
    {
        //
        //  We've run out of files with no successful connection, so start a new
        //  rv, or exit.
        //

        QString rvExe;

        if (getenv("RVPUSH_RV_EXECUTABLE_PATH"))
            rvExe = getenv("RVPUSH_RV_EXECUTABLE_PATH");
        else
        {
            //
            //  Assume that rv we'll run is in same dir as rvpush.
            //
            rvExe = QCoreApplication::applicationDirPath();

#if defined(PLATFORM_WINDOWS)
            rvExe += "/rv.exe";
#elif defined(PLATFORM_DARWIN)
            rvExe += "/RV";
#else
            rvExe += "/rv";
#endif
        }

        if (rvExe == "none" || !QFileInfo(rvExe).isExecutable())
        {
            cerr << "ERROR: cannot connect to any running RV" << endl;
            m_error = 11;
        }
        else
        {
            cerr << "INFO: cannot connect to any running RV, starting new one"
                 << endl;

            QStringList qargs;

            //
            //  Start a new rv with the appropriate tag.
            //
            if (m_tag != "")
            {
                qargs.push_back("-networkTag");
                qargs.push_back(m_tag.c_str());
            }

            //
            //  New RV must be listening for future rvpush connections.
            //
            qargs.append("-network");

            if (m_command == "mu-eval")
                qargs.append("-eval");
            else if (m_command == "mu-eval-return")
                qargs.append("-eval");
            else if (m_command == "py-eval")
                qargs.append("-pyeval");
            else if (m_command == "py-eval-return")
                qargs.append("-pyeval");
            else if (m_command == "py-exec")
                qargs.append("-pyeval");

            if (m_command == "url")
            {
                qargs.append(m_argv[0].c_str());
            }
            else
            {
                for (int i = 0; i < m_argv.size(); ++i)
                {
#ifdef PLATFORM_WINDOWS
                    if (m_command == "set" || m_command == "merge")
                        std::replace(m_argv[i].begin(), m_argv[i].end(), '\\',
                                     '/');
#endif
                    qargs.append(m_argv[i].c_str());
                }
            }

            DB("    starting '" << rvExe.toStdString() << "'");
            for (int i = 0; i < qargs.size(); ++i)
                DB("        " << qargs[i].toStdString());

            QProcess::startDetached(rvExe, qargs);

            DB("    done");

            m_rvStarted = true;
            m_error = 15;
        }

        m_app.quit();
    }
    else
    {
        //
        //  We have a candidate port number, so try it.  Success will send us
        //  to newContact(), failure to contactLeft().
        //
        QHostAddress addr("127.0.0.1");
        m_client->connectTo("rv", addr, port);
    }
}

void RvPusher::newContact(const QString& contact)
{
    //
    //  We've successfully connected, so send our one eval event, and exit
    //  unless we're waiting for a return value.
    //

    m_contact = contact;
    DB("connected to " << contact.toStdString());

    bool rsvp =
        (m_command == "mu-eval-return" || m_command == "py-eval-return");

    if (m_command == "mu-eval" || m_command == "mu-eval-return"
        || m_command == "py-eval" || m_command == "py-eval-return"
        || m_command == "py-exec")
    {
        QString src = "";
        for (int i = 0; i < m_argv.size(); ++i)
        {
            src += m_argv[i].c_str();
            src += " ";
        }
        DB("    " << m_command << " '" << src.toStdString() << "'");

        QString com = "invalid";

        if (m_command == "mu-eval" || m_command == "mu-eval-return")
            com = "remote-eval";
        else if (m_command == "py-eval" || m_command == "py-eval-return")
            com = "remote-pyeval";
        else
            com = "remote-pyexec";

        bool rsvp =
            (m_command == "mu-eval-return" || m_command == "py-eval-return");

        m_client->sendEvent(m_contact, com, "*", src, rsvp);
    }
    else if (m_command == "set" || m_command == "merge")
    {
        //
        //  If "set" clear the session before we add media, otherwise merge it
        //  in.
        //
        QString mu = (m_command == "set")
                         ? "{ require rvui; rvui.clearEverything(); "
                         : "{ ";

        //
        //  Run through args, use '+' instead of '-' for per-source args.
        //

        mu += "addSources(string[] {";
        bool inSource = false;

        for (int i = 0; i < m_argv.size(); ++i)
        {
            if (m_argv[i] == "[")
                inSource = true;
            else if (m_argv[i] == "]")
                inSource = false;
            else if (inSource && m_argv[i][0] == '-' && isalpha(m_argv[i][1]))
                m_argv[i][0] = '+';

            // convert relative to absolute file path
            QFileInfo curFileInfo(QString::fromStdString(m_argv[i]));
            // Not all arguements are file path. We change only if the arguement
            // is an existing file.
            if (curFileInfo.exists() && curFileInfo.isRelative())
            {
                m_argv[i] = curFileInfo.absoluteFilePath().toStdString();
            }

#ifdef PLATFORM_WINDOWS
            std::replace(m_argv[i].begin(), m_argv[i].end(), '\\', '/');
#endif

            if (i)
                mu += ", \"";
            else
                mu += "\"";

            mu += m_argv[i].c_str();

            mu += "\"";
        }
        mu += "}, \"rvpush\", false, ";
        mu += ((m_command == "merge") ? "true);" : "false);");
        mu += " mainWindowWidget().raise(); }";

        DB("    " << m_command << " '" << mu.toStdString() << "'");

        m_client->sendEvent(m_contact, "remote-eval", "*", mu, false);
    }
    else if (m_command == "url")
    {
        QString mu = "{ mainWindowWidget().raise(); ";

        mu += "sessionFromUrl(\"";
        mu += m_argv[0].c_str();
        mu += "\"); }";

        DB("    " << m_command << " '" << mu.toStdString() << "'");

        m_client->sendEvent(m_contact, "remote-eval", "*", mu, false);
    }

    if (!rsvp)
        disconnect();
}

void RvPusher::disconnect()
{
    m_client->signOff(m_contact);
//
//  Allow time for the communication to finish before we bail.
//
#ifdef PLATFORM_WINDOWS
    Sleep(250);
#else
    usleep(250000);
#endif
    m_app.quit();
}

void RvPusher::connectionFailed(QAbstractSocket::SocketError error)
{
    //
    //  Not sure under what circumstances we'd end up here, but if we do we
    //  should quit
    //
    DB("error");
    m_error = 4;
    m_app.quit();
}

void RvPusher::contactLeft(const QString& contact)
{
    DB("contactLeft " << contact.toStdString());

    //
    //  Contact failed so remove this port file
    //
    DB("    removing "
       << m_fileList[m_currentFile].absoluteFilePath().toStdString());
    QFile::remove(m_fileList[m_currentFile].absoluteFilePath());

    //
    //  Increment file counter and try again
    //
    ++m_currentFile;
    attemptConnection();
}

void RvPusher::newMessage(const QString& sender, const QString& inmessage)
{
    DB("newMessage '" << inmessage.toStdString() << "'");

    if (!inmessage.startsWith("RETURN "))
        return;

    QString message = inmessage;
    message.remove(0, 7);

    DB("returned '" << message.toStdString() << "'");

    cout << message.toStdString() << endl;

    disconnect();
}
