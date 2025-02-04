//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __MuQt5__QMainWindowType__h__
#define __MuQt5__QMainWindowType__h__
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
    class MuQt_QMainWindow;

    //
    //  NOTE: file generated by qt2mu.py
    //

    class QMainWindowType : public Class
    {
    public:
        typedef MuQt_QMainWindow MuQtType;
        typedef QMainWindow QtType;

        //
        //  Constructors
        //

        QMainWindowType(Context* context, const char* name,
                        Class* superClass = 0, Class* superClass2 = 0);

        virtual ~QMainWindowType();

        static bool isInheritable() { return true; }

        static inline ClassInstance* cachedInstance(const MuQtType*);

        //
        //  Class API
        //

        virtual void load();

        MemberFunction* _func[33];
    };

    // Inheritable object

    class MuQt_QMainWindow : public QMainWindow
    {
    public:
        virtual ~MuQt_QMainWindow();
        MuQt_QMainWindow(Pointer muobj, const CallEnvironment*, QWidget* parent,
                         Qt::WindowFlags flags);
        virtual QMenu* createPopupMenu();

    protected:
        virtual void contextMenuEvent(QContextMenuEvent* event);
        virtual bool event(QEvent* event_);

    public:
        virtual bool hasHeightForWidth() const;
        virtual int heightForWidth(int w) const;
        virtual QVariant inputMethodQuery(Qt::InputMethodQuery query) const;
        virtual QSize minimumSizeHint() const;
        virtual QSize sizeHint() const;

    protected:
        virtual void changeEvent(QEvent* event);
        virtual void closeEvent(QCloseEvent* event);
        virtual void dragEnterEvent(QDragEnterEvent* event);
        virtual void dragLeaveEvent(QDragLeaveEvent* event);
        virtual void dragMoveEvent(QDragMoveEvent* event);
        virtual void dropEvent(QDropEvent* event);
        virtual void enterEvent(QEvent* event);
        virtual void focusInEvent(QFocusEvent* event);
        virtual bool focusNextPrevChild(bool next);
        virtual void focusOutEvent(QFocusEvent* event);
        virtual void hideEvent(QHideEvent* event);
        virtual void keyPressEvent(QKeyEvent* event);
        virtual void keyReleaseEvent(QKeyEvent* event);
        virtual void leaveEvent(QEvent* event);
        virtual void mouseDoubleClickEvent(QMouseEvent* event);
        virtual void mouseMoveEvent(QMouseEvent* event);
        virtual void mousePressEvent(QMouseEvent* event);
        virtual void mouseReleaseEvent(QMouseEvent* event);
        virtual void moveEvent(QMoveEvent* event);
        virtual void paintEvent(QPaintEvent* event);
        virtual void resizeEvent(QResizeEvent* event);
        virtual void showEvent(QShowEvent* event);
        virtual void tabletEvent(QTabletEvent* event);
        virtual void wheelEvent(QWheelEvent* event);
        virtual int metric(PaintDeviceMetric m) const;

    public:
        void contextMenuEvent_pub(QContextMenuEvent* event)
        {
            contextMenuEvent(event);
        }

        void contextMenuEvent_pub_parent(QContextMenuEvent* event)
        {
            QMainWindow::contextMenuEvent(event);
        }

        bool event_pub(QEvent* event_) { return event(event_); }

        bool event_pub_parent(QEvent* event_)
        {
            return QMainWindow::event(event_);
        }

        void changeEvent_pub(QEvent* event) { changeEvent(event); }

        void changeEvent_pub_parent(QEvent* event)
        {
            QMainWindow::changeEvent(event);
        }

        void closeEvent_pub(QCloseEvent* event) { closeEvent(event); }

        void closeEvent_pub_parent(QCloseEvent* event)
        {
            QMainWindow::closeEvent(event);
        }

        void dragEnterEvent_pub(QDragEnterEvent* event)
        {
            dragEnterEvent(event);
        }

        void dragEnterEvent_pub_parent(QDragEnterEvent* event)
        {
            QMainWindow::dragEnterEvent(event);
        }

        void dragLeaveEvent_pub(QDragLeaveEvent* event)
        {
            dragLeaveEvent(event);
        }

        void dragLeaveEvent_pub_parent(QDragLeaveEvent* event)
        {
            QMainWindow::dragLeaveEvent(event);
        }

        void dragMoveEvent_pub(QDragMoveEvent* event) { dragMoveEvent(event); }

        void dragMoveEvent_pub_parent(QDragMoveEvent* event)
        {
            QMainWindow::dragMoveEvent(event);
        }

        void dropEvent_pub(QDropEvent* event) { dropEvent(event); }

        void dropEvent_pub_parent(QDropEvent* event)
        {
            QMainWindow::dropEvent(event);
        }

        void enterEvent_pub(QEvent* event) { enterEvent(event); }

        void enterEvent_pub_parent(QEvent* event)
        {
            QMainWindow::enterEvent(event);
        }

        void focusInEvent_pub(QFocusEvent* event) { focusInEvent(event); }

        void focusInEvent_pub_parent(QFocusEvent* event)
        {
            QMainWindow::focusInEvent(event);
        }

        bool focusNextPrevChild_pub(bool next)
        {
            return focusNextPrevChild(next);
        }

        bool focusNextPrevChild_pub_parent(bool next)
        {
            return QMainWindow::focusNextPrevChild(next);
        }

        void focusOutEvent_pub(QFocusEvent* event) { focusOutEvent(event); }

        void focusOutEvent_pub_parent(QFocusEvent* event)
        {
            QMainWindow::focusOutEvent(event);
        }

        void hideEvent_pub(QHideEvent* event) { hideEvent(event); }

        void hideEvent_pub_parent(QHideEvent* event)
        {
            QMainWindow::hideEvent(event);
        }

        void keyPressEvent_pub(QKeyEvent* event) { keyPressEvent(event); }

        void keyPressEvent_pub_parent(QKeyEvent* event)
        {
            QMainWindow::keyPressEvent(event);
        }

        void keyReleaseEvent_pub(QKeyEvent* event) { keyReleaseEvent(event); }

        void keyReleaseEvent_pub_parent(QKeyEvent* event)
        {
            QMainWindow::keyReleaseEvent(event);
        }

        void leaveEvent_pub(QEvent* event) { leaveEvent(event); }

        void leaveEvent_pub_parent(QEvent* event)
        {
            QMainWindow::leaveEvent(event);
        }

        void mouseDoubleClickEvent_pub(QMouseEvent* event)
        {
            mouseDoubleClickEvent(event);
        }

        void mouseDoubleClickEvent_pub_parent(QMouseEvent* event)
        {
            QMainWindow::mouseDoubleClickEvent(event);
        }

        void mouseMoveEvent_pub(QMouseEvent* event) { mouseMoveEvent(event); }

        void mouseMoveEvent_pub_parent(QMouseEvent* event)
        {
            QMainWindow::mouseMoveEvent(event);
        }

        void mousePressEvent_pub(QMouseEvent* event) { mousePressEvent(event); }

        void mousePressEvent_pub_parent(QMouseEvent* event)
        {
            QMainWindow::mousePressEvent(event);
        }

        void mouseReleaseEvent_pub(QMouseEvent* event)
        {
            mouseReleaseEvent(event);
        }

        void mouseReleaseEvent_pub_parent(QMouseEvent* event)
        {
            QMainWindow::mouseReleaseEvent(event);
        }

        void moveEvent_pub(QMoveEvent* event) { moveEvent(event); }

        void moveEvent_pub_parent(QMoveEvent* event)
        {
            QMainWindow::moveEvent(event);
        }

        void paintEvent_pub(QPaintEvent* event) { paintEvent(event); }

        void paintEvent_pub_parent(QPaintEvent* event)
        {
            QMainWindow::paintEvent(event);
        }

        void resizeEvent_pub(QResizeEvent* event) { resizeEvent(event); }

        void resizeEvent_pub_parent(QResizeEvent* event)
        {
            QMainWindow::resizeEvent(event);
        }

        void showEvent_pub(QShowEvent* event) { showEvent(event); }

        void showEvent_pub_parent(QShowEvent* event)
        {
            QMainWindow::showEvent(event);
        }

        void tabletEvent_pub(QTabletEvent* event) { tabletEvent(event); }

        void tabletEvent_pub_parent(QTabletEvent* event)
        {
            QMainWindow::tabletEvent(event);
        }

        void wheelEvent_pub(QWheelEvent* event) { wheelEvent(event); }

        void wheelEvent_pub_parent(QWheelEvent* event)
        {
            QMainWindow::wheelEvent(event);
        }

        int metric_pub(PaintDeviceMetric m) const { return metric(m); }

        int metric_pub_parent(PaintDeviceMetric m) const
        {
            return QMainWindow::metric(m);
        }

    public:
        const QMainWindowType* _baseType;
        ClassInstance* _obj;
        const CallEnvironment* _env;
    };

    inline ClassInstance*
    QMainWindowType::cachedInstance(const QMainWindowType::MuQtType* obj)
    {
        return obj->_obj;
    }

} // namespace Mu

#endif // __MuQt__QMainWindowType__h__
