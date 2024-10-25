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

#ifndef __MuQt6__QSpinBoxType__h__
#define __MuQt6__QSpinBoxType__h__
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
class MuQt_QSpinBox;

class QSpinBoxType : public Class
{
  public:

    typedef MuQt_QSpinBox MuQtType;
    typedef QSpinBox QtType;

    //
    //  Constructors
    //

    QSpinBoxType(Context* context, 
           const char* name,
           Class* superClass = 0,
           Class* superClass2 = 0);

    virtual ~QSpinBoxType();

    static bool isInheritable() { return true; }
    static inline ClassInstance* cachedInstance(const MuQtType*);

    //
    //  Class API
    //

    virtual void load();

    MemberFunction* _func[24];
};

// Inheritable object

class MuQt_QSpinBox : public QSpinBox
{
  public:
    virtual ~MuQt_QSpinBox();
    MuQt_QSpinBox(Pointer muobj, const CallEnvironment*, QWidget * parent) ;
  protected:
    virtual QString textFromValue(int value) const;
    virtual int valueFromText(const QString & text) const;
    virtual bool event(QEvent * event_) ;
  public:
    virtual void stepBy(int steps) ;
    virtual QVariant inputMethodQuery(Qt::InputMethodQuery query) const;
    virtual QSize minimumSizeHint() const;
    virtual QSize sizeHint() const;
  protected:
    virtual QAbstractSpinBox::StepEnabled stepEnabled() const;
    virtual void changeEvent(QEvent * event) ;
    virtual void closeEvent(QCloseEvent * event) ;
    virtual void contextMenuEvent(QContextMenuEvent * event) ;
    virtual void focusInEvent(QFocusEvent * event) ;
    virtual void focusOutEvent(QFocusEvent * event) ;
    virtual void hideEvent(QHideEvent * event) ;
    virtual void keyPressEvent(QKeyEvent * event) ;
    virtual void keyReleaseEvent(QKeyEvent * event) ;
    virtual void mouseMoveEvent(QMouseEvent * event) ;
    virtual void mousePressEvent(QMouseEvent * event) ;
    virtual void mouseReleaseEvent(QMouseEvent * event) ;
    virtual void paintEvent(QPaintEvent * event) ;
    virtual void resizeEvent(QResizeEvent * event) ;
    virtual void showEvent(QShowEvent * event) ;
    virtual void timerEvent(QTimerEvent * event) ;
    virtual void wheelEvent(QWheelEvent * event) ;
  public:
    QString textFromValue_pub(int value) const { return textFromValue(value); }
    QString textFromValue_pub_parent(int value) const { return QSpinBox::textFromValue(value); }
    int valueFromText_pub(const QString & text) const { return valueFromText(text); }
    int valueFromText_pub_parent(const QString & text) const { return QSpinBox::valueFromText(text); }
    bool event_pub(QEvent * event_)  { return event(event_); }
    bool event_pub_parent(QEvent * event_)  { return QSpinBox::event(event_); }
    QAbstractSpinBox::StepEnabled stepEnabled_pub() const { return stepEnabled(); }
    QAbstractSpinBox::StepEnabled stepEnabled_pub_parent() const { return QSpinBox::stepEnabled(); }
    void changeEvent_pub(QEvent * event)  { changeEvent(event); }
    void changeEvent_pub_parent(QEvent * event)  { QSpinBox::changeEvent(event); }
    void closeEvent_pub(QCloseEvent * event)  { closeEvent(event); }
    void closeEvent_pub_parent(QCloseEvent * event)  { QSpinBox::closeEvent(event); }
    void contextMenuEvent_pub(QContextMenuEvent * event)  { contextMenuEvent(event); }
    void contextMenuEvent_pub_parent(QContextMenuEvent * event)  { QSpinBox::contextMenuEvent(event); }
    void focusInEvent_pub(QFocusEvent * event)  { focusInEvent(event); }
    void focusInEvent_pub_parent(QFocusEvent * event)  { QSpinBox::focusInEvent(event); }
    void focusOutEvent_pub(QFocusEvent * event)  { focusOutEvent(event); }
    void focusOutEvent_pub_parent(QFocusEvent * event)  { QSpinBox::focusOutEvent(event); }
    void hideEvent_pub(QHideEvent * event)  { hideEvent(event); }
    void hideEvent_pub_parent(QHideEvent * event)  { QSpinBox::hideEvent(event); }
    void keyPressEvent_pub(QKeyEvent * event)  { keyPressEvent(event); }
    void keyPressEvent_pub_parent(QKeyEvent * event)  { QSpinBox::keyPressEvent(event); }
    void keyReleaseEvent_pub(QKeyEvent * event)  { keyReleaseEvent(event); }
    void keyReleaseEvent_pub_parent(QKeyEvent * event)  { QSpinBox::keyReleaseEvent(event); }
    void mouseMoveEvent_pub(QMouseEvent * event)  { mouseMoveEvent(event); }
    void mouseMoveEvent_pub_parent(QMouseEvent * event)  { QSpinBox::mouseMoveEvent(event); }
    void mousePressEvent_pub(QMouseEvent * event)  { mousePressEvent(event); }
    void mousePressEvent_pub_parent(QMouseEvent * event)  { QSpinBox::mousePressEvent(event); }
    void mouseReleaseEvent_pub(QMouseEvent * event)  { mouseReleaseEvent(event); }
    void mouseReleaseEvent_pub_parent(QMouseEvent * event)  { QSpinBox::mouseReleaseEvent(event); }
    void paintEvent_pub(QPaintEvent * event)  { paintEvent(event); }
    void paintEvent_pub_parent(QPaintEvent * event)  { QSpinBox::paintEvent(event); }
    void resizeEvent_pub(QResizeEvent * event)  { resizeEvent(event); }
    void resizeEvent_pub_parent(QResizeEvent * event)  { QSpinBox::resizeEvent(event); }
    void showEvent_pub(QShowEvent * event)  { showEvent(event); }
    void showEvent_pub_parent(QShowEvent * event)  { QSpinBox::showEvent(event); }
    void timerEvent_pub(QTimerEvent * event)  { timerEvent(event); }
    void timerEvent_pub_parent(QTimerEvent * event)  { QSpinBox::timerEvent(event); }
    void wheelEvent_pub(QWheelEvent * event)  { wheelEvent(event); }
    void wheelEvent_pub_parent(QWheelEvent * event)  { QSpinBox::wheelEvent(event); }
  public:
    const QSpinBoxType* _baseType;
    ClassInstance* _obj;
    const CallEnvironment* _env;
};

inline ClassInstance* QSpinBoxType::cachedInstance(const QSpinBoxType::MuQtType* obj) { return obj->_obj; }

} // Mu

#endif // __MuQt__QSpinBoxType__h__
