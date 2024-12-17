//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __TwkQtCoreUtil__QtConvert__h__
#define __TwkQtCoreUtil__QtConvert__h__
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QStringList>

//

namespace TwkQtCoreUtil
{
    namespace UTF8
    {

        QString qconvert(const std::string&);
        QString qconvert(const char*);
        QString qconvert(const std::ostringstream&);
        std::string qconvert(QString);
        std::string qconvert(QUrl);
        std::vector<std::string> qconvert(QStringList);
        QStringList qconvert(const std::vector<std::string>&);

    } // namespace UTF8
} // namespace TwkQtCoreUtil

#endif // __TwkQtCoreUtil__QtConvert__h__
