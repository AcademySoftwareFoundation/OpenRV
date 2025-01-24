//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __rv_qt__QTPaletteDump__h__
#define __rv_qt__QTPaletteDump__h__

#include <QtGui/QPixmap>
#include <QtGui/QIcon>
#include <QtCore/QString>

namespace Rv
{

    //
    //  Return an icon from a resource path or file. The icon will be
    //  inverted if the background is darker than HSV value < 0.5
    //

    QPixmap colorAdjustedPixmap(const QString&, bool invertSense = false);
    QIcon colorAdjustedIcon(const QString&, bool invertSense = false);

    //
    //  Call to make a copy of the default palette
    //

    void initializeDefaultPalette();
    void setDefaultPalette();

    //
    //  Hard coded "dark" palette
    //

    void installApplicationPalette();
    void dumpApplicationPalette();

} // namespace Rv

#endif // __rv_qt__QTPaletteDump__h__
