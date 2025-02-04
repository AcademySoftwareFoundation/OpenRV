//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __MuQt__Bridge__h__
#define __MuQt__Bridge__h__
#include <iostream>
#include <MuQt5/qtUtils.h>
#include <QtCore/QtCore>
#include <QtNetwork/QtNetwork>
#include <Mu/Type.h>
#include <Mu/Class.h>

namespace Mu
{

    void dumpMetaInfo(const QMetaObject&);

    const char* qtType(const Mu::Type* t);

    void populate(Class*, const QMetaObject&, const char** propExclusions = 0);

    QGenericArgument argument(STLVector<Pointer>::Type& gcCache, const Type* T,
                              Value& v, QString& s, QVariant& qv);

} // namespace Mu

#endif // __MuQt__Bridge__h__
