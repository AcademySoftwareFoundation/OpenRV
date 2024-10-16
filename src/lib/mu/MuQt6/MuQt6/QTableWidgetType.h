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

#ifndef __MuQt6__QTableWidgetType__h__
#define __MuQt6__QTableWidgetType__h__
#include <iostream>
#include <Mu/Class.h>
#include <Mu/MuProcess.h>
#include <QtCore/QtCore>
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>
#include <QtNetwork/QtNetwork>
#include <QtWebEngineWidgets/QtWebEngineWidgets>
#include <QtQml/QtQml>
#include <QtQuick/QtQuick>
#include <QtQuickWidgets/QtQuickWidgets>
#include <QtSvg/QtSvg>
#include <QSvgWidget>
#include <MuQt6/Bridge.h>

namespace Mu {
class MuQt_QTableWidget;

class QTableWidgetType : public Class
{
  public:

    typedef MuQt_QTableWidget MuQtType;
    typedef QTableWidget QtType;

    //
    //  Constructors
    //

    QTableWidgetType(Context* context, 
           const char* name,
           Class* superClass = 0,
           Class* superClass2 = 0);

    virtual ~QTableWidgetType();

    static bool isInheritable() { return true; }
    static inline ClassInstance* cachedInstance(const MuQtType*);

    //
    //  Class API
    //

    virtual void load();

    MemberFunction* _func[26];
};

// Inheritable object

class MuQt_QTableWidget : public QTableWidget
{
  public:
    virtual ~MuQt_QTableWidget();
    MuQt_QTableWidget(Pointer muobj, const CallEnvironment*, QWidget * parent) ;
    MuQt_QTableWidget(Pointer muobj, const CallEnvironment*, int rows, int columns, QWidget * parent) ;
  protected:
    virtual bool dropMimeData(int row, int column, const QMimeData * data, Qt::DropAction action) ;
    virtual QMimeData * mimeData(const QList<QTableWidgetItem * > & items) const;
    virtual QStringList mimeTypes() const;
    virtual Qt::DropActions supportedDropActions() const;
    virtual void dropEvent(QDropEvent * event) ;
    virtual bool event(QEvent * e) ;
  public:
    virtual QModelIndex indexAt(const QPoint & pos) const;
    virtual void scrollTo(const QModelIndex & index, QAbstractItemView::ScrollHint hint) ;
    virtual void setRootIndex(const QModelIndex & index) ;
    virtual void setSelectionModel(QItemSelectionModel * selectionModel) ;
    virtual QRect visualRect(const QModelIndex & index) const;
  protected:
    virtual void currentChanged(const QModelIndex & current, const QModelIndex & previous) ;
    virtual int horizontalOffset() const;
    virtual bool isIndexHidden(const QModelIndex & index) const;
    virtual void paintEvent(QPaintEvent * event) ;
    virtual void scrollContentsBy(int dx, int dy) ;
    virtual QModelIndexList selectedIndexes() const;
    virtual void selectionChanged(const QItemSelection & selected, const QItemSelection & deselected) ;
    virtual void setSelection(const QRect & rect, QItemSelectionModel::SelectionFlags flags) ;
    virtual int sizeHintForColumn(int column) const;
    virtual int sizeHintForRow(int row) const;
    virtual void timerEvent(QTimerEvent * event) ;
    virtual void updateGeometries() ;
    virtual int verticalOffset() const;
    virtual QSize viewportSizeHint() const;
    virtual QRegion visualRegionForSelection(const QItemSelection & selection) const;
  public:
    bool dropMimeData_pub(int row, int column, const QMimeData * data, Qt::DropAction action)  { return dropMimeData(row, column, data, action); }
    bool dropMimeData_pub_parent(int row, int column, const QMimeData * data, Qt::DropAction action)  { return QTableWidget::dropMimeData(row, column, data, action); }
    QMimeData * mimeData_pub(const QList<QTableWidgetItem * > & items) const { return mimeData(items); }
    QMimeData * mimeData_pub_parent(const QList<QTableWidgetItem * > & items) const { return QTableWidget::mimeData(items); }
    QStringList mimeTypes_pub() const { return mimeTypes(); }
    QStringList mimeTypes_pub_parent() const { return QTableWidget::mimeTypes(); }
    Qt::DropActions supportedDropActions_pub() const { return supportedDropActions(); }
    Qt::DropActions supportedDropActions_pub_parent() const { return QTableWidget::supportedDropActions(); }
    void dropEvent_pub(QDropEvent * event)  { dropEvent(event); }
    void dropEvent_pub_parent(QDropEvent * event)  { QTableWidget::dropEvent(event); }
    bool event_pub(QEvent * e)  { return event(e); }
    bool event_pub_parent(QEvent * e)  { return QTableWidget::event(e); }
    void currentChanged_pub(const QModelIndex & current, const QModelIndex & previous)  { currentChanged(current, previous); }
    void currentChanged_pub_parent(const QModelIndex & current, const QModelIndex & previous)  { QTableWidget::currentChanged(current, previous); }
    int horizontalOffset_pub() const { return horizontalOffset(); }
    int horizontalOffset_pub_parent() const { return QTableWidget::horizontalOffset(); }
    bool isIndexHidden_pub(const QModelIndex & index) const { return isIndexHidden(index); }
    bool isIndexHidden_pub_parent(const QModelIndex & index) const { return QTableWidget::isIndexHidden(index); }
    void paintEvent_pub(QPaintEvent * event)  { paintEvent(event); }
    void paintEvent_pub_parent(QPaintEvent * event)  { QTableWidget::paintEvent(event); }
    void scrollContentsBy_pub(int dx, int dy)  { scrollContentsBy(dx, dy); }
    void scrollContentsBy_pub_parent(int dx, int dy)  { QTableWidget::scrollContentsBy(dx, dy); }
    QModelIndexList selectedIndexes_pub() const { return selectedIndexes(); }
    QModelIndexList selectedIndexes_pub_parent() const { return QTableWidget::selectedIndexes(); }
    void selectionChanged_pub(const QItemSelection & selected, const QItemSelection & deselected)  { selectionChanged(selected, deselected); }
    void selectionChanged_pub_parent(const QItemSelection & selected, const QItemSelection & deselected)  { QTableWidget::selectionChanged(selected, deselected); }
    void setSelection_pub(const QRect & rect, QItemSelectionModel::SelectionFlags flags)  { setSelection(rect, flags); }
    void setSelection_pub_parent(const QRect & rect, QItemSelectionModel::SelectionFlags flags)  { QTableWidget::setSelection(rect, flags); }
    int sizeHintForColumn_pub(int column) const { return sizeHintForColumn(column); }
    int sizeHintForColumn_pub_parent(int column) const { return QTableWidget::sizeHintForColumn(column); }
    int sizeHintForRow_pub(int row) const { return sizeHintForRow(row); }
    int sizeHintForRow_pub_parent(int row) const { return QTableWidget::sizeHintForRow(row); }
    void timerEvent_pub(QTimerEvent * event)  { timerEvent(event); }
    void timerEvent_pub_parent(QTimerEvent * event)  { QTableWidget::timerEvent(event); }
    void updateGeometries_pub()  { updateGeometries(); }
    void updateGeometries_pub_parent()  { QTableWidget::updateGeometries(); }
    int verticalOffset_pub() const { return verticalOffset(); }
    int verticalOffset_pub_parent() const { return QTableWidget::verticalOffset(); }
    QSize viewportSizeHint_pub() const { return viewportSizeHint(); }
    QSize viewportSizeHint_pub_parent() const { return QTableWidget::viewportSizeHint(); }
    QRegion visualRegionForSelection_pub(const QItemSelection & selection) const { return visualRegionForSelection(selection); }
    QRegion visualRegionForSelection_pub_parent(const QItemSelection & selection) const { return QTableWidget::visualRegionForSelection(selection); }
  public:
    const QTableWidgetType* _baseType;
    ClassInstance* _obj;
    const CallEnvironment* _env;
};

inline ClassInstance* QTableWidgetType::cachedInstance(const QTableWidgetType::MuQtType* obj) { return obj->_obj; }

} // Mu

#endif // __MuQt__QTableWidgetType__h__
