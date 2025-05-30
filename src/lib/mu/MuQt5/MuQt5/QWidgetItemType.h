//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __MuQt5__QWidgetItemType__h__
#define __MuQt5__QWidgetItemType__h__
#include <iostream>
#include <Mu/Class.h>
#include <Mu/MuProcess.h>
#include <QtCore/QtCore>
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>
#include <QtNetwork/QtNetwork>
#include <QtSvg/QtSvg>
#include <MuQt5/Bridge.h>

namespace Mu
{
    class MuQt_QWidgetItem;

    //
    //  NOTE: file generated by qt2mu.py
    //

    class QWidgetItemType : public Class
    {
    public:
        typedef MuQt_QWidgetItem MuQtType;
        typedef QWidgetItem QtType;

        //
        //  Constructors
        //

        QWidgetItemType(Context* context, const char* name,
                        Class* superClass = 0);
        virtual ~QWidgetItemType();

        static bool isInheritable() { return true; }

        static inline ClassInstance* cachedInstance(const MuQtType*);

        //
        //  Class API
        //

        virtual void load();

        MemberFunction* _func[14];
    };

    // Inheritable object

    class MuQt_QWidgetItem : public QWidgetItem
    {
    public:
        virtual ~MuQt_QWidgetItem();
        MuQt_QWidgetItem(Pointer muobj, const CallEnvironment*,
                         QWidget* widget);
        virtual QSizePolicy::ControlTypes controlTypes() const;
        virtual Qt::Orientations expandingDirections() const;
        virtual QRect geometry() const;
        virtual bool hasHeightForWidth() const;
        virtual int heightForWidth(int w) const;
        virtual bool isEmpty() const;
        virtual QSize maximumSize() const;
        virtual QSize minimumSize() const;
        virtual void setGeometry(const QRect& rect);
        virtual QSize sizeHint() const;
        virtual QWidget* widget();
        virtual void invalidate();
        virtual QLayout* layout();
        virtual int minimumHeightForWidth(int w) const;

    public:
        const QWidgetItemType* _baseType;
        ClassInstance* _obj;
        const CallEnvironment* _env;
    };

    inline ClassInstance*
    QWidgetItemType::cachedInstance(const QWidgetItemType::MuQtType* obj)
    {
        return obj->_obj;
    }

} // namespace Mu

#endif // __MuQt5__QWidgetItemType__h__
