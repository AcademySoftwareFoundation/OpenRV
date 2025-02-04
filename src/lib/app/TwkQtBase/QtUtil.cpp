//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <TwkQtBase/QtUtil.h>

#include <QtCore/QtCore>
#include <TwkUtil/User.h>
#include <sstream>
#include <time.h>

using namespace std;

namespace TwkQtBase
{

    namespace
    {

        const char* ky(int index)
        {
            return (index == 0) ? "StringType::String* url = NODE_ARG_OBJECT"
                                : "const StringType* stype = static_cast<const "
                                  "StringType*>";
        }

    }; // namespace

    string encode(string pass, int key)
    {
        static bool first = true;

        QCryptographicHash qch(QCryptographicHash::Sha1);
        string topSecretKey = ky(key);
        string uidStr = TwkUtil::uidString();
        ostringstream text;
        text << topSecretKey << "-" << uidStr;
        string simpleKey = text.str();
        QString saltStr;

        if (first)
        {
            time_t tt;
            time(&tt);
            srand((int)tt);
            first = false;
        }
        saltStr.setNum(rand());
        text << "-" << saltStr.toUtf8().constData();

        //
        //  First round
        //
        QByteArray textArray(text.str().c_str());
        QString qpass(pass.c_str());
        ;
        QString withoutSemis = qpass.replace(";", "###SEMICOLON###");

        ostringstream passStr;
        passStr << withoutSemis.toStdString() << ";" << "succeeded";
        QByteArray passArray(passStr.str().c_str());

        textArray = qch.hash(textArray, QCryptographicHash::Sha1);

        for (int i = 0; i < passArray.size(); ++i)
            passArray[i] = passArray[i] ^ textArray[i % textArray.size()];

        //
        //  Second round
        //
        passArray = saltStr.toUtf8() + ";" + passArray;
        ;

        textArray = QByteArray(simpleKey.c_str());
        textArray = qch.hash(textArray, QCryptographicHash::Sha1);

        for (int i = 0; i < passArray.size(); ++i)
        {
            passArray[i] = passArray[i] ^ textArray[i % textArray.size()];
        }

        passArray = passArray.toHex();
        QString qs(passArray);

        return (qs.toStdString());
    }

    string decode(string pass, int key)
    {
        QByteArray passArray(pass.c_str());
        passArray = QByteArray::fromHex(passArray);

        //
        //  Undo second round
        //

        QCryptographicHash qch(QCryptographicHash::Sha1);
        string topSecretKey = ky(key);
        ostringstream text;
        text << topSecretKey << "-" << TwkUtil::uidString();
        QByteArray textArray(text.str().c_str());

        textArray = qch.hash(textArray, QCryptographicHash::Sha1);
        for (int i = 0; i < passArray.size(); ++i)
            passArray[i] = passArray[i] ^ textArray[i % textArray.size()];

        //
        //  Undo first round
        //

        QStringList qsl = (QString(passArray).split(";"));
        int breakPoint = passArray.indexOf(';');
        if (breakPoint == -1)
            return "";

        QByteArray randomInt = passArray;
        randomInt.chop(randomInt.size() - breakPoint);
        passArray.remove(0, breakPoint + 1);
        text << "-" << randomInt.constData();
        textArray = QByteArray(text.str().c_str());

        textArray = qch.hash(textArray, QCryptographicHash::Sha1);

        for (int i = 0; i < passArray.size(); ++i)
            passArray[i] = passArray[i] ^ textArray[i % textArray.size()];

        //
        //  Check result
        //

        QString qs(passArray);
        qsl = (QString(passArray)).split(";");

        if (qsl.size() == 2 && qsl[1] == "succeeded")
        {
            QString withSemis = qsl[0].replace("###SEMICOLON###", ";");
            return withSemis.toStdString();
        }
        else
            return "";
    }

}; //  namespace TwkQtBase
