//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <TwkQtCoreUtil/QtConvert.h>

using namespace std;

namespace TwkQtCoreUtil
{
    namespace UTF8
    {

        QString qconvert(const string& s)
        {
            return QString::fromUtf8(s.c_str(), s.size());
        }

        QString qconvert(const char* s)
        {
            if (s)
            {
                return QString::fromUtf8(s, strlen(s));
            }
            else
            {
                return QString();
            }
        }

        QString qconvert(const std::ostringstream& str)
        {
            string s = str.str();
            return QString::fromUtf8(s.c_str(), s.size());
        }

        string qconvert(QString s)
        {
            string x(s.toUtf8().constData());
            return x;
        }

        string qconvert(QUrl s)
        {
            string x(s.toString().toUtf8().constData());
            return x;
        }

        vector<string> qconvert(QStringList strings)
        {
            vector<string> newList(strings.size());

            for (size_t i = 0; i < strings.size(); i++)
            {
                newList[i] = qconvert(strings[i]);
            }

            return newList;
        }

        QStringList qconvert(const std::vector<std::string>& v)
        {
            QStringList list;
            for (size_t i = 0, s = v.size(); i < s; i++)
                list << v[i].c_str();
            return list;
        }

    } // namespace UTF8
} // namespace TwkQtCoreUtil
