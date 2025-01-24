//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <RvCommon/StreamConnection.h>
#include <QtCore/QFile>
#include <QtCore/QDataStream>
#include <TwkQtCoreUtil/QtConvert.h>
#include <iostream>

#ifdef PLATFORM_LINUX
#include <stdint.h>
#endif

namespace Rv
{
    using namespace TwkQtCoreUtil;
    using namespace TwkQtChat;
    using namespace std;

    Connection* StreamConnection::connectionFactory(QObject* obj, bool pingpong)
    {
        return new StreamConnection(obj, pingpong);
    }

#define STREAM_CONNECTION_MAGIC_NUMBER 312967
#define STREAM_CONNECTION_VERSION 1

    StreamConnection::StreamConnection(QObject* parent, bool pingpong)
        : TwkQtChat::Connection(parent, pingpong)
        , m_streamFile(0)
        , m_streamDataStream(0)
        , m_streamState(NormalState)
        , m_name("spoofer")
        , m_app("rv")
        , m_contactName("spoofer@localhost")
        , m_timeScale(1.0)
        , m_verbose(false)
    {
        m_intervalTimer.setSingleShot(true);
    }

    StreamConnection::~StreamConnection()
    {
        delete m_streamDataStream;
        delete m_streamFile;
    }

    void StreamConnection::storeStream(const string& baseDir)
    {
        QString fileName(baseDir.c_str());

#ifdef PLATFORM_WINDOWS
        fileName.append(
            QString("/rv-network-stream-%1").arg((unsigned __int64)this));
#else
        fileName.append(QString("/rv-network-stream-%1").arg(uint64_t(this)));
#endif

        m_streamFile = new QFile(fileName);

        if (!m_streamFile->open(QIODevice::WriteOnly))
        {
            cerr << "ERROR: storeStream cannot open file '"
                 << UTF8::qconvert(fileName) << endl;
            return;
        }

        m_streamDataStream = new QDataStream(m_streamFile);

        *m_streamDataStream << int(STREAM_CONNECTION_MAGIC_NUMBER)
                            << int(STREAM_CONNECTION_VERSION);
        m_streamFile->flush();

        m_eventTimer.start();

        m_streamState = StoringState;
    }

    static int validateStreamConnection(QDataStream* stream,
                                        const string& fileName, bool verbose)
    {
        int magic;
        *stream >> magic;
        if (verbose)
            cerr << "stream magic: " << magic << endl;
        if (magic != STREAM_CONNECTION_MAGIC_NUMBER)
        {
            cerr << "ERROR: file '" << fileName
                 << "' is not an RV StreamConnection file." << endl;
            return 0;
        }

        int version;
        *stream >> version;
        if (verbose)
            cerr << "stream version: " << version << endl;
        if (version != STREAM_CONNECTION_VERSION)
        {
            cerr << "ERROR: file '" << fileName
                 << "': unknown RV StreamConnection version: " << version
                 << endl;
            return 0;
        }

        return version;
    }

    void StreamConnection::dumpStream(const string& fileName)
    {
        m_streamFile = new QFile(fileName.c_str());
        m_streamFile->open(QIODevice::ReadOnly);
        m_streamDataStream = new QDataStream(m_streamFile);

        int version =
            validateStreamConnection(m_streamDataStream, fileName, true);
        if (version == 0)
            return;

        int currentType;
        QString currentMessage;
        QString currentInterp;
        QByteArray currentData;

        while (!m_streamDataStream->atEnd())
        {
            *m_streamDataStream >> m_currentTime >> currentType;
            switch (currentType)
            {
            case MessageItemType:
                *m_streamDataStream >> currentMessage;
                cerr << "message " << m_currentTime << " '"
                     << UTF8::qconvert(currentMessage) << "'" << endl;
                break;

            case DataItemType:
                *m_streamDataStream >> m_currentTime >> currentData;
                cerr << "data " << m_currentTime << " '"
                     << UTF8::qconvert(currentInterp)
                     << "', bytes: " << currentData.size() << endl;
                break;
            }
        }
    }

    void StreamConnection::handleNextEvent()
    {
        int currentType;
        QString currentMessage;
        QString currentInterp;
        QByteArray currentData;

        *m_streamDataStream >> currentType;

        float diff;

        switch (currentType)
        {
        case MessageItemType:
            *m_streamDataStream >> currentMessage;
            if (m_verbose)
            {
                diff = 1000.0
                       * (m_timeScale * m_currentTime
                          - float(m_eventTimer.elapsed()));
                cerr << "message " << m_currentTime << " (diff " << diff
                     << ") '" << UTF8::qconvert(currentMessage) << "'" << endl;
            }
            emit newMessage(remoteContactName(), currentMessage);
            break;

        case DataItemType:
            *m_streamDataStream >> currentInterp >> currentData;
            if (m_verbose)
            {
                diff = 1000.0
                       * (m_timeScale * m_currentTime
                          - float(m_eventTimer.elapsed()));
                cerr << "data " << m_currentTime << " (diff " << diff << ") '"
                     << UTF8::qconvert(currentInterp)
                     << "', bytes: " << currentData.size() << endl;
            }
            emit newData(remoteContactName(), currentInterp, currentData);
            break;
        }

        if (m_streamDataStream->atEnd())
            return;

        *m_streamDataStream >> m_currentTime;

        if (m_timeScale == 0.0)
        {
            //
            //  In this special case, we stream the events "asap" while still
            //  allowing time for other events.
            //

            m_intervalTimer.start(0);
        }
        else
        {
            float diff =
                m_timeScale * m_currentTime - float(m_eventTimer.elapsed());

            //
            //  If we're "behind" handle the next event immediately, otherwise
            //  schedule it to be handled at the appropriate time in the future.
            //

            if (diff <= 0.0)
                handleNextEvent();
            else
            {
                int mSecs = int(1000.0 * diff);
                m_intervalTimer.start(mSecs);
            }
        }
    }

    bool StreamConnection::spoofStream(const string& fileName, bool verbose)
    {
        m_verbose = verbose;

        m_streamFile = new QFile(fileName.c_str());
        if (!m_streamFile->open(QIODevice::ReadOnly))
        {
            cerr << "ERROR: can't open network stream file '" << fileName << "'"
                 << endl;
            return false;
        }

        if (m_verbose)
            cerr << "stream file: " << fileName << endl;

        m_streamDataStream = new QDataStream(m_streamFile);

        int version =
            validateStreamConnection(m_streamDataStream, fileName, verbose);
        if (version == 0)
        {
            return false;
        }

        connect(&m_intervalTimer, SIGNAL(timeout()), this,
                SLOT(handleNextEvent()));

        m_streamState = SpoofingState;

        return true;
    }

    void StreamConnection::startSpoofing(float timeScale)
    {
        if (m_streamState == SpoofingState && !m_streamDataStream->atEnd())
        {
            m_timeScale = timeScale;
            //
            //  Ignore timing of first event.
            //
            float t;
            *m_streamDataStream >> t;
            m_currentTime = 0.0;

            m_eventTimer.start();

            handleNextEvent();
        }
    }

    bool StreamConnection::sendMessage(const QString& message)
    {
        if (m_streamState == StoringState)
        {
            *m_streamDataStream << float(m_eventTimer.elapsed())
                                << int(MessageItemType) << message;
            m_streamFile->flush();
        }
        else if (m_streamState == SpoofingState)
        {
            return true;
        }
        return Connection::sendMessage(message);
    }

    bool StreamConnection::sendData(const QString& interp,
                                    const QByteArray& data)
    {
        if (m_streamState == StoringState)
        {
            *m_streamDataStream << float(m_eventTimer.elapsed())
                                << int(DataItemType) << interp << data;
            m_streamFile->flush();
        }
        else if (m_streamState == SpoofingState)
        {
            return true;
        }
        return Connection::sendData(interp, data);
    }

    bool StreamConnection::isLocal()
    {
        if (m_streamState == SpoofingState)
            return true;
        else
            return TwkQtChat::Connection::isLocal();
    }

    const QString& StreamConnection::remoteName() const
    {
        if (m_streamState == SpoofingState)
            return m_name;
        else
            return TwkQtChat::Connection::remoteName();
    }

    const QString& StreamConnection::remoteApp() const
    {
        if (m_streamState == SpoofingState)
            return m_app;
        else
            return TwkQtChat::Connection::remoteApp();
    }

    const QString& StreamConnection::remoteContactName() const
    {
        if (m_streamState == SpoofingState)
            return m_contactName;
        else
            return TwkQtChat::Connection::remoteContactName();
    }

}; // namespace Rv
