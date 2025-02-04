//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __RvCommon__RvWebManager__h__
#define __RvCommon__RvWebManager__h__
#include <iostream>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QSslError>
#include <QtCore/QPair>

namespace IPCore
{
    class Session;
}

namespace Rv
{

    class LocalNetworkAccessManager : public QNetworkAccessManager
    {
        Q_OBJECT

    public:
        LocalNetworkAccessManager(QObject* parent)
            : QNetworkAccessManager(parent) {};

        virtual QNetworkReply*
        createRequest(QNetworkAccessManager::Operation operation,
                      const QNetworkRequest& request, QIODevice* device);
    };

    class RvWebManager : public QObject
    {
        Q_OBJECT

    public:
        typedef QPair<QString, QString> StringPair;
        typedef QList<StringPair> HeaderList;

        struct ReplyData
        {
            ReplyData()
                : session(0)
                , callback(0)
            {
            }

            IPCore::Session* session;
            QUrl url;
            QString replyEvent;
            QString authenticationEvent;
            QString progressEvent;
            bool ignoreSslErrors;
            void (*callback)();
            QString tag;
        };

        typedef QMap<QNetworkReply*, ReplyData> ReplyMap;

        RvWebManager(QObject* parent = 0);
        virtual ~RvWebManager();

        void httpGet(const QString& url, const HeaderList& headers,
                     IPCore::Session* session, const QString& replyEvent,
                     const QString& authenticationEvent,
                     const QString& progressEvent, bool ignoreSslErrors = false,
                     bool urlIsEncoded = false);

        void httpPost(const QString& url, const HeaderList& headers,
                      const QByteArray& postData, IPCore::Session* session,
                      const QString& replyEvent,
                      const QString& authenticationEvent,
                      const QString& progressEvent,
                      bool ignoreSslErrors = false, bool urlIsEncoded = false,
                      void (*callback)() = 0, QString tag = "");

        void httpPut(const QString& url, const HeaderList& headers,
                     const QByteArray& putData, IPCore::Session* session,
                     const QString& replyEvent,
                     const QString& authenticationEvent,
                     const QString& progressEvent, bool ignoreSslErrors = false,
                     bool urlIsEncoded = false, void (*callback)() = 0,
                     QString tag = "");

        QNetworkAccessManager* netManager() const { return m_netManager; }

    public slots:
        void replyDone(QNetworkReply*);
        void replyRead();
        void replyError(QNetworkReply::NetworkError);
        void replySSLErrors(QList<QSslError>);
        void managerSSLErrors(QNetworkReply*, QList<QSslError>);
        void replyAuthentication(QNetworkReply*, QAuthenticator*);

    private:
        LocalNetworkAccessManager* m_netManager;
        ReplyMap m_replyMap;
    };

    class NetworkReplyTimeout : public QObject
    {
        Q_OBJECT

    public:
        NetworkReplyTimeout(QNetworkReply* reply, const int timeout);
        ~NetworkReplyTimeout();

    private slots:
        void timeout();

    private:
        QString m_tag;
    };

} // namespace Rv

#endif // __RvCommon__RvWebManager__h__
