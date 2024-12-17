//******************************************************************************
// Copyright (c) 2004 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __mu_usf__QTTranslator__h__
#define __mu_usf__QTTranslator__h__
#include <TwkApp/EventNode.h>
#include <TwkApp/VideoDevice.h>
#include <string>
#include <QtCore/QEvent>
#include <QtWidgets/QWidget>
#include <QtGui/QInputEvent>

namespace Rv
{

    //
    //  A TwkApp::Event Translator for a given Fl_Widget
    //

    class QTTranslator
    {
    public:
        QTTranslator(TwkApp::EventNode*, QWidget* w = 0);

        void setWidet(QWidget* w) { m_widget = w; }

        void sendEvent(const TwkApp::Event&) const;

        //
        //  Here's the QT entry point
        //

        bool sendQTEvent(QEvent* event, float activationTime = 0.0) const;

        bool sendTabletEvent(QEvent* event) const;
        bool sendKeyEvent(QEvent* event) const;
        bool sendMouseEvent(QEvent* event, float activationTime = 0.0) const;
        bool sendMouseWheelEvent(QEvent* event) const;
        bool sendEnterLeaveEvent(QEvent* event) const;
        bool sendDNDEvent(QEvent* event) const;
        unsigned int modifiers(QInputEvent* event) const;

        void resetModifiers() const;
        unsigned int modifiers(Qt::KeyboardModifiers) const;
        unsigned int buttons() const;
        Qt::MouseButtons qbuttons(unsigned int) const;
        std::string modifierString(Qt::KeyboardModifiers, bool) const;
        int key(int) const;
        int pointer(int) const;
        std::string pointerString(bool, bool, bool, QEvent::Type,
                                  Qt::KeyboardModifiers, Qt::MouseButtons,
                                  bool) const;

        void setRelativeDomain(float w, float h) const;
        void setScaleAndOffset(float x, float y, float sx, float sy) const;

        Qt::KeyboardModifiers currentModifiers() const { return m_modifiers; };

        void setCurrentModifiers(Qt::KeyboardModifiers m) const
        {
            m_modifiers = m;
        };

    private:
        TwkApp::EventNode* m_node;
        QWidget* m_widget;
        mutable Qt::KeyboardModifiers m_modifiers;
        mutable QEvent::Type m_lastType;
        mutable Qt::KeyboardModifiers m_lastMods;
        mutable unsigned int m_lastButtons;
        mutable bool m_firstTime;
        mutable bool m_b1;
        mutable bool m_b2;
        mutable bool m_b3;
        mutable bool m_tabletPushed;
        mutable int m_pushx;
        mutable int m_pushy;
        mutable int m_x;
        mutable int m_y;
        mutable int m_lastx;
        mutable int m_lasty;
        mutable float m_xscale;
        mutable float m_yscale;
        mutable float m_xoffset;
        mutable float m_yoffset;
        mutable int m_width;
        mutable int m_height;
    };

} // namespace Rv

#endif // __mu_usf__QTTranslator__h__
