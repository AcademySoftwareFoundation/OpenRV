//*****************************************************************************
// Copyright (c) 2024 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************

// IMPORTANT: This file (not the template) is auto-generated by qt2mu.py script.
//            The prefered way to do a fix is to handrolled it or modify the
//            qt2mu.py script. If it is not possible, manual editing is ok but
//            it could be lost in future generations.

#ifndef __MuQt6__QAbstractListModelType__h__
#define __MuQt6__QAbstractListModelType__h__
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

namespace Mu
{
    class MuQt_QAbstractListModel;

    class QAbstractListModelType : public Class
    {
    public:
        typedef MuQt_QAbstractListModel MuQtType;
        typedef QAbstractListModel QtType;

        //
        //  Constructors
        //

        QAbstractListModelType(Context* context, const char* name,
                               Class* superClass = 0, Class* superClass2 = 0);

        virtual ~QAbstractListModelType();

        static bool isInheritable() { return true; }

        static inline ClassInstance* cachedInstance(const MuQtType*);

        //
        //  Class API
        //

        virtual void load();

        MemberFunction* _func[27];
    };

    // Inheritable object

    class MuQt_QAbstractListModel : public QAbstractListModel
    {
    public:
        virtual ~MuQt_QAbstractListModel();
        MuQt_QAbstractListModel(Pointer muobj, const CallEnvironment*,
                                QObject* parent);
        virtual bool dropMimeData(const QMimeData* data, Qt::DropAction action,
                                  int row, int column,
                                  const QModelIndex& parent);
        virtual Qt::ItemFlags flags(const QModelIndex& index) const;
        virtual QModelIndex index(int row, int column,
                                  const QModelIndex& parent) const;
        virtual QModelIndex sibling(int row, int column,
                                    const QModelIndex& idx) const;
        virtual QModelIndex buddy(const QModelIndex& index) const;
        virtual bool canDropMimeData(const QMimeData* data,
                                     Qt::DropAction action, int row, int column,
                                     const QModelIndex& parent) const;
        virtual bool canFetchMore(const QModelIndex& parent) const;
        virtual bool clearItemData(const QModelIndex& index);
        virtual QVariant data(const QModelIndex& index, int role) const;
        virtual void fetchMore(const QModelIndex& parent);
        virtual QVariant headerData(int section, Qt::Orientation orientation,
                                    int role) const;
        virtual bool insertColumns(int column, int count,
                                   const QModelIndex& parent);
        virtual bool insertRows(int row, int count, const QModelIndex& parent);
        virtual QModelIndexList match(const QModelIndex& start, int role,
                                      const QVariant& value, int hits,
                                      Qt::MatchFlags flags) const;
        virtual QMimeData* mimeData(const QModelIndexList& indexes) const;
        virtual QStringList mimeTypes() const;
        virtual bool moveColumns(const QModelIndex& sourceParent,
                                 int sourceColumn, int count,
                                 const QModelIndex& destinationParent,
                                 int destinationChild);
        virtual bool moveRows(const QModelIndex& sourceParent, int sourceRow,
                              int count, const QModelIndex& destinationParent,
                              int destinationChild);
        virtual bool removeColumns(int column, int count,
                                   const QModelIndex& parent);
        virtual bool removeRows(int row, int count, const QModelIndex& parent);
        virtual int rowCount(const QModelIndex& parent) const;
        virtual bool setData(const QModelIndex& index, const QVariant& value,
                             int role);
        virtual bool setHeaderData(int section, Qt::Orientation orientation,
                                   const QVariant& value, int role);
        virtual void sort(int column, Qt::SortOrder order);
        virtual QSize span(const QModelIndex& index) const;
        virtual Qt::DropActions supportedDragActions() const;
        virtual Qt::DropActions supportedDropActions() const;

    public:
        const QAbstractListModelType* _baseType;
        ClassInstance* _obj;
        const CallEnvironment* _env;
    };

    inline ClassInstance* QAbstractListModelType::cachedInstance(
        const QAbstractListModelType::MuQtType* obj)
    {
        return obj->_obj;
    }

} // namespace Mu

#endif // __MuQt__QAbstractListModelType__h__
