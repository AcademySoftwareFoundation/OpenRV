//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <RvCommon/RvWebManager.h>
#include <IPCore/Session.h>
#include <QtCore/QUrl>
#include <QtCore/QDir>
#include <QtCore/QTextCodec>
#include <QtCore/QStandardPaths>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkDiskCache>
#include <QtCore/QtCore>
#include <TwkQtCoreUtil/QtConvert.h>
#include <TwkDeploy/Deploy.h>
#include <TwkUtil/User.h>

namespace Rv
{
    using namespace std;
    using namespace IPCore;
    using namespace TwkQtCoreUtil;

    static QByteArray rvUserAgentHeader()
    {
        static QByteArray userAgentString;

        if (userAgentString.isEmpty())
        {
            ostringstream str;
            int minVersion = TWK_DEPLOY_MINOR_VERSION();
            int relVersion = 2 * (minVersion / 2);

            str << "RV/3." << relVersion
                << " (rv:" << TWK_DEPLOY_MAJOR_VERSION() << "."
                << TWK_DEPLOY_MINOR_VERSION() << "." << TWK_DEPLOY_PATCH_LEVEL()
                << ") Autodesk: www.autodesk.com";
            userAgentString = QByteArray(str.str().c_str());
        }

        return userAgentString;
    }

    QNetworkReply* LocalNetworkAccessManager::createRequest(
        QNetworkAccessManager::Operation operation,
        const QNetworkRequest& request, QIODevice* device)
    {
        //  cerr << "createRequest url '" <<
        //  request.url().toString().toStdString() << "'" << endl;

        QNetworkRequest* r = (QNetworkRequest*)&request;

        r->setRawHeader("User-Agent", rvUserAgentHeader());

        return QNetworkAccessManager::createRequest(operation, request, device);
    }

    static bool reportErrors = false;

    NetworkReplyTimeout::NetworkReplyTimeout(QNetworkReply* reply,
                                             const int timeout)
        : QObject(reply)
    {
        if (reply)
        {
            QTimer::singleShot(timeout, this, SLOT(timeout()));
        }
    }

    NetworkReplyTimeout::~NetworkReplyTimeout()
    {
        //  cerr << "***********************************
        //  NetworkReplyTimeoutDeleted, reply " <<
        //  static_cast<QNetworkReply*>(parent()) << endl;
    }

    void NetworkReplyTimeout::timeout()
    {
        bool authDebug = (getenv("RV_SHOTGUN_AUTH_DEBUG") != 0);
        if (authDebug)
            cerr << "INFO: network reply timeout" << endl;

        QNetworkReply* reply = static_cast<QNetworkReply*>(parent());
        if (reply->isRunning())
            reply->close();
    }

    RvWebManager::RvWebManager(QObject* parent)
        : QObject(parent)
    {
        reportErrors = (0 != getenv("RV_REPORT_QNETWORK_ERRORS"));

        m_netManager = new LocalNetworkAccessManager(this);

        QStringList cacheLocations =
            QStandardPaths::standardLocations(QStandardPaths::CacheLocation);
        QString cacheLocation = cacheLocations.front();

        //
        //  XXX the below is lame.  It looks like something goes wrong
        //  btween the cache and the webview, so that https pages cached
        //  by a previous RV process are not rendered.  So we clear the
        //  https cache here.  Should ask trolltech about it.
        //
        QDir cachePath(cacheLocation);
        QDir httpsDir(cachePath.absoluteFilePath("https"));
        QStringList entries = httpsDir.entryList();
        for (int i = 0; i < entries.size(); ++i)
            httpsDir.remove(entries[i]);

        //
        //  Add a disk cache
        //
        QNetworkDiskCache* diskCache = new QNetworkDiskCache(this);
        diskCache->setCacheDirectory(cacheLocation);
        m_netManager->setCache(diskCache);

        // QWebSettings::setMaximumPagesInCache(10);

        connect(m_netManager,
                SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)),
                this,
                SLOT(replyAuthentication(QNetworkReply*, QAuthenticator*)));
        connect(m_netManager,
                SIGNAL(sslErrors(QNetworkReply*, QList<QSslError>)), this,
                SLOT(managerSSLErrors(QNetworkReply*, QList<QSslError>)));
    }

    RvWebManager::~RvWebManager() {}

    void RvWebManager::httpGet(const QString& url, const HeaderList& headers,
                               Session* session, const QString& replyEvent,
                               const QString& authenticationEvent,
                               const QString& progressEvent,
                               bool ignoreSslErrors, bool urlIsEncoded)
    {
        QUrl u = urlIsEncoded ? QUrl::fromEncoded(url.toUtf8()) : QUrl(url);
        QNetworkRequest request(u);

        for (size_t i = 0; i < headers.size(); i++)
        {
            const StringPair& p = headers[i];
            QByteArray header = p.first.toUtf8();
            QByteArray value = p.second.toUtf8();
            request.setRawHeader(header, value);
        }

        //  cerr << "INFO: RvWebManager GET from " << url.toStdString() << endl;

        QNetworkReply* reply = m_netManager->get(request);

        reply->setReadBufferSize(0); // unlimited

        connect(reply, SIGNAL(readyRead()), this, SLOT(replyRead()));
        connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this,
                SLOT(replyError(QNetworkReply::NetworkError)));
        connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this,
                SLOT(replySSLErrors(QList<QSslError>)));

        connect(m_netManager, SIGNAL(finished(QNetworkReply*)), this,
                SLOT(replyDone(QNetworkReply*)));

        ReplyData rdata;
        rdata.url = u;
        rdata.session = session;
        rdata.replyEvent = replyEvent;
        rdata.authenticationEvent = authenticationEvent;
        rdata.progressEvent = progressEvent;
        rdata.ignoreSslErrors = ignoreSslErrors;

        m_replyMap.insert(reply, rdata);
    }

    void RvWebManager::httpPost(const QString& url, const HeaderList& headers,
                                const QByteArray& postData, Session* session,
                                const QString& replyEvent,
                                const QString& authenticationEvent,
                                const QString& progressEvent,
                                bool ignoreSslErrors, bool urlIsEncoded,
                                void (*callback)(), QString tag)
    {
        QUrl u = urlIsEncoded ? QUrl::fromEncoded(url.toUtf8()) : QUrl(url);
        QNetworkRequest request(u);
        request.setRawHeader("User-Agent", rvUserAgentHeader());

        for (size_t i = 0; i < headers.size(); i++)
        {
            const StringPair& p = headers[i];
            QByteArray header = p.first.toUtf8();
            QByteArray value = p.second.toUtf8();
            request.setRawHeader(header, value);
        }

        bool authDebug = (getenv("RV_SHOTGUN_AUTH_DEBUG") != 0);
        if (authDebug)
            cerr << "INFO: RvWebManager POST to " << UTF8::qconvert(url)
                 << endl;
        /*
        //
        //  Uncommenting this will reveal passwords, so not for release!
        //
        if (authDebug)
        {
            cerr << "POST DATA ----------- START" << endl;
            QString postString(postData);
            cerr << UTF8::qconvert(postString) << endl;
            cerr << "POST DATA ----------- END" << endl;
        }
        */

        QNetworkReply* reply = m_netManager->post(request, postData);
        if (callback)
        {
            int timeout = 20000;
            if (const char* to = getenv("RV_SHOTGUN_AUTH_TIMEOUT"))
            {
                timeout = atoi(to);
            }
            new NetworkReplyTimeout(reply, timeout);
        }

        reply->setReadBufferSize(0); // unlimited

        connect(reply, SIGNAL(readyRead()), this, SLOT(replyRead()));
        connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this,
                SLOT(replyError(QNetworkReply::NetworkError)));
        connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this,
                SLOT(replySSLErrors(QList<QSslError>)));

        connect(m_netManager, SIGNAL(finished(QNetworkReply*)), this,
                SLOT(replyDone(QNetworkReply*)));

        ReplyData rdata;
        rdata.url = u;
        rdata.session = session;
        rdata.replyEvent = replyEvent;
        rdata.authenticationEvent = authenticationEvent;
        rdata.progressEvent = progressEvent;
        rdata.ignoreSslErrors = ignoreSslErrors;
        rdata.callback = callback;
        rdata.tag = tag;

        m_replyMap.insert(reply, rdata);
    }

    void RvWebManager::httpPut(const QString& url, const HeaderList& headers,
                               const QByteArray& putData, Session* session,
                               const QString& replyEvent,
                               const QString& authenticationEvent,
                               const QString& progressEvent,
                               bool ignoreSslErrors, bool urlIsEncoded,
                               void (*callback)(), QString tag)
    {
        QUrl u = urlIsEncoded ? QUrl::fromEncoded(url.toUtf8()) : QUrl(url);
        QNetworkRequest request(u);
        request.setRawHeader("User-Agent", rvUserAgentHeader());

        for (size_t i = 0; i < headers.size(); i++)
        {
            const StringPair& p = headers[i];
            QByteArray header = p.first.toUtf8();
            QByteArray value = p.second.toUtf8();
            request.setRawHeader(header, value);
        }

        bool authDebug = (getenv("RV_SHOTGUN_AUTH_DEBUG") != 0);
        if (authDebug)
            cerr << "INFO: RvWebManager PUT to " << UTF8::qconvert(url) << endl;
        /*
        //
        //  Uncommenting this will reveal passwords, so not for release!
        //
        if (authDebug)
        {
            cerr << "PUT URL  ----------- START" << endl;
            cerr << u.toString().toStdString() << endl;
            cerr << "PUT URL  ----------- END" << endl;
            cerr << "PUT DATA ----------- START" << endl;
            QString putString(putData);
            cerr << UTF8::qconvert(putString) << endl;
            cerr << "PUT DATA ----------- END" << endl;
        }
        */

        QNetworkReply* reply = m_netManager->put(request, putData);
        if (callback)
        {
            int timeout = 20000;
            if (const char* to = getenv("RV_SHOTGUN_AUTH_TIMEOUT"))
            {
                timeout = atoi(to);
            }
            new NetworkReplyTimeout(reply, timeout);
        }

        reply->setReadBufferSize(0); // unlimited

        connect(reply, SIGNAL(readyRead()), this, SLOT(replyRead()));
        connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this,
                SLOT(replyError(QNetworkReply::NetworkError)));
        connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this,
                SLOT(replySSLErrors(QList<QSslError>)));

        connect(m_netManager, SIGNAL(finished(QNetworkReply*)), this,
                SLOT(replyDone(QNetworkReply*)));

        ReplyData rdata;
        rdata.url = u;
        rdata.session = session;
        rdata.replyEvent = replyEvent;
        rdata.authenticationEvent = authenticationEvent;
        rdata.progressEvent = progressEvent;
        rdata.ignoreSslErrors = ignoreSslErrors;
        rdata.callback = callback;
        rdata.tag = tag;

        m_replyMap.insert(reply, rdata);
    }

    void RvWebManager::replyDone(QNetworkReply* reply)
    {
        if (m_replyMap.count(reply) != 1)
            return;

        ReplyData d = m_replyMap[reply];
        QString replyErrorString;

        bool authDebug = (getenv("RV_SHOTGUN_AUTH_DEBUG") != 0);

        if (reply->error() != QNetworkReply::NoError)
        {
            if (reportErrors)
            {
                replyErrorString = reply->errorString();
                cerr << "ERROR: RvWebManager http reply: "
                     << UTF8::qconvert(replyErrorString) << endl;
            }
        }
        /*
        else
        {
            cerr << "INFO: RvWebManager reply from " <<
        reply->request().url().toString().toStdString() << endl;
        }
        */

        QByteArray array = reply->readAll();
        QString ctype =
            reply->header(QNetworkRequest::ContentTypeHeader).toString();

        if (authDebug)
            cerr << "INFO: reply " << reply << ", replyDone (" << array.size()
                 << " byes): '" << array.constData() << "'" << endl;

        if (d.session)
        {
            if (ctype.contains("text/", Qt::CaseInsensitive)
                || ctype.contains("application/json", Qt::CaseInsensitive)
                || !replyErrorString.isEmpty())
            {
                //
                //  Text, convert to UTF-8 for Mu
                //

                QTextCodec* codec = QTextCodec::codecForHtml(array);
                QString u = codec->toUnicode(array);
                QByteArray utf8 = u.toUtf8();
                if (!replyErrorString.isEmpty())
                    utf8 = replyErrorString.toUtf8();

                (void)d.session->userRawDataEvent(
                    d.replyEvent.toUtf8().constData(),
                    ctype.toUtf8().constData(), array.constData(), array.size(),
                    utf8.constData());
            }
            else
            {
                //
                //  Binary data, no text rep
                //

                (void)d.session->userRawDataEvent(
                    d.replyEvent.toUtf8().constData(),
                    ctype.toUtf8().constData(), array.constData(), array.size(),
                    0);
            }
        }

        m_replyMap.remove(reply);
        reply->deleteLater();
    }

    void RvWebManager::replyRead()
    {
        //  cerr << "INFO: RvWebManager: readReady signal" << endl;
    }

    void RvWebManager::replyError(QNetworkReply::NetworkError error)
    {
        if (reportErrors)
            cerr << "WARNING: reply error #" << error << endl;
    }

    void RvWebManager::replySSLErrors(QList<QSslError> errList)
    {
        if (reportErrors)
        {
            foreach (QSslError err, errList)
            {
                cerr << "WARNING: reply SSL error: "
                     << UTF8::qconvert(err.errorString()) << endl;
            }
        }
    }

    void RvWebManager::managerSSLErrors(QNetworkReply* reply,
                                        QList<QSslError> errList)
    {
        bool ignore = true;

        //
        //  Ignore ssl errors either if the user asked us to, or if the request
        //  was internally generated, so the user had no chance to specify the
        //  behavior.
        //

        if (m_replyMap.contains(reply))
        {
            ignore = m_replyMap[reply].ignoreSslErrors;
        }

        if (ignore)
            reply->ignoreSslErrors();

        if (reportErrors)
        {
            foreach (QSslError err, errList)
            {
                cerr << "WARNING: " << ((ignore) ? "ignoring" : "not ignoring")
                     << " SSL error: " << UTF8::qconvert(err.errorString())
                     << endl;
            }
        }
    }

    void RvWebManager::replyAuthentication(QNetworkReply*, QAuthenticator*)
    {
        //  cerr << "replyAuthentication" << endl;
    }

} // namespace Rv
