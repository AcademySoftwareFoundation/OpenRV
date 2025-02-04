//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __MuQt5__QTreeViewType__h__
#define __MuQt5__QTreeViewType__h__
#include <iostream>
#include <Mu/Class.h>
#include <Mu/MuProcess.h>
#include <QtCore/QtCore>
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>
#include <QtNetwork/QtNetwork>
#include <QtWebEngine/QtWebEngine>
#include <QtWebEngineWidgets/QtWebEngineWidgets>
#include <QtQml/QtQml>
#include <QtQuick/QtQuick>
#include <QtQuickWidgets/QtQuickWidgets>
#include <QtSvg/QtSvg>
#include <MuQt5/Bridge.h>

namespace Mu
{
    class MuQt_QTreeView;

    //
    //  NOTE: file generated by qt2mu.py
    //

    class QTreeViewType : public Class
    {
    public:
        typedef MuQt_QTreeView MuQtType;
        typedef QTreeView QtType;

        //
        //  Constructors
        //

        QTreeViewType(Context* context, const char* name, Class* superClass = 0,
                      Class* superClass2 = 0);

        virtual ~QTreeViewType();

        static bool isInheritable() { return true; }

        static inline ClassInstance* cachedInstance(const MuQtType*);

        //
        //  Class API
        //

        virtual void load();

        MemberFunction* _func[44];
    };

    // Inheritable object

    class MuQt_QTreeView : public QTreeView
    {
    public:
        virtual ~MuQt_QTreeView();
        MuQt_QTreeView(Pointer muobj, const CallEnvironment*, QWidget* parent);
        virtual QModelIndex indexAt(const QPoint& point) const;
        virtual void keyboardSearch(const QString& search);
        virtual void reset();
        virtual void selectAll();
        virtual void setModel(QAbstractItemModel* model);
        virtual void setRootIndex(const QModelIndex& index);
        virtual void setSelectionModel(QItemSelectionModel* selectionModel);
        virtual QRect visualRect(const QModelIndex& index) const;

    protected:
        virtual void currentChanged(const QModelIndex& current,
                                    const QModelIndex& previous);
        virtual void dragMoveEvent(QDragMoveEvent* event);
        virtual int horizontalOffset() const;
        virtual bool isIndexHidden(const QModelIndex& index) const;
        virtual void keyPressEvent(QKeyEvent* event);
        virtual void mouseDoubleClickEvent(QMouseEvent* event);
        virtual void mouseMoveEvent(QMouseEvent* event);
        virtual void mousePressEvent(QMouseEvent* event);
        virtual void mouseReleaseEvent(QMouseEvent* event);
        virtual void paintEvent(QPaintEvent* event);
        virtual void rowsAboutToBeRemoved(const QModelIndex& parent, int start,
                                          int end);
        virtual void rowsInserted(const QModelIndex& parent, int start,
                                  int end);
        virtual void scrollContentsBy(int dx, int dy);
        virtual QModelIndexList selectedIndexes() const;
        virtual void selectionChanged(const QItemSelection& selected,
                                      const QItemSelection& deselected);
        virtual void setSelection(const QRect& rect,
                                  QItemSelectionModel::SelectionFlags command);
        virtual int sizeHintForColumn(int column) const;
        virtual void timerEvent(QTimerEvent* event);
        virtual void updateGeometries();
        virtual int verticalOffset() const;
        virtual bool viewportEvent(QEvent* event);
        virtual QSize viewportSizeHint() const;
        virtual QRegion
        visualRegionForSelection(const QItemSelection& selection) const;

    public:
        virtual int sizeHintForRow(int row) const;
        virtual QVariant inputMethodQuery(Qt::InputMethodQuery query) const;

    protected:
        virtual bool edit(const QModelIndex& index,
                          QAbstractItemView::EditTrigger trigger,
                          QEvent* event);
        virtual QItemSelectionModel::SelectionFlags
        selectionCommand(const QModelIndex& index, const QEvent* event) const;
        virtual void startDrag(Qt::DropActions supportedActions);
        virtual void dragEnterEvent(QDragEnterEvent* event);
        virtual void dragLeaveEvent(QDragLeaveEvent* event);
        virtual void dropEvent(QDropEvent* event);
        virtual bool event(QEvent* event_);
        virtual void focusInEvent(QFocusEvent* event);
        virtual bool focusNextPrevChild(bool next);
        virtual void focusOutEvent(QFocusEvent* event);
        virtual void resizeEvent(QResizeEvent* event);

    public:
        int indexRowSizeHint_pub(const QModelIndex& index) const
        {
            return indexRowSizeHint(index);
        }

        int indexRowSizeHint_pub_parent(const QModelIndex& index) const
        {
            return QTreeView::indexRowSizeHint(index);
        }

        int rowHeight_pub(const QModelIndex& index) const
        {
            return rowHeight(index);
        }

        int rowHeight_pub_parent(const QModelIndex& index) const
        {
            return QTreeView::rowHeight(index);
        }

        void currentChanged_pub(const QModelIndex& current,
                                const QModelIndex& previous)
        {
            currentChanged(current, previous);
        }

        void currentChanged_pub_parent(const QModelIndex& current,
                                       const QModelIndex& previous)
        {
            QTreeView::currentChanged(current, previous);
        }

        void dragMoveEvent_pub(QDragMoveEvent* event) { dragMoveEvent(event); }

        void dragMoveEvent_pub_parent(QDragMoveEvent* event)
        {
            QTreeView::dragMoveEvent(event);
        }

        int horizontalOffset_pub() const { return horizontalOffset(); }

        int horizontalOffset_pub_parent() const
        {
            return QTreeView::horizontalOffset();
        }

        bool isIndexHidden_pub(const QModelIndex& index) const
        {
            return isIndexHidden(index);
        }

        bool isIndexHidden_pub_parent(const QModelIndex& index) const
        {
            return QTreeView::isIndexHidden(index);
        }

        void keyPressEvent_pub(QKeyEvent* event) { keyPressEvent(event); }

        void keyPressEvent_pub_parent(QKeyEvent* event)
        {
            QTreeView::keyPressEvent(event);
        }

        void mouseDoubleClickEvent_pub(QMouseEvent* event)
        {
            mouseDoubleClickEvent(event);
        }

        void mouseDoubleClickEvent_pub_parent(QMouseEvent* event)
        {
            QTreeView::mouseDoubleClickEvent(event);
        }

        void mouseMoveEvent_pub(QMouseEvent* event) { mouseMoveEvent(event); }

        void mouseMoveEvent_pub_parent(QMouseEvent* event)
        {
            QTreeView::mouseMoveEvent(event);
        }

        void mousePressEvent_pub(QMouseEvent* event) { mousePressEvent(event); }

        void mousePressEvent_pub_parent(QMouseEvent* event)
        {
            QTreeView::mousePressEvent(event);
        }

        void mouseReleaseEvent_pub(QMouseEvent* event)
        {
            mouseReleaseEvent(event);
        }

        void mouseReleaseEvent_pub_parent(QMouseEvent* event)
        {
            QTreeView::mouseReleaseEvent(event);
        }

        void paintEvent_pub(QPaintEvent* event) { paintEvent(event); }

        void paintEvent_pub_parent(QPaintEvent* event)
        {
            QTreeView::paintEvent(event);
        }

        void rowsAboutToBeRemoved_pub(const QModelIndex& parent, int start,
                                      int end)
        {
            rowsAboutToBeRemoved(parent, start, end);
        }

        void rowsAboutToBeRemoved_pub_parent(const QModelIndex& parent,
                                             int start, int end)
        {
            QTreeView::rowsAboutToBeRemoved(parent, start, end);
        }

        void rowsInserted_pub(const QModelIndex& parent, int start, int end)
        {
            rowsInserted(parent, start, end);
        }

        void rowsInserted_pub_parent(const QModelIndex& parent, int start,
                                     int end)
        {
            QTreeView::rowsInserted(parent, start, end);
        }

        void scrollContentsBy_pub(int dx, int dy) { scrollContentsBy(dx, dy); }

        void scrollContentsBy_pub_parent(int dx, int dy)
        {
            QTreeView::scrollContentsBy(dx, dy);
        }

        QModelIndexList selectedIndexes_pub() const
        {
            return selectedIndexes();
        }

        QModelIndexList selectedIndexes_pub_parent() const
        {
            return QTreeView::selectedIndexes();
        }

        void selectionChanged_pub(const QItemSelection& selected,
                                  const QItemSelection& deselected)
        {
            selectionChanged(selected, deselected);
        }

        void selectionChanged_pub_parent(const QItemSelection& selected,
                                         const QItemSelection& deselected)
        {
            QTreeView::selectionChanged(selected, deselected);
        }

        void setSelection_pub(const QRect& rect,
                              QItemSelectionModel::SelectionFlags command)
        {
            setSelection(rect, command);
        }

        void
        setSelection_pub_parent(const QRect& rect,
                                QItemSelectionModel::SelectionFlags command)
        {
            QTreeView::setSelection(rect, command);
        }

        int sizeHintForColumn_pub(int column) const
        {
            return sizeHintForColumn(column);
        }

        int sizeHintForColumn_pub_parent(int column) const
        {
            return QTreeView::sizeHintForColumn(column);
        }

        void timerEvent_pub(QTimerEvent* event) { timerEvent(event); }

        void timerEvent_pub_parent(QTimerEvent* event)
        {
            QTreeView::timerEvent(event);
        }

        void updateGeometries_pub() { updateGeometries(); }

        void updateGeometries_pub_parent() { QTreeView::updateGeometries(); }

        int verticalOffset_pub() const { return verticalOffset(); }

        int verticalOffset_pub_parent() const
        {
            return QTreeView::verticalOffset();
        }

        bool viewportEvent_pub(QEvent* event) { return viewportEvent(event); }

        bool viewportEvent_pub_parent(QEvent* event)
        {
            return QTreeView::viewportEvent(event);
        }

        QSize viewportSizeHint_pub() const { return viewportSizeHint(); }

        QSize viewportSizeHint_pub_parent() const
        {
            return QTreeView::viewportSizeHint();
        }

        QRegion
        visualRegionForSelection_pub(const QItemSelection& selection) const
        {
            return visualRegionForSelection(selection);
        }

        QRegion visualRegionForSelection_pub_parent(
            const QItemSelection& selection) const
        {
            return QTreeView::visualRegionForSelection(selection);
        }

        bool edit_pub(const QModelIndex& index,
                      QAbstractItemView::EditTrigger trigger, QEvent* event)
        {
            return edit(index, trigger, event);
        }

        bool edit_pub_parent(const QModelIndex& index,
                             QAbstractItemView::EditTrigger trigger,
                             QEvent* event)
        {
            return QTreeView::edit(index, trigger, event);
        }

        QItemSelectionModel::SelectionFlags
        selectionCommand_pub(const QModelIndex& index,
                             const QEvent* event) const
        {
            return selectionCommand(index, event);
        }

        QItemSelectionModel::SelectionFlags
        selectionCommand_pub_parent(const QModelIndex& index,
                                    const QEvent* event) const
        {
            return QTreeView::selectionCommand(index, event);
        }

        void startDrag_pub(Qt::DropActions supportedActions)
        {
            startDrag(supportedActions);
        }

        void startDrag_pub_parent(Qt::DropActions supportedActions)
        {
            QTreeView::startDrag(supportedActions);
        }

        void dragEnterEvent_pub(QDragEnterEvent* event)
        {
            dragEnterEvent(event);
        }

        void dragEnterEvent_pub_parent(QDragEnterEvent* event)
        {
            QTreeView::dragEnterEvent(event);
        }

        void dragLeaveEvent_pub(QDragLeaveEvent* event)
        {
            dragLeaveEvent(event);
        }

        void dragLeaveEvent_pub_parent(QDragLeaveEvent* event)
        {
            QTreeView::dragLeaveEvent(event);
        }

        void dropEvent_pub(QDropEvent* event) { dropEvent(event); }

        void dropEvent_pub_parent(QDropEvent* event)
        {
            QTreeView::dropEvent(event);
        }

        bool event_pub(QEvent* event_) { return event(event_); }

        bool event_pub_parent(QEvent* event_)
        {
            return QTreeView::event(event_);
        }

        void focusInEvent_pub(QFocusEvent* event) { focusInEvent(event); }

        void focusInEvent_pub_parent(QFocusEvent* event)
        {
            QTreeView::focusInEvent(event);
        }

        bool focusNextPrevChild_pub(bool next)
        {
            return focusNextPrevChild(next);
        }

        bool focusNextPrevChild_pub_parent(bool next)
        {
            return QTreeView::focusNextPrevChild(next);
        }

        void focusOutEvent_pub(QFocusEvent* event) { focusOutEvent(event); }

        void focusOutEvent_pub_parent(QFocusEvent* event)
        {
            QTreeView::focusOutEvent(event);
        }

        void resizeEvent_pub(QResizeEvent* event) { resizeEvent(event); }

        void resizeEvent_pub_parent(QResizeEvent* event)
        {
            QTreeView::resizeEvent(event);
        }

    public:
        const QTreeViewType* _baseType;
        ClassInstance* _obj;
        const CallEnvironment* _env;
    };

    inline ClassInstance*
    QTreeViewType::cachedInstance(const QTreeViewType::MuQtType* obj)
    {
        return obj->_obj;
    }

} // namespace Mu

#endif // __MuQt__QTreeViewType__h__
