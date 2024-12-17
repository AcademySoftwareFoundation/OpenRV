//
//  Copyright (c) 2008 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __RvCommon__StreamConnection__h__
#define __RvCommon__StreamConnection__h__

#include <TwkQtChat/Connection.h>
#include <TwkQtChat/Connection.h>
#include <TwkUtil/Timer.h>
#include <string>

class QFile;
class QDataStream;

namespace Rv
{

    class StreamConnection : public TwkQtChat::Connection
    {
        Q_OBJECT

    public:
        enum StreamItemType
        {
            MessageItemType,
            DataItemType
        };

        enum StreamState
        {
            NormalState,
            StoringState,
            SpoofingState
        };

        StreamConnection(QObject* parent, bool pingpong);
        ~StreamConnection();

        static TwkQtChat::Connection* connectionFactory(QObject*, bool);

        void storeStream(const std::string& baseDir);

        bool spoofStream(const std::string& fileName, bool verbose);
        void startSpoofing(float timeScale = 1.0);

        void dumpStream(const std::string& fileName);

        StreamState streamState() { return m_streamState; }

        //
        //   TwkQtChat::Connection API
        //

        virtual bool sendMessage(const QString& message);
        virtual bool sendData(const QString& interp, const QByteArray& data);

        virtual bool isLocal();
        virtual const QString& remoteName() const;
        virtual const QString& remoteApp() const;
        virtual const QString& remoteContactName() const;

    private slots:
        void handleNextEvent();

    private:
        StreamState m_streamState; //  Normal, Storing, or Spoofing
        QFile* m_streamFile;
        QDataStream* m_streamDataStream; //  Write tream for storage or Read
                                         //  stream for spoofing.
        TwkUtil::Timer m_eventTimer;     //  Absolute time.
        QTimer m_intervalTimer;          //  For spacing out spoofed events.

        QString m_name;
        QString m_app;
        QString m_contactName;

        float m_currentTime; //  Scheduled time of current event.
        float m_timeScale;   //  Smaller is faster
        bool m_verbose;
    };

}; // namespace Rv

#endif //__RvCommon__StreamConnection__h__
