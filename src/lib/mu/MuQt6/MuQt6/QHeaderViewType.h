//*****************************************************************************
// Copyright (c) 2024 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************

// IMPORTANT: This file (not the template) is auto-generated by qt2mu.py script.
//            The prefered way to do a fix is to handrolled it or modify the qt2mu.py script.
//            If it is not possible, manual editing is ok but it could be lost in future generations.

#ifndef __MuQt6__QHeaderViewType__h__
#define __MuQt6__QHeaderViewType__h__
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
#include <MuQt6/Bridge.h>

namespace Mu {
class MuQt_QHeaderView;

//
//  NOTE: file generated by qt2mu.py
//

class QHeaderViewType : public Class
{
  public:

    typedef MuQt_QHeaderView MuQtType;
    typedef QHeaderView QtType;

    //
    //  Constructors
    //

    QHeaderViewType(Context* context, 
           const char* name,
           Class* superClass = 0,
           Class* superClass2 = 0);

    virtual ~QHeaderViewType();

    static bool isInheritable() { return true; }
    static inline ClassInstance* cachedInstance(const MuQtType*);

    //
    //  Class API
    //

    virtual void load();

    MemberFunction* _func[39];
};

// Inheritable object

class MuQt_QHeaderView : public QHeaderView
{
  public:
    virtual ~MuQt_QHeaderView();
    MuQt_QHeaderView(Pointer muobj, const CallEnvironment*, Qt::Orientation orientation, QWidget * parent) ;
    virtual void reset() ;
    virtual void setModel(QAbstractItemModel * model) ;
    virtual void setVisible(bool v) ;
    virtual QSize sizeHint() const;
  protected:
    virtual QSize sectionSizeFromContents(int logicalIndex) const;
    virtual void currentChanged(const QModelIndex & current, const QModelIndex & old) ;
    virtual bool event(QEvent * e) ;
    virtual int horizontalOffset() const;
    virtual void mouseDoubleClickEvent(QMouseEvent * e) ;
    virtual void mouseMoveEvent(QMouseEvent * e) ;
    virtual void mousePressEvent(QMouseEvent * e) ;
    virtual void mouseReleaseEvent(QMouseEvent * e) ;
    virtual void paintEvent(QPaintEvent * e) ;
    virtual void setSelection(const QRect & rect, QItemSelectionModel::SelectionFlags flags) ;
    virtual int verticalOffset() const;
    virtual bool viewportEvent(QEvent * e) ;
  public:
    virtual void keyboardSearch(const QString & search) ;
    virtual void setSelectionModel(QItemSelectionModel * selectionModel) ;
    virtual int sizeHintForColumn(int column) const;
    virtual int sizeHintForRow(int row) const;
    virtual QVariant inputMethodQuery(Qt::InputMethodQuery query) const;
  protected:
    virtual bool edit(const QModelIndex & index, QAbstractItemView::EditTrigger trigger, QEvent * event) ;
    virtual bool isIndexHidden(const QModelIndex & index) const ;
    virtual QModelIndexList selectedIndexes() const;
    virtual QItemSelectionModel::SelectionFlags selectionCommand(const QModelIndex & index, const QEvent * event) const;
    virtual void startDrag(Qt::DropActions supportedActions) ;
    virtual QRegion visualRegionForSelection(const QItemSelection & selection) const ;
    virtual void dragEnterEvent(QDragEnterEvent * event) ;
    virtual void dragLeaveEvent(QDragLeaveEvent * event) ;
    virtual void dragMoveEvent(QDragMoveEvent * event) ;
    virtual void dropEvent(QDropEvent * event) ;
    virtual bool eventFilter(QObject * object, QEvent * event) ;
    virtual void focusInEvent(QFocusEvent * event) ;
    virtual bool focusNextPrevChild(bool next) ;
    virtual void focusOutEvent(QFocusEvent * event) ;
    virtual void keyPressEvent(QKeyEvent * event) ;
    virtual void resizeEvent(QResizeEvent * event) ;
    virtual void timerEvent(QTimerEvent * event) ;
    virtual QSize viewportSizeHint() const;
  public:
    QSize sectionSizeFromContents_pub(int logicalIndex) const { return sectionSizeFromContents(logicalIndex); }
    QSize sectionSizeFromContents_pub_parent(int logicalIndex) const { return QHeaderView::sectionSizeFromContents(logicalIndex); }
    void currentChanged_pub(const QModelIndex & current, const QModelIndex & old)  { currentChanged(current, old); }
    void currentChanged_pub_parent(const QModelIndex & current, const QModelIndex & old)  { QHeaderView::currentChanged(current, old); }
    bool event_pub(QEvent * e)  { return event(e); }
    bool event_pub_parent(QEvent * e)  { return QHeaderView::event(e); }
    int horizontalOffset_pub() const { return horizontalOffset(); }
    int horizontalOffset_pub_parent() const { return QHeaderView::horizontalOffset(); }
    void mouseDoubleClickEvent_pub(QMouseEvent * e)  { mouseDoubleClickEvent(e); }
    void mouseDoubleClickEvent_pub_parent(QMouseEvent * e)  { QHeaderView::mouseDoubleClickEvent(e); }
    void mouseMoveEvent_pub(QMouseEvent * e)  { mouseMoveEvent(e); }
    void mouseMoveEvent_pub_parent(QMouseEvent * e)  { QHeaderView::mouseMoveEvent(e); }
    void mousePressEvent_pub(QMouseEvent * e)  { mousePressEvent(e); }
    void mousePressEvent_pub_parent(QMouseEvent * e)  { QHeaderView::mousePressEvent(e); }
    void mouseReleaseEvent_pub(QMouseEvent * e)  { mouseReleaseEvent(e); }
    void mouseReleaseEvent_pub_parent(QMouseEvent * e)  { QHeaderView::mouseReleaseEvent(e); }
    void paintEvent_pub(QPaintEvent * e)  { paintEvent(e); }
    void paintEvent_pub_parent(QPaintEvent * e)  { QHeaderView::paintEvent(e); }
    void setSelection_pub(const QRect & rect, QItemSelectionModel::SelectionFlags flags)  { setSelection(rect, flags); }
    void setSelection_pub_parent(const QRect & rect, QItemSelectionModel::SelectionFlags flags)  { QHeaderView::setSelection(rect, flags); }
    int verticalOffset_pub() const { return verticalOffset(); }
    int verticalOffset_pub_parent() const { return QHeaderView::verticalOffset(); }
    bool viewportEvent_pub(QEvent * e)  { return viewportEvent(e); }
    bool viewportEvent_pub_parent(QEvent * e)  { return QHeaderView::viewportEvent(e); }
    bool edit_pub(const QModelIndex & index, QAbstractItemView::EditTrigger trigger, QEvent * event)  { return edit(index, trigger, event); }
    bool edit_pub_parent(const QModelIndex & index, QAbstractItemView::EditTrigger trigger, QEvent * event)  { return QHeaderView::edit(index, trigger, event); }
    bool isIndexHidden_pub(const QModelIndex & index) const  { return isIndexHidden(index); }
    bool isIndexHidden_pub_parent(const QModelIndex & index) const  { return QHeaderView::isIndexHidden(index); }
    QModelIndexList selectedIndexes_pub() const { return selectedIndexes(); }
    QModelIndexList selectedIndexes_pub_parent() const { return QHeaderView::selectedIndexes(); }
    QItemSelectionModel::SelectionFlags selectionCommand_pub(const QModelIndex & index, const QEvent * event) const { return selectionCommand(index, event); }
    QItemSelectionModel::SelectionFlags selectionCommand_pub_parent(const QModelIndex & index, const QEvent * event) const { return QHeaderView::selectionCommand(index, event); }
    void startDrag_pub(Qt::DropActions supportedActions)  { startDrag(supportedActions); }
    void startDrag_pub_parent(Qt::DropActions supportedActions)  { QHeaderView::startDrag(supportedActions); }
    QRegion visualRegionForSelection_pub(const QItemSelection & selection) const  { return visualRegionForSelection(selection); }
    QRegion visualRegionForSelection_pub_parent(const QItemSelection & selection) const  { return QHeaderView::visualRegionForSelection(selection); }
    void dragEnterEvent_pub(QDragEnterEvent * event)  { dragEnterEvent(event); }
    void dragEnterEvent_pub_parent(QDragEnterEvent * event)  { QHeaderView::dragEnterEvent(event); }
    void dragLeaveEvent_pub(QDragLeaveEvent * event)  { dragLeaveEvent(event); }
    void dragLeaveEvent_pub_parent(QDragLeaveEvent * event)  { QHeaderView::dragLeaveEvent(event); }
    void dragMoveEvent_pub(QDragMoveEvent * event)  { dragMoveEvent(event); }
    void dragMoveEvent_pub_parent(QDragMoveEvent * event)  { QHeaderView::dragMoveEvent(event); }
    void dropEvent_pub(QDropEvent * event)  { dropEvent(event); }
    void dropEvent_pub_parent(QDropEvent * event)  { QHeaderView::dropEvent(event); }
    bool eventFilter_pub(QObject * object, QEvent * event)  { return eventFilter(object, event); }
    bool eventFilter_pub_parent(QObject * object, QEvent * event)  { return QHeaderView::eventFilter(object, event); }
    void focusInEvent_pub(QFocusEvent * event)  { focusInEvent(event); }
    void focusInEvent_pub_parent(QFocusEvent * event)  { QHeaderView::focusInEvent(event); }
    bool focusNextPrevChild_pub(bool next)  { return focusNextPrevChild(next); }
    bool focusNextPrevChild_pub_parent(bool next)  { return QHeaderView::focusNextPrevChild(next); }
    void focusOutEvent_pub(QFocusEvent * event)  { focusOutEvent(event); }
    void focusOutEvent_pub_parent(QFocusEvent * event)  { QHeaderView::focusOutEvent(event); }
    void keyPressEvent_pub(QKeyEvent * event)  { keyPressEvent(event); }
    void keyPressEvent_pub_parent(QKeyEvent * event)  { QHeaderView::keyPressEvent(event); }
    void resizeEvent_pub(QResizeEvent * event)  { resizeEvent(event); }
    void resizeEvent_pub_parent(QResizeEvent * event)  { QHeaderView::resizeEvent(event); }
    void timerEvent_pub(QTimerEvent * event)  { timerEvent(event); }
    void timerEvent_pub_parent(QTimerEvent * event)  { QHeaderView::timerEvent(event); }
    QSize viewportSizeHint_pub() const { return viewportSizeHint(); }
    QSize viewportSizeHint_pub_parent() const { return QHeaderView::viewportSizeHint(); }
  public:
    const QHeaderViewType* _baseType;
    ClassInstance* _obj;
    const CallEnvironment* _env;
};

inline ClassInstance* QHeaderViewType::cachedInstance(const QHeaderViewType::MuQtType* obj) { return obj->_obj; }

} // Mu

#endif // __MuQt__QHeaderViewType__h__
