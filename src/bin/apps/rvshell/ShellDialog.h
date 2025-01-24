//
//  Copyright (c) 2008 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __rvshell__ShellDialog__h__
#define __rvshell__ShellDialog__h__
#include <iostream>
#include "ui_ShellDialog.h"
#include <TwkQtChat/Client.h>

class ShellDialog
    : public QMainWindow
    , private Ui::ShellDialog
{
    Q_OBJECT

public:
    ShellDialog(QWidget* parent = 0);

    TwkQtChat::Client* client() const { return m_client; }

public slots:
    void appendMessage(const QString& from, const QString& message);

private slots:
    void go();
    void newContact(const QString&);
    void contactLeft(const QString&);
    void loadImage();

    void play();
    void stop();
    void forward();
    void back();
    void fullScreen(bool);
    void cacheOn(bool);
    void quit();

    void sendImage();

private:
    TwkQtChat::Client* m_client;
    QTextTableFormat m_tableFormat;
    QGraphicsScene* m_scene;
    QPixmap m_pixmap;
    QGraphicsPixmapItem* m_pixmapItem;
    QString m_contact;
    QString m_mediaName;
    unsigned char* m_imageData;
    bool m_stop;
};

#endif // __rvshell__ShellDialog__h__
