//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>
#include "ShellDialog.h"
#include <stdlib.h>
#include <iostream>

using namespace TwkQtChat;
using namespace std;

static bool updateOnly = false;

ShellDialog::ShellDialog(QWidget* parent)
    : QMainWindow(parent)
{
    if (getenv("RVSHELL_PIXEL_UPDATE_ONLY"))
        updateOnly = true;

    setupUi(this);
    //
    //  Last arg below turns off the ping-pong "heartbeat" from rv
    //
    m_client = new Client("rv-shell-1", "rv-shell", 0, false);
    m_imageData = 0;
    m_stop = false;

    plainTextEdit->setFocusPolicy(Qt::StrongFocus);
    textEdit->setFocusPolicy(Qt::NoFocus);
    textEdit->setReadOnly(true);
    listWidget->setFocusPolicy(Qt::NoFocus);

    m_scene = new QGraphicsScene(this);
    m_pixmapItem = 0;
    graphicsView->setScene(m_scene);

    connect(actionSend, SIGNAL(triggered()), this, SLOT(go()));
    connect(actionQuit, SIGNAL(triggered()), this, SLOT(quit()));

    connect(m_client, SIGNAL(newMessage(const QString&, const QString&)), this,
            SLOT(appendMessage(const QString&, const QString&)));
    connect(m_client, SIGNAL(newContact(const QString&)), this,
            SLOT(newContact(const QString&)));
    connect(m_client, SIGNAL(contactLeft(const QString&)), this,
            SLOT(contactLeft(const QString&)));

    connect(sendImageButton, SIGNAL(clicked()), this, SLOT(sendImage()));

    connect(playButton, SIGNAL(clicked()), this, SLOT(play()));
    connect(stopButton, SIGNAL(clicked()), this, SLOT(stop()));
    connect(forwardButton, SIGNAL(clicked()), this, SLOT(forward()));
    connect(backButton, SIGNAL(clicked()), this, SLOT(back()));
    connect(fullScreenButton, SIGNAL(clicked(bool)), this,
            SLOT(fullScreen(bool)));
    connect(cacheButton, SIGNAL(clicked(bool)), this, SLOT(cacheOn(bool)));
    connect(loadImageButton, SIGNAL(clicked()), this, SLOT(loadImage()));

    m_tableFormat.setBorder(0);
    // QTimer::singleShot(10 * 1000, this, SLOT(showInformation()));

    xTilesLineEdit->setText("1");
    yTilesLineEdit->setText("1");
}

void ShellDialog::appendMessage(const QString& from, const QString& message)
{
    if (from.isEmpty() || message.isEmpty())
        return;

    QTextCursor cursor(textEdit->textCursor());
    cursor.movePosition(QTextCursor::End);
    QTextTable* table = cursor.insertTable(1, 2, m_tableFormat);
    table->cellAt(0, 0).firstCursorPosition().insertText("<" + from + "> ");
    table->cellAt(0, 1).firstCursorPosition().insertText(message);
    QScrollBar* bar = textEdit->verticalScrollBar();
    bar->setValue(bar->maximum());
}

void ShellDialog::go()
{
    QString text = plainTextEdit->toPlainText();
    QString event = eventNameLineEdit->text();
    if (text.isEmpty())
        return;
    m_client->sendEvent(m_contact, event, eventTargetLineEdit->text(), text,
                        true);
    // plainTextEdit->clear();
}

void ShellDialog::newContact(const QString& contact)
{
    if (contact.isEmpty())
        return;

    QColor color = textEdit->textColor();
    textEdit->setTextColor(Qt::gray);
    textEdit->append(tr("Connected to %1").arg(contact));
    textEdit->setTextColor(color);
    listWidget->addItem(contact);
    m_contact = contact;
}

void ShellDialog::contactLeft(const QString& contact)
{
    if (contact.isEmpty())
        return;

    QList<QListWidgetItem*> items =
        listWidget->findItems(contact, Qt::MatchExactly);
    if (items.isEmpty())
        return;
    delete items.at(0);

    QColor color = textEdit->textColor();
    textEdit->setTextColor(Qt::gray);
    textEdit->append(tr("No longer connected to %1").arg(contact));
    textEdit->setTextColor(color);

    m_stop = true;
}

void ShellDialog::play()
{
    m_client->sendEvent(m_contact, "remote-eval", eventTargetLineEdit->text(),
                        "play();"
                        "\"FRAME = \" + frame();",
                        true);
}

void ShellDialog::stop()
{
    m_client->sendEvent(m_contact, "remote-eval", eventTargetLineEdit->text(),
                        "stop();"
                        "\"FRAME = \" + frame();",
                        true);
}

void ShellDialog::forward()
{
    m_client->sendEvent(m_contact, "remote-eval", eventTargetLineEdit->text(),
                        "extra_commands.stepForward1();"
                        "\"FRAME = \" + frame();",
                        true);
}

void ShellDialog::back()
{
    m_client->sendEvent(m_contact, "remote-eval", eventTargetLineEdit->text(),
                        "extra_commands.stepBackward1();"
                        "\"FRAME = \" + frame();",
                        true);
}

void ShellDialog::fullScreen(bool b)
{
    QString msg = QString("fullScreenMode(%1)").arg(b ? "true" : "false");
    m_client->sendEvent(m_contact, "remote-eval", eventTargetLineEdit->text(),
                        msg);
}

void ShellDialog::cacheOn(bool b)
{
    QString msg =
        QString("setCacheMode(%1)").arg(b ? "CacheBuffer" : "CacheOff");
    m_client->sendEvent(m_contact, "remote-eval", eventTargetLineEdit->text(),
                        msg);
}

void ShellDialog::loadImage()
{
    QString file = QFileDialog::getOpenFileName(this, "Select Image", ".",
                                                "Images (*.png *.xpm *.jpg)");

    if (file == "")
        return;

    if (!m_pixmap.load(file))
    {
        QMessageBox::critical(this, "Image not Read",
                              "The image could not be read");
        return;
    }

    delete[] m_imageData;
    m_imageData = 0;

    if (m_pixmapItem)
        m_scene->removeItem(m_pixmapItem);
    m_pixmapItem = m_scene->addPixmap(m_pixmap);

    QImage image = m_pixmap.toImage();

    const size_t w = image.width();
    const size_t h = image.height();
    const size_t scanlineSize = w * 4;
    m_imageData = new unsigned char[w * h * 4];

    for (size_t y = 0; y < h; y++)
    {
        unsigned char* scanline = m_imageData + scanlineSize * y;

        for (size_t x = 0; x < w; x++)
        {
            QRgb p = image.pixel(int(x), int(h - y - 1));
            scanline[x * 4 + 0] = qRed(p);
            scanline[x * 4 + 1] = qGreen(p);
            scanline[x * 4 + 2] = qBlue(p);
            scanline[x * 4 + 3] = qAlpha(p);
        }
    }

    //
    //  Each load creates a new image source in the remote rv. This
    //  creates a name based on the current time and date. This
    //  ensures that each time we load a new image we end up creating
    //  a new unique source on the RV side for it. Otherwise, we end
    //  up send pixel buckets to the same source over and over.
    //

    m_mediaName = QTime::currentTime().toString(
        QDate::currentDate().toString("yyyy-M-d-h:m:s:z"));
}

void ShellDialog::sendImage()
{
    //
    //  This is pointless if there's no image data
    //

    if (!m_imageData)
        return;

    static bool first = true;

    //
    //  Number of tiles the user specified in the UI
    //

    const int nx = xTilesLineEdit->text().toInt();
    const int ny = yTilesLineEdit->text().toInt();

    //
    //  Reset the stop variable. This may be set by contactLeft() in
    //  the case that we're hung up on.
    //

    m_stop = false;

    //
    //  Copy the QImage it into something we know the layout of
    //

    const size_t w = m_pixmap.width();
    const size_t h = m_pixmap.height();
    const size_t scanlineSize = w * 4;
    unsigned char* data = m_imageData;

    //
    //  NOTE: don't get confused by the order in which we built up the
    //  command string the next few lines of code. We're trying to
    //  send a single command to RV to create a unqiue image source
    //  that has space allocated for a left and right image. Since we
    //  don't really care what the source name is on this end (just
    //  the "media name" that we created and which identifies where
    //  the pixels should go) we create a temporary variable "s" which
    //  holds the source name. The source name is required to create
    //  new source pixels. By doing everything in one little code
    //  snippet we avoid having to get the source name and use it in
    //  subsequent calls to newImageSourcePixels(). This can be taken
    //  to an extreme and entire Mu modules with thousands of lines of
    //  code can be uploaded to RV like this.
    //

    //
    //  The newImageSource() function needs to be called for RV create
    //  space for the image. After this the pixel blocks will refer to
    //  the media name. The media name -- which we created when the
    //  image was loaded in loadImage() above -- identifies where the
    //  pixels will go. newImageSource() will return the name of the
    //  new source, but we'll only need that to setup the source
    //  pixels not when we send tiles.
    //

    if (first || !updateOnly)
    {
        first = false;
        QString newImageSource =
            QString("newImageSource( \"%1\", %2, %3, " // name, w, h
                    "%2, %3, 0, 0, "     // uncrop w, h, x-off, y-off,
                    "1.0, 4, 8, false, " // pixaspect, 4 ch, 8 bit, nofloat
                    "1, 1, 24.0, "       // fs, fe, fps
                    "nil, "              // layers (none in this case)
                    "string[] {\"left\", \"right\"} )") // views
                .arg(m_mediaName)
                .arg(w)
                .arg(h);

        //
        //  The "s" argument below will be the result of newImageSource()
        //  (which we constructed above) when we put this all
        //  together. For this example, we're sending stereo images (which
        //  is image the user chose and its inverse).
        //

        QString leftPixels = "newImageSourcePixels( s, 1, nil, \"left\")";
        QString rightPixels = "newImageSourcePixels( s, 1, nil, \"right\")";

        //
        //  Put together the newImageSource() and newImageSourcePixels()
        //  calls.  Make a Mu lexical block and call the two
        //  newImageSourcePixels() calls with the result of
        //  newImageSource(). So the final string will look like this:
        //
        //      {
        //          let s = newImageSource(...);
        //          newImageSourcePixels(s, ..., "left");
        //          newImageSourcePixels(s, ..., "right");
        //      }
        //
        //  The entire thing will be evaluated in one go on the RV end.
        //

        QString msg = QString("{ let s = %1; %2; %3; }")
                          .arg(newImageSource)
                          .arg(leftPixels)
                          .arg(rightPixels);

        //
        //  Send the command
        //

        m_client->sendEvent(m_contact, "remote-eval", "*", msg.toUtf8().data(),
                            true);

        m_client->waitForSend(m_contact);

        if (!m_client->waitForMessage(m_contact))
        {
            QMessageBox::critical(
                this, "Timeout",
                QString("%1 failed to respond in time").arg(m_contact));
            return;
        }
    }

    const unsigned char* d = m_imageData;
    size_t iw = w;
    size_t ih = h;

    //
    //  Loop over the tiles. If m_stop ever becomes true break out
    //  of the loops (nobody is listening).
    //

    for (size_t tx = 0; !m_stop && tx < nx; tx++)
    {
        size_t tw = iw / nx;
        size_t ix = tw * tx;
        if (tx == (nx - 1))
            tw = iw - tw * tx; // pick up the leftover

        for (size_t ty = 0; !m_stop && ty < ny; ty++)
        {
            size_t th = ih / ny;
            size_t iy = th * ty;
            if (ty == (ny - 1))
                th = ih - th * ty; // pick up the leftover

#if 0
            cout << "SENDING TILE (" << tx << ", " << ty << ")" 
                 << " -- tw = " << tw << ", th = " << th
                 << endl;
#endif

            QByteArray tile;
#ifdef PLATFORM_WINDOWS
            int ro = (updateOnly) ? (rand() % 128) : 0;
#else
            int ro = (updateOnly) ? (random() % 128) : 0;
#endif

            for (size_t tr = 0; tr < th; tr++)
            {
                for (size_t i = 0; i < tw * 4; i++)
                {
                    tile.push_back(
                        ro
                        + *(d + iy * (iw * 4) + tr * (iw * 4) + (ix * 4) + i));
                }
            }

            if (tile.size() != (tw * th * 4))
            {
                cout << "bad tiles" << endl;
                cout << "tile.size() = " << tile.size() << endl;
                cout << "tw * th * 4 = " << (tw * th * 4) << endl;
            }

            //
            //  Create the interp string. For RV the PIXELTILE looks
            //  like a python function call with keyword args. There
            //  can be no spaces in the string. The following arg
            //  variables are available, if they are not specified
            //  they get the defaults:
            //
            //      event            string, default = "pixel-block"
            //      tag (name)       unique identifier for this image
            //      media (name)     string, default = ""
            //      layer (name)     string, default = ""
            //      view  (name)     string, default = ""
            //      w (width)        value > 0, no default (required)
            //      h (height)       value > 0, no default (required)
            //      x (x tile pos)   any int, default is 0
            //      y (y tile pos)   any int, default is 0
            //      f (frame)        any int, default is 1
            //

            QString interpLeft =
                QString("PIXELTILE("
                        "media=%1,view=left,w=%2,h=%3,x=%4,y=%5,f=1)")
                    .arg(m_mediaName)
                    .arg(tw)
                    .arg(th)
                    .arg(ix)
                    .arg(iy);

            QString interpRight =
                QString("PIXELTILE("
                        "media=%1,view=right,w=%2,h=%3,x=%4,y=%5,f=1)")
                    .arg(m_mediaName)
                    .arg(tw)
                    .arg(th)
                    .arg(ix)
                    .arg(iy);

            m_client->sendData(m_contact, interpLeft, tile);
            m_client->waitForSend(m_contact);

            //
            //  Make an inverted tile for the other eye
            //

            for (int i = 0; i < tile.size(); i++)
            {
                tile[i] = 255 - tile[i];
            }

            m_client->sendData(m_contact, interpRight, tile);
            m_client->waitForSend(m_contact);

            //
            //  If we don't process events, the network lib will
            //  eventually timeout because it may not respond to
            //  ping" packets sent to verify the connection. You can
            //  use other methods, like creating a QEventLoop which
            //  will accomplish the same goal.
            //
            //  Also, if the RV we're talking to suddenly hangs up
            //  we'll need to know about it. The contactLeft() slot
            //  will be called during event processing causing the
            //  m_stop member variable to be set to true.
            //

            QCoreApplication::instance()->processEvents();
        }
    }
}

void ShellDialog::quit()
{
    m_client->signOff(m_contact);
    qApp->quit();
}
