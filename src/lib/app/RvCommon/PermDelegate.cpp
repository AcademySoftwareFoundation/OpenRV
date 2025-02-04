//
//  Copyright (c) 2008 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <RvCommon/PermDelegate.h>
#include <QtWidgets/QComboBox>
#include <QtGui/QStandardItemModel>

namespace Rv
{
    using namespace std;

    PermDelegate::PermDelegate(QObject* parent)
        : QItemDelegate(parent)
    {
    }

    QWidget*
    PermDelegate::createEditor(QWidget* parent,
                               const QStyleOptionViewItem& /* option */,
                               const QModelIndex& /* index */) const
    {
        return new QComboBox(parent);
    }

    void PermDelegate::setEditorData(QWidget* editor,
                                     const QModelIndex& index) const
    {
        int value = index.model()->data(index, Qt::EditRole).toInt();

        const QStandardItemModel* m =
            static_cast<const QStandardItemModel*>(index.model());

        QComboBox* box = static_cast<QComboBox*>(editor);
        box->addItem("Ask");
        box->addItem("Allow");
        box->addItem("Reject");

        QStandardItem* i = m->item(index.row(), index.column());

        if (i->text() == "Ask")
            box->setCurrentIndex(0);
        else if (i->text() == "Allow")
            box->setCurrentIndex(1);
        else if (i->text() == "Reject")
            box->setCurrentIndex(2);
    }

    void PermDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                    const QModelIndex& index) const
    {
        QComboBox* cbox = static_cast<QComboBox*>(editor);
        QStandardItemModel* m = static_cast<QStandardItemModel*>(model);
        QStandardItem* i = m->item(index.row(), index.column());
        i->setText(cbox->itemText(cbox->currentIndex()));

        // Just to make it signal
        m->setData(index, i->text(), Qt::EditRole);
    }

    void
    PermDelegate::updateEditorGeometry(QWidget* editor,
                                       const QStyleOptionViewItem& option,
                                       const QModelIndex& /* index */) const
    {
        editor->setGeometry(option.rect);
    }

} // namespace Rv
