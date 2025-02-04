//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __MuQt5__QTextEditType__h__
#define __MuQt5__QTextEditType__h__
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
    class MuQt_QTextEdit;

    //
    //  NOTE: file generated by qt2mu.py
    //

    class QTextEditType : public Class
    {
    public:
        typedef MuQt_QTextEdit MuQtType;
        typedef QTextEdit QtType;

        //
        //  Constructors
        //

        QTextEditType(Context* context, const char* name, Class* superClass = 0,
                      Class* superClass2 = 0);

        virtual ~QTextEditType();

        static bool isInheritable() { return true; }

        static inline ClassInstance* cachedInstance(const MuQtType*);

        //
        //  Class API
        //

        virtual void load();

        MemberFunction* _func[31];
    };

    // Inheritable object

    class MuQt_QTextEdit : public QTextEdit
    {
    public:
        virtual ~MuQt_QTextEdit();
        MuQt_QTextEdit(Pointer muobj, const CallEnvironment*, QWidget* parent);
        MuQt_QTextEdit(Pointer muobj, const CallEnvironment*,
                       const QString& text, QWidget* parent);
        virtual QVariant loadResource(int type, const QUrl& name);
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
        virtual bool focusNextPrevChild(bool next);
        virtual void focusOutEvent(QFocusEvent* e);
        virtual void keyPressEvent(QKeyEvent* e);
        virtual void keyReleaseEvent(QKeyEvent* e);
        virtual void mouseDoubleClickEvent(QMouseEvent* e);
        virtual void mouseMoveEvent(QMouseEvent* e);
        virtual void mousePressEvent(QMouseEvent* e);
        virtual void mouseReleaseEvent(QMouseEvent* e);
        virtual void paintEvent(QPaintEvent* event);
        virtual void resizeEvent(QResizeEvent* e);
        virtual void scrollContentsBy(int dx, int dy);
        virtual void showEvent(QShowEvent* _p13);
        virtual void wheelEvent(QWheelEvent* e);

    public:
        virtual void setupViewport(QWidget* viewport);
        virtual QSize minimumSizeHint() const;
        virtual QSize sizeHint() const;

    protected:
        virtual bool viewportEvent(QEvent* event);
        virtual QSize viewportSizeHint() const;
        virtual bool event(QEvent* event_);

    public:
        bool canInsertFromMimeData_pub(const QMimeData* source) const
        {
            return canInsertFromMimeData(source);
        }

        bool canInsertFromMimeData_pub_parent(const QMimeData* source) const
        {
            return QTextEdit::canInsertFromMimeData(source);
        }

        QMimeData* createMimeDataFromSelection_pub() const
        {
            return createMimeDataFromSelection();
        }

        QMimeData* createMimeDataFromSelection_pub_parent() const
        {
            return QTextEdit::createMimeDataFromSelection();
        }

        void insertFromMimeData_pub(const QMimeData* source)
        {
            insertFromMimeData(source);
        }

        void insertFromMimeData_pub_parent(const QMimeData* source)
        {
            QTextEdit::insertFromMimeData(source);
        }

        void changeEvent_pub(QEvent* e) { changeEvent(e); }

        void changeEvent_pub_parent(QEvent* e) { QTextEdit::changeEvent(e); }

        void contextMenuEvent_pub(QContextMenuEvent* event)
        {
            contextMenuEvent(event);
        }

        void contextMenuEvent_pub_parent(QContextMenuEvent* event)
        {
            QTextEdit::contextMenuEvent(event);
        }

        void dragEnterEvent_pub(QDragEnterEvent* e) { dragEnterEvent(e); }

        void dragEnterEvent_pub_parent(QDragEnterEvent* e)
        {
            QTextEdit::dragEnterEvent(e);
        }

        void dragLeaveEvent_pub(QDragLeaveEvent* e) { dragLeaveEvent(e); }

        void dragLeaveEvent_pub_parent(QDragLeaveEvent* e)
        {
            QTextEdit::dragLeaveEvent(e);
        }

        void dragMoveEvent_pub(QDragMoveEvent* e) { dragMoveEvent(e); }

        void dragMoveEvent_pub_parent(QDragMoveEvent* e)
        {
            QTextEdit::dragMoveEvent(e);
        }

        void dropEvent_pub(QDropEvent* e) { dropEvent(e); }

        void dropEvent_pub_parent(QDropEvent* e) { QTextEdit::dropEvent(e); }

        void focusInEvent_pub(QFocusEvent* e) { focusInEvent(e); }

        void focusInEvent_pub_parent(QFocusEvent* e)
        {
            QTextEdit::focusInEvent(e);
        }

        bool focusNextPrevChild_pub(bool next)
        {
            return focusNextPrevChild(next);
        }

        bool focusNextPrevChild_pub_parent(bool next)
        {
            return QTextEdit::focusNextPrevChild(next);
        }

        void focusOutEvent_pub(QFocusEvent* e) { focusOutEvent(e); }

        void focusOutEvent_pub_parent(QFocusEvent* e)
        {
            QTextEdit::focusOutEvent(e);
        }

        void keyPressEvent_pub(QKeyEvent* e) { keyPressEvent(e); }

        void keyPressEvent_pub_parent(QKeyEvent* e)
        {
            QTextEdit::keyPressEvent(e);
        }

        void keyReleaseEvent_pub(QKeyEvent* e) { keyReleaseEvent(e); }

        void keyReleaseEvent_pub_parent(QKeyEvent* e)
        {
            QTextEdit::keyReleaseEvent(e);
        }

        void mouseDoubleClickEvent_pub(QMouseEvent* e)
        {
            mouseDoubleClickEvent(e);
        }

        void mouseDoubleClickEvent_pub_parent(QMouseEvent* e)
        {
            QTextEdit::mouseDoubleClickEvent(e);
        }

        void mouseMoveEvent_pub(QMouseEvent* e) { mouseMoveEvent(e); }

        void mouseMoveEvent_pub_parent(QMouseEvent* e)
        {
            QTextEdit::mouseMoveEvent(e);
        }

        void mousePressEvent_pub(QMouseEvent* e) { mousePressEvent(e); }

        void mousePressEvent_pub_parent(QMouseEvent* e)
        {
            QTextEdit::mousePressEvent(e);
        }

        void mouseReleaseEvent_pub(QMouseEvent* e) { mouseReleaseEvent(e); }

        void mouseReleaseEvent_pub_parent(QMouseEvent* e)
        {
            QTextEdit::mouseReleaseEvent(e);
        }

        void paintEvent_pub(QPaintEvent* event) { paintEvent(event); }

        void paintEvent_pub_parent(QPaintEvent* event)
        {
            QTextEdit::paintEvent(event);
        }

        void resizeEvent_pub(QResizeEvent* e) { resizeEvent(e); }

        void resizeEvent_pub_parent(QResizeEvent* e)
        {
            QTextEdit::resizeEvent(e);
        }

        void scrollContentsBy_pub(int dx, int dy) { scrollContentsBy(dx, dy); }

        void scrollContentsBy_pub_parent(int dx, int dy)
        {
            QTextEdit::scrollContentsBy(dx, dy);
        }

        void showEvent_pub(QShowEvent* _p13) { showEvent(_p13); }

        void showEvent_pub_parent(QShowEvent* _p13)
        {
            QTextEdit::showEvent(_p13);
        }

        void wheelEvent_pub(QWheelEvent* e) { wheelEvent(e); }

        void wheelEvent_pub_parent(QWheelEvent* e) { QTextEdit::wheelEvent(e); }

        bool viewportEvent_pub(QEvent* event) { return viewportEvent(event); }

        bool viewportEvent_pub_parent(QEvent* event)
        {
            return QTextEdit::viewportEvent(event);
        }

        QSize viewportSizeHint_pub() const { return viewportSizeHint(); }

        QSize viewportSizeHint_pub_parent() const
        {
            return QTextEdit::viewportSizeHint();
        }

        bool event_pub(QEvent* event_) { return event(event_); }

        bool event_pub_parent(QEvent* event_)
        {
            return QTextEdit::event(event_);
        }

    public:
        const QTextEditType* _baseType;
        ClassInstance* _obj;
        const CallEnvironment* _env;
    };

    inline ClassInstance*
    QTextEditType::cachedInstance(const QTextEditType::MuQtType* obj)
    {
        return obj->_obj;
    }

} // namespace Mu

#endif // __MuQt__QTextEditType__h__
