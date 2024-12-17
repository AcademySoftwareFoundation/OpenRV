//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __RvNetworkAccessManager__h__
#define __RvNetworkAccessManager__h__

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QSslError>
#include <QtCore/QPair>

class RvNetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT

public:
    virtual ~RvNetworkAccessManager() {}

    RvNetworkAccessManager(QObject*);

    virtual QNetworkReply*
    createRequest(QNetworkAccessManager::Operation operation,
                  const QNetworkRequest& request, QIODevice* device);

public slots:
    void sslErrorsManagerSlot(QNetworkReply*, QList<QSslError>);
    void authenticationRequiredManagerSlot(QNetworkReply*, QAuthenticator*);
    void finishedManagerSlot(QNetworkReply*);
    void errorReplySlot(QNetworkReply::NetworkError);
    void sslErrorsReplySlot(QList<QSslError>);
};

#endif // __RvNetworkAccessManager__h__
