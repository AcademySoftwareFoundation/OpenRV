//
//  Copyright (c) 2008 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __RvCommon__PermDelegate__h__
#define __RvCommon__PermDelegate__h__
#include <iostream>
#include <QtCore/QtCore>
#include <QtWidgets/QItemDelegate>

namespace Rv
{

    //
    //  Permissions combo delegate for editing the contacts UI
    //

    class PermDelegate : public QItemDelegate
    {
        Q_OBJECT
    public:
        PermDelegate(QObject* parent = 0);

        QWidget* createEditor(QWidget* parent,
                              const QStyleOptionViewItem& option,
                              const QModelIndex& index) const;

        void setEditorData(QWidget* editor, const QModelIndex& index) const;

        void setModelData(QWidget* editor, QAbstractItemModel* model,
                          const QModelIndex& index) const;

        void updateEditorGeometry(QWidget* editor,
                                  const QStyleOptionViewItem& option,
                                  const QModelIndex& index) const;
    };

} // namespace Rv

#endif // __RvCommon__PermDelegate__h__
