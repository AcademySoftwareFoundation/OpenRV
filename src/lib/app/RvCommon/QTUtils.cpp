//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <RvCommon/QTUtils.h>
#include <QtCore/QtCore>
#include <QtGui/QtGui>
#include <iostream>
#include <map>

namespace Rv
{
    using namespace std;
    typedef map<int, QColor> CMap;

    QPixmap colorAdjustedPixmap(const QString& rpath, bool invertSense)
    {
        const char* darkval = getenv("RV_DARK");
        bool dark = darkval && !strcmp(darkval, "1");

        QImage image(rpath);

        if (dark != invertSense)
        {
            image.invertPixels();
        }

        return QPixmap(QPixmap::fromImage(image));
    }

    QIcon colorAdjustedIcon(const QString& rpath, bool invertSense)
    {
        const char* darkval = getenv("RV_DARK");
        bool dark = darkval && !strcmp(darkval, "1");

        QImage image(rpath);

        if (dark != invertSense)
        {
            image.invertPixels();
        }

        return QIcon(QPixmap::fromImage(image));
    }

    static QPalette defaultPalette;

    void setDefaultPalette() { qApp->setPalette(defaultPalette); }

    void initializeDefaultPalette() { defaultPalette = qApp->palette(); }

    void installApplicationPalette()
    {
        QPalette p = qApp->palette();
        p.setColor(QPalette::Active, QPalette::WindowText,
                   QColor(217, 217, 197));
        p.setColor(QPalette::Active, QPalette::Button, QColor(99, 99, 99));
        p.setColor(QPalette::Active, QPalette::Window, QColor(68, 68, 66));
        p.setColor(QPalette::Active, QPalette::Shadow, QColor(0, 0, 0));
        p.setColor(QPalette::Active, QPalette::Highlight,
                   QColor(117, 117, 112));
        p.setColor(QPalette::Active, QPalette::HighlightedText,
                   QColor(248, 248, 227));
        p.setColor(QPalette::Active, QPalette::Link, QColor(0, 0, 238));
        p.setColor(QPalette::Active, QPalette::LinkVisited,
                   QColor(82, 24, 139));
        p.setColor(QPalette::Active, QPalette::AlternateBase,
                   QColor(65, 69, 65));
        p.setColor(QPalette::Active, QPalette::Light, QColor(106, 106, 106));
        p.setColor(QPalette::Active, QPalette::Midlight, QColor(124, 124, 124));
        p.setColor(QPalette::Active, QPalette::Dark, QColor(49, 49, 49));
        p.setColor(QPalette::Active, QPalette::Mid, QColor(66, 66, 66));
        p.setColor(QPalette::Active, QPalette::Text, QColor(180, 180, 163));
        p.setColor(QPalette::Active, QPalette::BrightText,
                   QColor(255, 255, 255));
        p.setColor(QPalette::Active, QPalette::ButtonText,
                   QColor(234, 234, 213));
        p.setColor(QPalette::Active, QPalette::Base, QColor(55, 59, 55));

        p.setColor(QPalette::Disabled, QPalette::WindowText,
                   QColor(128, 128, 128));
        p.setColor(QPalette::Disabled, QPalette::Button, QColor(99, 99, 99));
        p.setColor(QPalette::Disabled, QPalette::Window, QColor(68, 68, 66));
        p.setColor(QPalette::Disabled, QPalette::Shadow, QColor(0, 0, 0));
        // p.setColor(QPalette::Disabled, QPalette::Highlight,       QColor(117,
        // 117, 112));
        //  Highlight and HighlightedText seem to be swapped in some cases
        p.setColor(QPalette::Disabled, QPalette::Highlight, QColor(55, 59, 55));
        p.setColor(QPalette::Disabled, QPalette::HighlightedText,
                   QColor(128, 128, 128));
        p.setColor(QPalette::Disabled, QPalette::Link, QColor(0, 0, 238));
        p.setColor(QPalette::Disabled, QPalette::LinkVisited,
                   QColor(82, 24, 139));
        p.setColor(QPalette::Disabled, QPalette::AlternateBase,
                   QColor(65, 69, 65));
        p.setColor(QPalette::Disabled, QPalette::Light, QColor(106, 106, 106));
        p.setColor(QPalette::Disabled, QPalette::Midlight,
                   QColor(124, 124, 124));
        p.setColor(QPalette::Disabled, QPalette::Dark, QColor(49, 49, 49));
        p.setColor(QPalette::Disabled, QPalette::Mid, QColor(66, 66, 66));
        p.setColor(QPalette::Disabled, QPalette::Text, QColor(91, 90, 90));
        p.setColor(QPalette::Disabled, QPalette::BrightText,
                   QColor(255, 255, 255));
        p.setColor(QPalette::Disabled, QPalette::ButtonText,
                   QColor(128, 128, 128));
        p.setColor(QPalette::Disabled, QPalette::Base, QColor(55, 59, 55));

        p.setColor(QPalette::Inactive, QPalette::WindowText,
                   QColor(217, 217, 197));
        p.setColor(QPalette::Inactive, QPalette::Button, QColor(99, 99, 99));
        p.setColor(QPalette::Inactive, QPalette::Window, QColor(68, 68, 66));
        p.setColor(QPalette::Inactive, QPalette::Shadow, QColor(0, 0, 0));
        p.setColor(QPalette::Inactive, QPalette::Highlight, QColor(55, 59, 55));
        p.setColor(QPalette::Inactive, QPalette::HighlightedText,
                   QColor(128, 128, 128));
        p.setColor(QPalette::Inactive, QPalette::Link, QColor(0, 0, 238));
        p.setColor(QPalette::Inactive, QPalette::LinkVisited,
                   QColor(82, 24, 139));
        p.setColor(QPalette::Inactive, QPalette::AlternateBase,
                   QColor(65, 69, 65));
        p.setColor(QPalette::Inactive, QPalette::Light, QColor(106, 106, 106));
        p.setColor(QPalette::Inactive, QPalette::Midlight,
                   QColor(124, 124, 124));
        p.setColor(QPalette::Inactive, QPalette::Dark, QColor(49, 49, 49));
        p.setColor(QPalette::Inactive, QPalette::Mid, QColor(66, 66, 66));
        p.setColor(QPalette::Inactive, QPalette::Text, QColor(180, 180, 163));
        p.setColor(QPalette::Inactive, QPalette::BrightText,
                   QColor(255, 255, 255));
        p.setColor(QPalette::Inactive, QPalette::ButtonText,
                   QColor(234, 234, 213));
        p.setColor(QPalette::Inactive, QPalette::Base, QColor(55, 59, 55));
        qApp->setPalette(p);
        // QFont font(QString("Kabel DM BT"), 12);
        // QFont f = qApp->font();
        // f.setPointSize(10);
        // qApp->setFont(f);
    }

    void dumpApplicationPalette()
    {
        QPalette p = qApp->palette();

        CMap active;
        CMap inactive;
        CMap disabled;

        for (int i = 0; i < 17; i++)
        {
            active[i] = p.color(QPalette::Active, (QPalette::ColorRole)i);
            inactive[i] = p.color(QPalette::Inactive, (QPalette::ColorRole)i);
            disabled[i] = p.color(QPalette::Disabled, (QPalette::ColorRole)i);

            QColor c = active[i];
            int r = c.red();
            int g = c.green();
            int b = c.blue();

            cout << "p.setColor(QPalette::Active, (QPalette::ColorRole)" << i
                 << ", QColor(" << r << ", " << g << ", " << b << "));" << endl;

            c = inactive[i];
            r = c.red();
            g = c.green();
            b = c.blue();

            cout << "p.setColor(QPalette::Inactive, (QPalette::ColorRole)" << i
                 << ", QColor(" << r << ", " << g << ", " << b << "));" << endl;

            c = disabled[i];
            r = c.red();
            g = c.green();
            b = c.blue();

            cout << "p.setColor(QPalette::Disabled, (QPalette::ColorRole)" << i
                 << ", QColor(" << r << ", " << g << ", " << b << "));" << endl;
        }
    }

} // namespace Rv
