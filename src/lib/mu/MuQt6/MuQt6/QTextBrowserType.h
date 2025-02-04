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

#ifndef __MuQt6__QTextBrowserType__h__
#define __MuQt6__QTextBrowserType__h__
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
    class MuQt_QTextBrowser;

    class QTextBrowserType : public Class
    {
    public:
        typedef MuQt_QTextBrowser MuQtType;
        typedef QTextBrowser QtType;

        //
        //  Constructors
        //

        QTextBrowserType(Context* context, const char* name,
                         Class* superClass = 0, Class* superClass2 = 0);

        virtual ~QTextBrowserType();

        static bool isInheritable() { return true; }

        static inline ClassInstance* cachedInstance(const MuQtType*);

        //
        //  Class API
        //

        virtual void load();

        MemberFunction* _func[27];
    };

    // Inheritable object

    class MuQt_QTextBrowser : public QTextBrowser
    {
    public:
        MuQt_QTextBrowser(Pointer muobj, const CallEnvironment*,
                          QWidget* parent);
        virtual QVariant loadResource(int type, const QUrl& name);

    protected:
        virtual void doSetSource(const QUrl& url,
                                 QTextDocument::ResourceType type);
        virtual bool event(QEvent* e);
        virtual bool focusNextPrevChild(bool next);
        virtual void focusOutEvent(QFocusEvent* ev);
        virtual void keyPressEvent(QKeyEvent* ev);
        virtual void mouseMoveEvent(QMouseEvent* e);
        virtual void mousePressEvent(QMouseEvent* e);
        virtual void mouseReleaseEvent(QMouseEvent* e);
        virtual void paintEvent(QPaintEvent* e);

    public:
        virtual QVariant inputMethodQuery(Qt::InputMethodQuery property) const;

    protected:
        virtual bool canInsertFromMimeData(const QMimeData* source) const;
        virtual QMimeData* createMimeDataFromSelection() const;
        virtual void insertFromMimeData(const QMimeData* source);
        virtual void changeEvent(QEvent* e);
        virtual void contextMenuEvent(QContextMenuEvent* event);
        virtual void dragEnterEvent(QDragEnterEvent* e);
        virtual void dragLeaveEvent(QDragLeaveEvent* e);
        virtual void dragMoveEvent(QDragMoveEvent* e);
        virtual void dropEvent(QDropEvent* e);
        virtual void focusInEvent(QFocusEvent* e);
        virtual void keyReleaseEvent(QKeyEvent* e);
        virtual void mouseDoubleClickEvent(QMouseEvent* e);
        virtual void resizeEvent(QResizeEvent* e);
        virtual void scrollContentsBy(int dx, int dy);
        virtual void showEvent(QShowEvent* _p13);
        virtual void wheelEvent(QWheelEvent* e);

    public:
        void doSetSource_pub(const QUrl& url, QTextDocument::ResourceType type)
        {
            doSetSource(url, type);
        }

        void doSetSource_pub_parent(const QUrl& url,
                                    QTextDocument::ResourceType type)
        {
            QTextBrowser::doSetSource(url, type);
        }

        bool event_pub(QEvent* e) { return event(e); }

        bool event_pub_parent(QEvent* e) { return QTextBrowser::event(e); }

        bool focusNextPrevChild_pub(bool next)
        {
            return focusNextPrevChild(next);
        }

        bool focusNextPrevChild_pub_parent(bool next)
        {
            return QTextBrowser::focusNextPrevChild(next);
        }

        void focusOutEvent_pub(QFocusEvent* ev) { focusOutEvent(ev); }

        void focusOutEvent_pub_parent(QFocusEvent* ev)
        {
            QTextBrowser::focusOutEvent(ev);
        }

        void keyPressEvent_pub(QKeyEvent* ev) { keyPressEvent(ev); }

        void keyPressEvent_pub_parent(QKeyEvent* ev)
        {
            QTextBrowser::keyPressEvent(ev);
        }

        void mouseMoveEvent_pub(QMouseEvent* e) { mouseMoveEvent(e); }

        void mouseMoveEvent_pub_parent(QMouseEvent* e)
        {
            QTextBrowser::mouseMoveEvent(e);
        }

        void mousePressEvent_pub(QMouseEvent* e) { mousePressEvent(e); }

        void mousePressEvent_pub_parent(QMouseEvent* e)
        {
            QTextBrowser::mousePressEvent(e);
        }

        void mouseReleaseEvent_pub(QMouseEvent* e) { mouseReleaseEvent(e); }

        void mouseReleaseEvent_pub_parent(QMouseEvent* e)
        {
            QTextBrowser::mouseReleaseEvent(e);
        }

        void paintEvent_pub(QPaintEvent* e) { paintEvent(e); }

        void paintEvent_pub_parent(QPaintEvent* e)
        {
            QTextBrowser::paintEvent(e);
        }

        bool canInsertFromMimeData_pub(const QMimeData* source) const
        {
            return canInsertFromMimeData(source);
        }

        bool canInsertFromMimeData_pub_parent(const QMimeData* source) const
        {
            return QTextBrowser::canInsertFromMimeData(source);
        }

        QMimeData* createMimeDataFromSelection_pub() const
        {
            return createMimeDataFromSelection();
        }

        QMimeData* createMimeDataFromSelection_pub_parent() const
        {
            return QTextBrowser::createMimeDataFromSelection();
        }

        void insertFromMimeData_pub(const QMimeData* source)
        {
            insertFromMimeData(source);
        }

        void insertFromMimeData_pub_parent(const QMimeData* source)
        {
            QTextBrowser::insertFromMimeData(source);
        }

        void changeEvent_pub(QEvent* e) { changeEvent(e); }

        void changeEvent_pub_parent(QEvent* e) { QTextBrowser::changeEvent(e); }

        void contextMenuEvent_pub(QContextMenuEvent* event)
        {
            contextMenuEvent(event);
        }

        void contextMenuEvent_pub_parent(QContextMenuEvent* event)
        {
            QTextBrowser::contextMenuEvent(event);
        }

        void dragEnterEvent_pub(QDragEnterEvent* e) { dragEnterEvent(e); }

        void dragEnterEvent_pub_parent(QDragEnterEvent* e)
        {
            QTextBrowser::dragEnterEvent(e);
        }

        void dragLeaveEvent_pub(QDragLeaveEvent* e) { dragLeaveEvent(e); }

        void dragLeaveEvent_pub_parent(QDragLeaveEvent* e)
        {
            QTextBrowser::dragLeaveEvent(e);
        }

        void dragMoveEvent_pub(QDragMoveEvent* e) { dragMoveEvent(e); }

        void dragMoveEvent_pub_parent(QDragMoveEvent* e)
        {
            QTextBrowser::dragMoveEvent(e);
        }

        void dropEvent_pub(QDropEvent* e) { dropEvent(e); }

        void dropEvent_pub_parent(QDropEvent* e) { QTextBrowser::dropEvent(e); }

        void focusInEvent_pub(QFocusEvent* e) { focusInEvent(e); }

        void focusInEvent_pub_parent(QFocusEvent* e)
        {
            QTextBrowser::focusInEvent(e);
        }

        void keyReleaseEvent_pub(QKeyEvent* e) { keyReleaseEvent(e); }

        void keyReleaseEvent_pub_parent(QKeyEvent* e)
        {
            QTextBrowser::keyReleaseEvent(e);
        }

        void mouseDoubleClickEvent_pub(QMouseEvent* e)
        {
            mouseDoubleClickEvent(e);
        }

        void mouseDoubleClickEvent_pub_parent(QMouseEvent* e)
        {
            QTextBrowser::mouseDoubleClickEvent(e);
        }

        void resizeEvent_pub(QResizeEvent* e) { resizeEvent(e); }

        void resizeEvent_pub_parent(QResizeEvent* e)
        {
            QTextBrowser::resizeEvent(e);
        }

        void scrollContentsBy_pub(int dx, int dy) { scrollContentsBy(dx, dy); }

        void scrollContentsBy_pub_parent(int dx, int dy)
        {
            QTextBrowser::scrollContentsBy(dx, dy);
        }

        void showEvent_pub(QShowEvent* _p13) { showEvent(_p13); }

        void showEvent_pub_parent(QShowEvent* _p13)
        {
            QTextBrowser::showEvent(_p13);
        }

        void wheelEvent_pub(QWheelEvent* e) { wheelEvent(e); }

        void wheelEvent_pub_parent(QWheelEvent* e)
        {
            QTextBrowser::wheelEvent(e);
        }

    public:
        const QTextBrowserType* _baseType;
        ClassInstance* _obj;
        const CallEnvironment* _env;
    };

    inline ClassInstance*
    QTextBrowserType::cachedInstance(const QTextBrowserType::MuQtType* obj)
    {
        return obj->_obj;
    }

} // namespace Mu

#endif // __MuQt__QTextBrowserType__h__
