//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <MuQt5/HintWidget.h>
#include <MuQt5/qtUtils.h>

namespace Mu
{
    using namespace std;

    HintWidget::HintWidget(QWidget* parent, QSize s)
        : QWidget(parent)
        , m_sizeHint(s)
    {
    }

    HintWidget::~HintWidget() {}

    void HintWidget::setWidget(QWidget* w)
    {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->addWidget(w);
        layout->setContentsMargins(0, 0, 0, 0);
        setLayout(layout);
    }

    QSize HintWidget::sizeHint() const { return m_sizeHint; }

} // namespace Mu
