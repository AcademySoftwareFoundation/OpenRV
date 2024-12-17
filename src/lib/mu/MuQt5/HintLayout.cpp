//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <MuQt5/qtUtils.h>
#include <MuQt5/HintLayout.h>

namespace Mu
{
    using namespace std;

    HintLayout::HintLayout(QWidget* parent, QSize size)
        : QLayout(parent)
        , m_size(size)
    {
    }

    HintLayout::~HintLayout()
    {
        QLayoutItem* item;
        while (item = takeAt(0))
            delete item;
    }

    void HintLayout::addItem(QLayoutItem* item) { m_itemList.append(item); }

    int HintLayout::count() const { return m_itemList.size(); }

    QLayoutItem* HintLayout::itemAt(int index) const
    {
        return m_itemList.value(index);
    }

    QLayoutItem* HintLayout::takeAt(int index)
    {
        if (index >= 0 && index < m_itemList.size())
        {
            return m_itemList.takeAt(index);
        }
        else
        {
            return 0;
        }
    }

    QSize HintLayout::sizeHint() const { return m_size; }

} // namespace Mu
