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

#ifndef __MuQt6__QAbstractItemModelType__h__
#define __MuQt6__QAbstractItemModelType__h__
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
class MuQt_QAbstractItemModel;

//
//  NOTE: file generated by qt2mu.py
//

class QAbstractItemModelType : public Class
{
  public:

    typedef MuQt_QAbstractItemModel MuQtType;
    typedef QAbstractItemModel QtType;

    //
    //  Constructors
    //

    QAbstractItemModelType(Context* context, 
           const char* name,
           Class* superClass = 0,
           Class* superClass2 = 0);

    virtual ~QAbstractItemModelType();

    static bool isInheritable() { return true; }
    static inline ClassInstance* cachedInstance(const MuQtType*);

    //
    //  Class API
    //

    virtual void load();

    MemberFunction* _func[34];
};

// Inheritable object

class MuQt_QAbstractItemModel : public QAbstractItemModel
{
  public:
    virtual ~MuQt_QAbstractItemModel();
    MuQt_QAbstractItemModel(Pointer muobj, const CallEnvironment*, QObject * parent) ;
    virtual QModelIndex buddy(const QModelIndex & index) const;
    virtual bool canDropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent) const;
    virtual bool canFetchMore(const QModelIndex & parent) const;
    virtual bool clearItemData(const QModelIndex & index) ;
    virtual int columnCount(const QModelIndex & parent) const ;
    virtual QVariant data(const QModelIndex & index, int role) const ;
    virtual bool dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent) ;
    virtual void fetchMore(const QModelIndex & parent) ;
    virtual Qt::ItemFlags flags(const QModelIndex & index) const;
    virtual bool hasChildren(const QModelIndex & parent) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual QModelIndex index(int row, int column, const QModelIndex & parent) const ;
    virtual bool insertColumns(int column, int count, const QModelIndex & parent) ;
    virtual bool insertRows(int row, int count, const QModelIndex & parent) ;
    virtual QModelIndexList match(const QModelIndex & start, int role, const QVariant & value, int hits, Qt::MatchFlags flags) const;
    virtual QMimeData * mimeData(const QModelIndexList & indexes) const;
    virtual QStringList mimeTypes() const;
    virtual bool moveColumns(const QModelIndex & sourceParent, int sourceColumn, int count, const QModelIndex & destinationParent, int destinationChild) ;
    virtual bool moveRows(const QModelIndex & sourceParent, int sourceRow, int count, const QModelIndex & destinationParent, int destinationChild) ;
    virtual QModelIndex parent(const QModelIndex & index) const ;
    virtual bool removeColumns(int column, int count, const QModelIndex & parent) ;
    virtual bool removeRows(int row, int count, const QModelIndex & parent) ;
    virtual int rowCount(const QModelIndex & parent) const ;
    virtual bool setData(const QModelIndex & index, const QVariant & value, int role) ;
    virtual bool setHeaderData(int section, Qt::Orientation orientation, const QVariant & value, int role) ;
    virtual QModelIndex sibling(int row, int column, const QModelIndex & index) const;
    virtual void sort(int column, Qt::SortOrder order) ;
    virtual QSize span(const QModelIndex & index) const;
    virtual Qt::DropActions supportedDragActions() const;
    virtual Qt::DropActions supportedDropActions() const;
    virtual bool event(QEvent * e) ;
    virtual bool eventFilter(QObject * watched, QEvent * event) ;
  protected:
    virtual void customEvent(QEvent * event) ;
    virtual void timerEvent(QTimerEvent * event) ;
  public:
    void beginInsertColumns_pub(const QModelIndex & parent, int first, int last)  { beginInsertColumns(parent, first, last); }
    void beginInsertColumns_pub_parent(const QModelIndex & parent, int first, int last)  { QAbstractItemModel::beginInsertColumns(parent, first, last); }
    void beginInsertRows_pub(const QModelIndex & parent, int first, int last)  { beginInsertRows(parent, first, last); }
    void beginInsertRows_pub_parent(const QModelIndex & parent, int first, int last)  { QAbstractItemModel::beginInsertRows(parent, first, last); }
    bool beginMoveColumns_pub(const QModelIndex & sourceParent, int sourceFirst, int sourceLast, const QModelIndex & destinationParent, int destinationChild)  { return beginMoveColumns(sourceParent, sourceFirst, sourceLast, destinationParent, destinationChild); }
    bool beginMoveColumns_pub_parent(const QModelIndex & sourceParent, int sourceFirst, int sourceLast, const QModelIndex & destinationParent, int destinationChild)  { return QAbstractItemModel::beginMoveColumns(sourceParent, sourceFirst, sourceLast, destinationParent, destinationChild); }
    bool beginMoveRows_pub(const QModelIndex & sourceParent, int sourceFirst, int sourceLast, const QModelIndex & destinationParent, int destinationChild)  { return beginMoveRows(sourceParent, sourceFirst, sourceLast, destinationParent, destinationChild); }
    bool beginMoveRows_pub_parent(const QModelIndex & sourceParent, int sourceFirst, int sourceLast, const QModelIndex & destinationParent, int destinationChild)  { return QAbstractItemModel::beginMoveRows(sourceParent, sourceFirst, sourceLast, destinationParent, destinationChild); }
    void beginRemoveColumns_pub(const QModelIndex & parent, int first, int last)  { beginRemoveColumns(parent, first, last); }
    void beginRemoveColumns_pub_parent(const QModelIndex & parent, int first, int last)  { QAbstractItemModel::beginRemoveColumns(parent, first, last); }
    void beginRemoveRows_pub(const QModelIndex & parent, int first, int last)  { beginRemoveRows(parent, first, last); }
    void beginRemoveRows_pub_parent(const QModelIndex & parent, int first, int last)  { QAbstractItemModel::beginRemoveRows(parent, first, last); }
    void beginResetModel_pub()  { beginResetModel(); }
    void beginResetModel_pub_parent()  { QAbstractItemModel::beginResetModel(); }
    void changePersistentIndex_pub(const QModelIndex & from, const QModelIndex & to)  { changePersistentIndex(from, to); }
    void changePersistentIndex_pub_parent(const QModelIndex & from, const QModelIndex & to)  { QAbstractItemModel::changePersistentIndex(from, to); }
    void changePersistentIndexList_pub(const QModelIndexList & from, const QModelIndexList & to)  { changePersistentIndexList(from, to); }
    void changePersistentIndexList_pub_parent(const QModelIndexList & from, const QModelIndexList & to)  { QAbstractItemModel::changePersistentIndexList(from, to); }
    void endInsertColumns_pub()  { endInsertColumns(); }
    void endInsertColumns_pub_parent()  { QAbstractItemModel::endInsertColumns(); }
    void endInsertRows_pub()  { endInsertRows(); }
    void endInsertRows_pub_parent()  { QAbstractItemModel::endInsertRows(); }
    void endMoveColumns_pub()  { endMoveColumns(); }
    void endMoveColumns_pub_parent()  { QAbstractItemModel::endMoveColumns(); }
    void endMoveRows_pub()  { endMoveRows(); }
    void endMoveRows_pub_parent()  { QAbstractItemModel::endMoveRows(); }
    void endRemoveColumns_pub()  { endRemoveColumns(); }
    void endRemoveColumns_pub_parent()  { QAbstractItemModel::endRemoveColumns(); }
    void endRemoveRows_pub()  { endRemoveRows(); }
    void endRemoveRows_pub_parent()  { QAbstractItemModel::endRemoveRows(); }
    void endResetModel_pub()  { endResetModel(); }
    void endResetModel_pub_parent()  { QAbstractItemModel::endResetModel(); }
    QModelIndexList persistentIndexList_pub() const { return persistentIndexList(); }
    QModelIndexList persistentIndexList_pub_parent() const { return QAbstractItemModel::persistentIndexList(); }
    void customEvent_pub(QEvent * event)  { customEvent(event); }
    void customEvent_pub_parent(QEvent * event)  { QAbstractItemModel::customEvent(event); }
    void timerEvent_pub(QTimerEvent * event)  { timerEvent(event); }
    void timerEvent_pub_parent(QTimerEvent * event)  { QAbstractItemModel::timerEvent(event); }
  public:
    QModelIndex createIndex0_pub(int row, int column, Pointer p) const { return createIndex(row, column, p); }
  public:
    const QAbstractItemModelType* _baseType;
    ClassInstance* _obj;
    const CallEnvironment* _env;
};

inline ClassInstance* QAbstractItemModelType::cachedInstance(const QAbstractItemModelType::MuQtType* obj) { return obj->_obj; }

} // Mu

#endif // __MuQt__QAbstractItemModelType__h__
