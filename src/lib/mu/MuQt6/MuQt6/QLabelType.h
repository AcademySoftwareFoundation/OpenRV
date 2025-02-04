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

#ifndef __MuQt6__QLabelType__h__
#define __MuQt6__QLabelType__h__
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
    class MuQt_QLabel;

    class QLabelType : public Class
    {
    public:
        typedef MuQt_QLabel MuQtType;
        typedef QLabel QtType;

        //
        //  Constructors
        //

        QLabelType(Context* context, const char* name, Class* superClass = 0,
                   Class* superClass2 = 0);

        virtual ~QLabelType();

        static bool isInheritable() { return true; }

        static inline ClassInstance* cachedInstance(const MuQtType*);

        //
        //  Class API
        //

        virtual void load();

        MemberFunction* _func[14];
    };

    // Inheritable object

    class MuQt_QLabel : public QLabel
    {
    public:
        virtual ~MuQt_QLabel();
        MuQt_QLabel(Pointer muobj, const CallEnvironment*, QWidget* parent,
                    Qt::WindowFlags f);
        MuQt_QLabel(Pointer muobj, const CallEnvironment*, const QString& text,
                    QWidget* parent, Qt::WindowFlags f);
        virtual int heightForWidth(int w) const;
        virtual QSize minimumSizeHint() const;
        virtual QSize sizeHint() const;

    protected:
        virtual void changeEvent(QEvent* ev);
        virtual void contextMenuEvent(QContextMenuEvent* ev);
        virtual bool event(QEvent* e);
        virtual void focusInEvent(QFocusEvent* ev);
        virtual bool focusNextPrevChild(bool next);
        virtual void focusOutEvent(QFocusEvent* ev);
        virtual void keyPressEvent(QKeyEvent* ev);
        virtual void mouseMoveEvent(QMouseEvent* ev);
        virtual void mousePressEvent(QMouseEvent* ev);
        virtual void mouseReleaseEvent(QMouseEvent* ev);
        virtual void paintEvent(QPaintEvent* _p14);

    public:
        void changeEvent_pub(QEvent* ev) { changeEvent(ev); }

        void changeEvent_pub_parent(QEvent* ev) { QLabel::changeEvent(ev); }

        void contextMenuEvent_pub(QContextMenuEvent* ev)
        {
            contextMenuEvent(ev);
        }

        void contextMenuEvent_pub_parent(QContextMenuEvent* ev)
        {
            QLabel::contextMenuEvent(ev);
        }

        bool event_pub(QEvent* e) { return event(e); }

        bool event_pub_parent(QEvent* e) { return QLabel::event(e); }

        void focusInEvent_pub(QFocusEvent* ev) { focusInEvent(ev); }

        void focusInEvent_pub_parent(QFocusEvent* ev)
        {
            QLabel::focusInEvent(ev);
        }

        bool focusNextPrevChild_pub(bool next)
        {
            return focusNextPrevChild(next);
        }

        bool focusNextPrevChild_pub_parent(bool next)
        {
            return QLabel::focusNextPrevChild(next);
        }

        void focusOutEvent_pub(QFocusEvent* ev) { focusOutEvent(ev); }

        void focusOutEvent_pub_parent(QFocusEvent* ev)
        {
            QLabel::focusOutEvent(ev);
        }

        void keyPressEvent_pub(QKeyEvent* ev) { keyPressEvent(ev); }

        void keyPressEvent_pub_parent(QKeyEvent* ev)
        {
            QLabel::keyPressEvent(ev);
        }

        void mouseMoveEvent_pub(QMouseEvent* ev) { mouseMoveEvent(ev); }

        void mouseMoveEvent_pub_parent(QMouseEvent* ev)
        {
            QLabel::mouseMoveEvent(ev);
        }

        void mousePressEvent_pub(QMouseEvent* ev) { mousePressEvent(ev); }

        void mousePressEvent_pub_parent(QMouseEvent* ev)
        {
            QLabel::mousePressEvent(ev);
        }

        void mouseReleaseEvent_pub(QMouseEvent* ev) { mouseReleaseEvent(ev); }

        void mouseReleaseEvent_pub_parent(QMouseEvent* ev)
        {
            QLabel::mouseReleaseEvent(ev);
        }

        void paintEvent_pub(QPaintEvent* _p14) { paintEvent(_p14); }

        void paintEvent_pub_parent(QPaintEvent* _p14)
        {
            QLabel::paintEvent(_p14);
        }

    public:
        const QLabelType* _baseType;
        ClassInstance* _obj;
        const CallEnvironment* _env;
    };

    inline ClassInstance*
    QLabelType::cachedInstance(const QLabelType::MuQtType* obj)
    {
        return obj->_obj;
    }

} // namespace Mu

#endif // __MuQt__QLabelType__h__
