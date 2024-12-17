//
//  Copyright (c) 2017 Autodesk.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __TwkQtCoreUtil__EnumNames__h__
#define __TwkQtCoreUtil__EnumNames__h__
#include <iostream>

namespace TwkQtCoreUtil
{

    template <typename EnumType> QString toString(const EnumType& enumValue)
    {
        const char* enumName = qt_getEnumName(enumValue);
        const QMetaObject* metaObject = qt_getEnumMetaObject(enumValue);
        if (metaObject)
        {
            const int enumIndex = metaObject->indexOfEnumerator(enumName);
            return QString("%1::%2::%3")
                .arg(metaObject->className(), enumName,
                     metaObject->enumerator(enumIndex).valueToKey(enumValue));
        }

        return QString("%1::%2").arg(enumName).arg(static_cast<int>(enumValue));
    }

} // namespace TwkQtCoreUtil

#endif // __TwkQtCoreUtil__EnumNames__h__
