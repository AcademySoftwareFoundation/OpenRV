//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <QtWidgets/QApplication>
#include <QFontDatabase>
#include <QFile>
#include <QIODevice>

#include <QTBundle/QTBundle.h>

#include <iostream>

TEST_CASE("Load QFont")
{
    int argc = 1;
    const char* argv[] = {"load_font_unit"};

    QApplication app(argc, const_cast<char**>(argv));
    const char* path = getenv("RV_FONT_PATH");
    REQUIRE_MESSAGE(path != nullptr,
                    "Please use RV_FONT_PATH to set a path to a font");

    const int loaded = QFontDatabase::addApplicationFont(path);
    CHECK_NE(loaded, -1);
}

TEST_CASE("Load QFont From Resource")
{
    int argc = 1;
    const char* argv[] = {"load_font_unit"};

    QApplication app(argc, const_cast<char**>(argv));

    Q_INIT_RESOURCE(RvCommon);

    QFile res(":/fonts/fontawesome-webfont.ttf");
    REQUIRE_MESSAGE(res.open(QIODevice::ReadOnly),
                    "Unable to find font in QRC");

    QByteArray fontData(res.readAll());
    res.close();

    const int loaded = QFontDatabase::addApplicationFontFromData(fontData);
    CHECK_NE(loaded, -1);
}

TEST_CASE("Load QFont From QTBundle")
{
    int argc = 1;
    const char* appName = "load_font_unit";
    const char* argv[] = {appName};

    QApplication app(argc, const_cast<char**>(argv));

    TwkApp::QTBundle bundle(appName, 0, 0, 0);
    bundle.initializeAfterQApplication();

    QFile res(":/fonts/fontawesome-webfont.ttf");
    REQUIRE_MESSAGE(res.open(QIODevice::ReadOnly),
                    "Unable to find font in QRC");

    QByteArray fontData(res.readAll());
    res.close();

    const int loaded = QFontDatabase::addApplicationFontFromData(fontData);
    CHECK_NE(loaded, -1);
}
