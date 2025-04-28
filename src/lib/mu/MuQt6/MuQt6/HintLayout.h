//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __MuQt__HintLayout__h__
#define __MuQt__HintLayout__h__
#include <iostream>
#include <QtGui/QtGui>

namespace Mu
{

    class HintLayout : public QLayout
    {
    public:
        HintLayout(QWidget* parent, QSize);
        virtual ~HintLayout();

        virtual void addItem(QLayoutItem* item);
        virtual int count() const;
        virtual QLayoutItem* itemAt(int index) const;
        virtual QLayoutItem* takeAt(int index);
        virtual QSize sizeHint() const;

    private:
        QSize m_size;
        QList<QLayoutItem*> m_itemList;
    };

} // namespace Mu

#endif // __MuQt__HintLayout__h__
