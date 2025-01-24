//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <MuQt5/RvNetworkAccessManager.h>

#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QSslError>
#include <QtCore/QPair>

#include <iostream>

using namespace std;

static bool printErrors = false;

RvNetworkAccessManager::RvNetworkAccessManager(QObject* parent)
    : QNetworkAccessManager(parent)
{
    connect(this,
            SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)),
            this,
            SLOT(authenticationRequiredManagerSlot(QNetworkReply*,
                                                   QAuthenticator*)));
    connect(this, SIGNAL(sslErrors(QNetworkReply*, QList<QSslError>)), this,
            SLOT(sslErrorsManagerSlot(QNetworkReply*, QList<QSslError>)));

    printErrors = (0 != getenv("RV_REPORT_QNETWORK_ERRORS"));

    if (printErrors)
    {
        connect(this, SIGNAL(finished(QNetworkReply*)), this,
                SLOT(finishedManagerSlot(QNetworkReply*)));
    }
}

void RvNetworkAccessManager::sslErrorsManagerSlot(QNetworkReply* reply,
                                                  QList<QSslError> errors)
{
    reply->ignoreSslErrors();

    if (printErrors)
    {
        foreach (QSslError err, errors)
        {
            cerr << "WARNING: manager SSL error: "
                 << err.errorString().toStdString() << endl;
            cerr << "WARNING:     from URL '"
                 << reply->url().toString().toStdString() << "'" << endl;
        }
    }
}

void
RvNetworkAccessManager::authenticationRequiredManagerSlot(QNetworkReply* reply,
                                                          QAuthenticator* auth)
{
    if (printErrors)
        cerr << "WARNING: authentication required from URL '"
             << reply->url().toString().toStdString() << "'" << endl;
}

void RvNetworkAccessManager::finishedManagerSlot(QNetworkReply* reply)
{
    if (printErrors)
    {
        if (reply->error() != QNetworkReply::NoError)
        {
            cerr << "ERROR: manager http error '"
                 << reply->errorString().toStdString() << "'" << endl;
            cerr << "ERROR:     from URL '"
                 << reply->url().toString().toStdString() << "'" << endl;
        }
    }
}

void RvNetworkAccessManager::errorReplySlot(QNetworkReply::NetworkError error)
{
    cerr << "ERROR: NetworkAccessManager: http reply error #" << error << endl;
}

void RvNetworkAccessManager::sslErrorsReplySlot(QList<QSslError> errors)
{
    foreach (QSslError err, errors)
    {
        cerr << "WARNING: reply SSL error: " << err.errorString().toStdString()
             << endl;
    }
}

QNetworkReply* RvNetworkAccessManager::createRequest(
    QNetworkAccessManager::Operation operation, const QNetworkRequest& request,
    QIODevice* device)
{
    //
    //  Cast away const-ness so we can modify request
    //
    //  When we do this for real, we'll set various things in user-agent header,
    //  including:
    //
    //  protocol version
    //  RV product and version
    //  licensing status: licensed through shotgrid, or not
    //
    //  QNetworkRequest* r = (QNetworkRequest*) &request;
    //  r->setRawHeader("User-Agent", "blah");

    QNetworkReply* reply =
        QNetworkAccessManager::createRequest(operation, request, device);

    //
    //  We only get these signals to print errors, so don't even connect unless
    //  we're printing errors.
    //
    if (printErrors)
    {
        connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this,
                SLOT(errorReplySlot(QNetworkReply::NetworkError)));
        connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this,
                SLOT(sslErrorsReplySlot(QList<QSslError>)));
    }

    return reply;
}
