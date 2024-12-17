//******************************************************************************
// Copyright (c) 2004 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <RvCommon/QTTranslator.h>
#include <QtCore/QUrl>
#include <QtCore/QFileInfo>
#include <RvApp/Options.h>
#include <sstream>
#include <iostream>
#include <ctype.h>
#include <TwkUtil/PathConform.h>
#include <TwkUtil/FrameUtils.h>
#include <QtCore/QMimeData>

namespace
{

    bool noAutoReleaseEvents = false;

};

namespace Rv
{
    using namespace TwkApp;
    using namespace std;
    using namespace TwkUtil;

    QTTranslator::QTTranslator(EventNode* n, QWidget* w)
        : m_node(n)
        , m_widget(w)
        , m_b1(false)
        , m_b2(false)
        , m_b3(false)
        , m_tabletPushed(false)
        , m_modifiers(0)
        , m_lastMods(Qt::NoModifier)
        , m_lastType(QEvent::None)
        , m_lastButtons(Qt::NoButton)
        , m_firstTime(true)
        , m_lastx(0)
        , m_lasty(0)
        , m_xscale(1.0)
        , m_yscale(1.0)
        , m_xoffset(0.0)
        , m_yoffset(0.0)
        , m_width(0)
        , m_height(0)
    {
        noAutoReleaseEvents = (getenv("TWK_NO_AUTO_RELEASE_EVENTS") != 0);
    }

    void QTTranslator::sendEvent(const Event& event) const
    {
        // cout << "event: ";
        // event.output(cout);
        // cout << endl;
        m_node->sendEvent(event);
    }

    unsigned int QTTranslator::modifiers(Qt::KeyboardModifiers s) const
    {
        unsigned int m = 0;

        if (s & Qt::ShiftModifier)
            m |= ModifierEvent::Shift;
        if (s & Qt::ControlModifier)
            m |= ModifierEvent::Control;
        if (s & Qt::AltModifier)
            m |= ModifierEvent::Alt;
        if (s & Qt::MetaModifier)
            m |= ModifierEvent::Meta;
        if (s & Qt::KeypadModifier)
            m |= ModifierEvent::ScrollLock;

        return m;
    }

    void QTTranslator::resetModifiers() const { m_modifiers = 0; }

    Qt::MouseButtons QTTranslator::qbuttons(unsigned int b) const
    {
        Qt::MouseButtons q = Qt::NoButton;

        if (b & 0x1 << 0)
            q |= Qt::LeftButton;
        if (b & 0x1 << 1)
            q |= Qt::MidButton;
        if (b & 0x1 << 2)
            q |= Qt::RightButton;
        return q;
    }

    bool QTTranslator::sendQTEvent(QEvent* event, float activationTime) const
    {
        //
        //  This is trying to correct for odd behavior with menu
        //  accelerators on the Mac. Qt will not necessarily send the up
        //  event on the modifer key so our internal state can get
        //  lost. For this reason, its a good idea not to use Alt combos
        //  in the menus. The Alt state is very strange in Qt.
        //

        if (QInputEvent* inputEvent = dynamic_cast<QInputEvent*>(event))
        {
            if ((inputEvent->modifiers() & 0xffff0000) != m_modifiers)
            {
                //             cout << "input event: modifier mismatch "
                //                  << hex << inputEvent->modifiers()
                //                  << " != "
                //                  << hex << m_modifiers
                //                  << "  (event=" << event->type() << ")"
                //                  << dec << endl;

                //             if (m_modifiers & Qt::ShiftModifier) cout << "
                //             Shift"; if (m_modifiers & Qt::AltModifier) cout
                //             << " Alt"; if (m_modifiers & Qt::ControlModifier)
                //             cout << " Control"; if (m_modifiers &
                //             Qt::MetaModifier) cout << " Meta"; if
                //             (m_modifiers & Qt::KeypadModifier) cout << "
                //             Keypad"; if (m_modifiers &
                //             Qt::GroupSwitchModifier) cout << " GroupSwitch";
                //             if (m_modifiers != 0) cout << endl << endl;

#ifndef PLATFORM_DARWIN
                //
                //  This seems to be no longer necessary, in fact causes
                //  problems on mac (qt 4.7), but not on win/lin.  The problem
                //  is that in many cases this code causes the control modifier
                //  to be lost.
                //
                if (m_modifiers & Qt::ControlModifier
                    || m_modifiers & Qt::MetaModifier)
                {
                    m_modifiers = inputEvent->modifiers() & 0xffff0000;
                }
#endif
            }
        }

        switch (event->type())
        {
        case QEvent::None: // invalid event
            break;
        case QEvent::Timer: // timer event
            break;
        case QEvent::Enter:          // mouse enters widget
        case QEvent::Leave:          // mouse leaves widget
        case QEvent::WindowActivate: // window becomes active
            return sendEnterLeaveEvent(event);
        case QEvent::MouseButtonPress:    // mouse button pressed
        case QEvent::MouseButtonRelease:  // mouse button released
        case QEvent::MouseButtonDblClick: // mouse button double click
        case QEvent::MouseMove:           // mouse move
            if (!Options::sharedOptions().stylusAsMouse && m_tabletPushed)
            {
                // For some reason if the pen is in use ocassionally we
                // get interfering mouse events. This is a workaround.
                return true;
            }
            else
            {
                return sendMouseEvent(event, activationTime);
            }
            break;
        case QEvent::Wheel:
            return sendMouseWheelEvent(event);

        case QEvent::ShortcutOverride: // ignore this or not?
        case QEvent::KeyPress:         // key pressed
        case QEvent::KeyRelease:       // key released
        case QEvent::Shortcut:
            return sendKeyEvent(event);

        case QEvent::FocusIn:   // keyboard focus received
        case QEvent::FocusOut:  // keyboard focus lost
        case QEvent::Clipboard: // internal clipboard event
            break;
        case QEvent::DragEnter:    // drag moves into widget
        case QEvent::DragMove:     // drag moves in widget
        case QEvent::DragLeave:    // drag leaves or is cancelled
        case QEvent::Drop:         // actual drop
        case QEvent::DragResponse: // drag accepted/rejected
            return sendDNDEvent(event);
        case QEvent::ContextMenu: // context popup menu
            break;
        case QEvent::TabletMove:    // Wacom tablet event
        case QEvent::TabletPress:   // tablet press
        case QEvent::TabletRelease: // tablet release
            if (!Options::sharedOptions().stylusAsMouse)
            {
                return sendTabletEvent(event);
            }
            break;
        case QEvent::OkRequest:   // CE (Ok) button pressed
        case QEvent::HelpRequest: // CE (?)  button pressed
            break;
        case QEvent::IconDrag: // proxy icon dragged
            break;
        case QEvent::TabletEnterProximity:
        case QEvent::TabletLeaveProximity:
            if (!Options::sharedOptions().stylusAsMouse)
            {
                return true;
            }
            break;
        default:
            break;
        }

        return false;
    }

    bool QTTranslator::sendKeyEvent(QEvent* qevent) const
    {
        ostringstream str;

        QKeyEvent* event = static_cast<QKeyEvent*>(qevent);
        bool showShift = true;
        bool control = false;
        bool nokeypad = false;
        bool meta = false;
        bool alt = false;
        bool keydown = event->type() == QEvent::KeyPress
                       || event->type() == QEvent::ShortcutOverride
                       || event->type() == QEvent::Shortcut;

        //  On the mac, there are no "releases" reported during auto-repeat,
        //  we do the below to make this consistant on other platforms.
        //
        if (noAutoReleaseEvents)
        {
            if (event->isAutoRepeat() && event->type() == QEvent::KeyRelease)
                return true;
        }

        QVector<unsigned int> ucs4string = event->text().toUcs4();
        string utf8string = event->text().toUtf8().data();

        //
        //  On the Mac the Alt/Option modifier key provides access to accents
        //  and additional symbols that we are not interested in dealing with.
        //  This platform specific code below will make sure things like "Alt
        //  + 4" send the "key-down--alt--4" event and not something about the
        //  United Kingdom's currency. The intended purpose is to make events
        //  match on all platforms.
        //

        //  XXX we _still_ have problems with release event modifiers not
        //  matching press event.  This might fix it (for later):
        //
        //  if (event->type() == QEvent::ShortcutOverride || event->type() ==
        //  QEvent::KeyRelease)
        //

        if (event->type() == QEvent::ShortcutOverride)
        {
            //
            //  Shortcuts with no mofifier are not have the text set in
            //  the event. The key is set, so if its ascii copy that to
            //  the event text.
            //

            if (event->key() <= Qt::Key_AsciiTilde)
            {
                if (event->key() >= Qt::Key_A && event->key() <= Qt::Key_Z)
                {
                    unsigned int kd = event->key() - Qt::Key_A;
                    ucs4string.resize(1);
                    utf8string = " ";

                    if (m_modifiers & Qt::ShiftModifier)
                    {
                        utf8string[0] = kd + 'A';
                    }
                    else
                    {
                        utf8string[0] = kd + 'a';
                    }

                    ucs4string[0] = utf8string[0];
                }
                else
                {
#ifndef PLATFORM_WINDOWS
                    char temp[2];
                    temp[0] = char(event->key());
                    temp[1] = 0;
                    utf8string = temp;
                    ucs4string.resize(1);
                    ucs4string[0] = event->key();
#endif
                }
            }
        }

        //
        //  Qt has some really wacky platform-dependant behavior with even
        //  modifiers and keys. In some cases the modifiers are completely
        //  ignored. On the Mac they switch control and meta. Fortunately, the
        //  actual modifier keys themselves are consistantly tracked on all
        //  platforms. So we keep track of the modifier state ourselves and
        //  ignore the Qt state. This can be problematic when enter/leave
        //  occurs, but that should be dealt with as a special case.
        //

        switch (event->key())
        {
        case Qt::Key_Shift:
            if (keydown)
                m_modifiers |= Qt::ShiftModifier;
            else
                m_modifiers &= ~Qt::ShiftModifier;
            break;

        case Qt::Key_Control:
            if (keydown)
                m_modifiers |= Qt::ControlModifier;
            else
                m_modifiers &= ~Qt::ControlModifier;
            break;

        case Qt::Key_Meta:
        case Qt::Key_Super_L:
        case Qt::Key_Super_R:
            meta = true;
            if (keydown)
                m_modifiers |= Qt::MetaModifier;
            else
                m_modifiers &= ~Qt::MetaModifier;
            break;

        case Qt::Key_Alt:
            alt = true;
            if (keydown)
                m_modifiers |= Qt::AltModifier;
            else
                m_modifiers &= ~Qt::AltModifier;
            break;

        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Down:
            //
            //    On the mac the arrows come across with keypad mod set. Stop
            //    this from being passed on.
            //
            nokeypad = true;
            break;
        }

        //
        //  If its an upper case key, make sure we show the shift modifier
        //  too. Also check for control keys.
        //

        if (utf8string.size() == 1)
        {
            char rkey = utf8string[0];
            if (isupper(rkey) || ispunct(rkey))
                showShift = false;
            control = event->key() <= 128 && iscntrl(rkey);
        }

        //
        //  Add the key and modifers to the event string
        //

        str << "key-" << (keydown ? "down-" : "up-");

        Qt::KeyboardModifiers mods = m_modifiers;
        if (nokeypad)
            mods &= ~Qt::KeypadModifier;

        str << modifierString(mods, showShift);

        switch (event->key())
        {
        case Qt::Key_Shift:
            str << "-shift";
            break;
        case Qt::Key_Control:
            str << "-control";
            break;
        case Qt::Key_Meta:
            str << "-meta";
            break;
        case Qt::Key_Alt:
            str << "-alt";
            break;
        case Qt::Key_Escape:
            str << "-escape";
            break;
        case Qt::Key_Tab:
            str << "-tab";
            break;
        case Qt::Key_Backtab:
            str << "-tab";
            break;
        case Qt::Key_Backspace:
            str << "-backspace";
            break;
        case Qt::Key_Return:
            str << "-enter";
            break;
        case Qt::Key_Enter:
            str << "-keypad-enter";
            break;
        case Qt::Key_Insert:
            str << "-insert";
            break;
        case Qt::Key_Delete:
            str << "-delete";
            break;
        case Qt::Key_Pause:
            str << "-pause";
            break;
        case Qt::Key_Print:
            str << "-print";
            break;
        case Qt::Key_SysReq:
            str << "-sysreq";
            break;
        case Qt::Key_Clear:
            str << "-clear";
            break;
        case Qt::Key_Home:
            str << "-home";
            break;
        case Qt::Key_End:
            str << "-end";
            break;
        case Qt::Key_Left:
            str << "-left";
            break;
        case Qt::Key_Up:
            str << "-up";
            break;
        case Qt::Key_Right:
            str << "-right";
            break;
        case Qt::Key_Down:
            str << "-down";
            break;
        case Qt::Key_PageUp:
            str << "-page-up";
            break;
        case Qt::Key_PageDown:
            str << "-page-down";
            break;
        case Qt::Key_AltGr:
            str << "-alt-gr";
            alt = true;
            break;
        case Qt::Key_CapsLock:
            str << "-caplock";
            break;
        case Qt::Key_NumLock:
            str << "-numlock";
            break;
        case Qt::Key_ScrollLock:
            str << "-scrolllock";
            break;
        case Qt::Key_Help:
            str << "-help";
            break;
        case Qt::Key_Space:
            str << "- ";
            break;
        case Qt::Key_Super_L:
            str << "-super-left";
            break;
        case Qt::Key_Super_R:
            str << "-super-right";
            break;
        case Qt::Key_Hyper_L:
            str << "-hyper-left";
            break;
        case Qt::Key_Hyper_R:
            str << "-hyper-right";
            break;
        case Qt::Key_Direction_L:
            str << "-direction-left";
            break;
        case Qt::Key_Direction_R:
            str << "-direction-right";
            break;
        case Qt::Key_Menu:
            str << "-menu";
            break;
        case 0:
        case Qt::Key_unknown:
            str << "-unknown";
            break;
        default:

#if 0
          if (event->modifiers() != m_modifiers)
          {
              cout << "keyboard: modifier mismatch "
                   << event->modifiers() 
                   << " != "
                   << hex << m_modifiers
                   << "  (event=" << event->type() << ")"
                   << dec << endl;
          }
#endif

            //
            //    Function Keys
            //
            if (event->key() >= Qt::Key_F1 && event->key() <= Qt::Key_F35)
            {
                str << "-f" << ((event->key() - Qt::Key_F1) + 1);
            }
            else if (event->key() >= Qt::Key_A && event->key() <= Qt::Key_Z)
            {
                unsigned int kd = event->key() - Qt::Key_A;

                if (m_modifiers & Qt::ShiftModifier)
                {
                    str << "-" << char(kd + 'A');
                }
                else
                {
                    str << "-" << char(kd + 'a');
                }
            }
            else
            {
                str << "-" << utf8string;
            }

            break;
        }

        //
        //  On windows, wchar_t == unsigned short, on all other platforms its
        //  unsigned int. Fortunately, Qt provides a UCS4 function which will
        //  work on all platforms.
        //

        unsigned int key =
            (alt || ucs4string.empty()) ? event->key() : ucs4string.front();

        if (keydown)
        {
            KeyPressEvent e(str.str(), m_node, key, modifiers(mods));

            sendEvent(e);
            return e.handled;
        }
        else
        {
            KeyReleaseEvent e(str.str(), m_node, key, modifiers(mods));

            sendEvent(e);
            return e.handled;
        }
    }

    string QTTranslator::pointerString(bool b1, bool b2, bool b3,
                                       QEvent::Type eventType,
                                       Qt::KeyboardModifiers mods,
                                       Qt::MouseButtons buttons,
                                       bool tabletPushed = false) const
    {
        string n;

        if (!Options::sharedOptions().stylusAsMouse
            && (eventType == QEvent::TabletPress
                || eventType == QEvent::TabletRelease
                || eventType == QEvent::TabletMove))
        {
            n = "";
        }
        else
        {
            n = "pointer-";
        }

        if (b1)
            n += "1-";
        if (b2)
            n += "2-";
        if (b3)
            n += "3-";

        n += modifierString(mods, true);
        n += "-";

        if (Options::sharedOptions().stylusAsMouse)
        {
            switch (eventType)
            {
            case QEvent::MouseButtonPress:
                n += "push";
                break;
            case QEvent::MouseButtonRelease:
                n += "release";
                break;
            case QEvent::MouseButtonDblClick:
                n += "double";
                break;
            case QEvent::MouseMove:
                n += (buttons) ? "drag" : "move";
                break;
            case QEvent::Enter:
                n += "enter";
                break;
            case QEvent::Leave:
                n += "leave";
                break;
            case QEvent::WindowActivate:
                n += "activate";
                break;
            default:
                break;
            }
        }
        else
        {
            switch (eventType)
            {
            case QEvent::MouseButtonPress:
            case QEvent::TabletPress:
                n += "push";
                break;
            case QEvent::MouseButtonDblClick:
                n += "double";
                break;
            case QEvent::MouseButtonRelease:
            case QEvent::TabletRelease:
                n += "release";
                break;
            case QEvent::MouseMove:
            case QEvent::TabletMove:
                n += (buttons || tabletPushed) ? "drag" : "move";
                break;
            case QEvent::TabletEnterProximity:
            case QEvent::Enter:
                n += "enter";
                break;
            case QEvent::WindowActivate:
                n += "activate";
                break;
                break;
            case QEvent::Leave:
            case QEvent::TabletLeaveProximity:
                n += "leave";
                break;
            default:
                break;
            }
        }
        return n;
    }

    bool QTTranslator::sendMouseWheelEvent(QEvent* qevent) const
    {
        QWheelEvent* event = static_cast<QWheelEvent*>(qevent);

        string n = pointerString(m_b1, m_b2, m_b3, event->type(), m_modifiers,
                                 event->buttons());

        n += "wheel";
        if (event->delta() < 0)
            n += "down";
        else
            n += "up";

        PointerEvent e(n, m_node, modifiers(m_modifiers), m_x, m_y, m_width,
                       m_height, m_pushx, m_pushy, buttons());

        sendEvent(e);
        return e.handled;
    }

    bool QTTranslator::sendEnterLeaveEvent(QEvent* event) const
    {
        //
        //  Enter/Leave doesn't contain much information (er... any) about
        //  the pointer. So we'll assume that the app was able to track
        //  the state somehow while the pointer was out of the window. On
        //  some platforms this is true by default (mac/windows) on Linux,
        //  the window manager can completely make a mess out of this by
        //  shifting focus in unexpected ways.
        //

        QEvent::Type type = event->type();
        Qt::KeyboardModifiers mods = m_modifiers;

        string n =
            pointerString(m_b1, m_b2, m_b3, type, mods, qbuttons(buttons()));

        PointerEvent e(n, m_node, modifiers(mods), m_x, m_y, m_width, m_height,
                       m_x, m_y, buttons());

        sendEvent(e);
        return e.handled;
    }

    bool QTTranslator::sendMouseEvent(QEvent* qevent,
                                      float activationTime) const
    {
        QMouseEvent* event = static_cast<QMouseEvent*>(qevent);
        QEvent::Type type = event->type();
        Qt::KeyboardModifiers mods = m_modifiers;
        Qt::MouseButtons ebuttons = event->buttons();
        bool handled = false;
        string n;

        bool b1 = event->buttons() & Qt::LeftButton;
        bool b2 = event->buttons() & Qt::MidButton;
        bool b3 = event->buttons() & Qt::RightButton;

        m_x = (m_xscale * event->x()) + m_xoffset;
        m_y = (m_yscale * (m_widget->height() - event->y())) + m_yoffset;

        if (type == QEvent::MouseMove && !buttons())
        {
            n = pointerString(false, false, false, type, mods, ebuttons);

            PointerEvent event(n, m_node, modifiers(mods), m_x, m_y, m_width,
                               m_height, m_lastx, m_lasty, buttons());

            sendEvent(event);
            handled = event.handled;
        }
        else if (type == QEvent::MouseButtonPress
                 || type == QEvent::MouseButtonDblClick)
        {
            //
            //  Release old push
            //

            if ((m_b1 || m_b2 || m_b3 || m_lastMods != m_modifiers)
                && !m_firstTime)
            {
                n = pointerString(m_b1, m_b2, m_b3, QEvent::MouseButtonRelease,
                                  m_lastMods, qbuttons(m_lastButtons));

                PointerButtonReleaseEvent event(
                    n, m_node, modifiers(m_lastMods), m_x, m_y, m_width,
                    m_height, m_pushx, m_pushy, m_lastButtons);
                sendEvent(event);
            }

            //
            //  Make new push
            //

            n = pointerString(b1, b2, b3, type, mods, ebuttons);

            PointerButtonPressEvent event(n, m_node, mods, m_x, m_y, m_width,
                                          m_height, m_x, m_y, buttons(), 0,
                                          activationTime);
            sendEvent(event);
            handled = event.handled;
            m_pushx = m_x;
            m_pushy = m_y;
        }
        else if (type == QEvent::MouseButtonRelease)
        {
            //
            //  Release relative to old event
            //

            n = pointerString(m_b1, m_b2, m_b3, QEvent::MouseButtonRelease,
                              mods, ebuttons);

            PointerButtonReleaseEvent event(n, m_node, modifiers(mods), m_x,
                                            m_y, m_width, m_height, m_pushx,
                                            m_pushy, buttons());
            sendEvent(event);
            handled = event.handled;

            if (b1 || b2 || b3)
            {
                n = pointerString(b1, b2, b3, QEvent::MouseButtonPress, mods,
                                  ebuttons);

                PointerButtonPressEvent event(n, m_node, modifiers(mods), m_x,
                                              m_y, m_width, m_height, m_x, m_y,
                                              buttons());
                sendEvent(event);
                handled = event.handled;
                m_pushx = m_x;
                m_pushy = m_y;
            }
        }
        else
        {
            if ((b1 != m_b1 || b2 != m_b2 || b3 != m_b3
                 || m_lastMods != m_modifiers)
                && !m_firstTime)
            {
                n = pointerString(m_b1, m_b2, m_b3, QEvent::MouseButtonRelease,
                                  m_lastMods, qbuttons(m_lastButtons));

                PointerButtonReleaseEvent eEnd(
                    n, m_node, modifiers(m_modifiers), m_x, m_y, m_width,
                    m_height, m_pushx, m_pushy, m_lastButtons);

                n = pointerString(b1, b2, b3, QEvent::MouseButtonPress, mods,
                                  ebuttons);

                PointerButtonPressEvent eBegin(n, m_node, modifiers(mods), m_x,
                                               m_y, m_width, m_height, m_x, m_y,
                                               buttons());

                sendEvent(eEnd);
                m_pushx = m_x;
                m_pushy = m_y;
                sendEvent(eBegin);
            }

            n = pointerString(b1, b2, b3, type, mods, ebuttons);

            PointerEvent event(n, m_node, modifiers(mods), m_x, m_y, m_width,
                               m_height, m_pushx, m_pushy, buttons());

            sendEvent(event);
            handled = event.handled;
        }

        m_lastButtons = buttons();
        m_lastType = type;
        m_lastMods = m_modifiers;
        m_b1 = b1;
        m_b2 = b2;
        m_b3 = b3;
        m_firstTime = false;
        m_lastx = m_x;
        m_lasty = m_y;

        return handled ? true : false;
    }

    unsigned int QTTranslator::buttons() const
    {
        return int(m_b1) | (int(m_b2) << 1) | (int(m_b3) << 2);
    }

    string QTTranslator::modifierString(Qt::KeyboardModifiers mods,
                                        bool showShift) const
    {
        string s;
        unsigned int m = modifiers(mods);

        if (m & ModifierEvent::Alt)
            s += "-alt";
        if (m & ModifierEvent::CapLock)
            s += "-caplock";
        if (m & ModifierEvent::Control)
            s += "-control";
        if (m & ModifierEvent::Meta)
            s += "-meta";
        if (m & ModifierEvent::NumLock)
            s += "-numlock";
        if (m & ModifierEvent::ScrollLock)
            s += "-scrolllock";

        if (showShift && m & ModifierEvent::Shift)
            s += "-shift";

        if (s.size())
            s += "-";

        return s;
    }

    bool QTTranslator::sendDNDEvent(QEvent* event) const
    {
        if (QDropEvent* de = dynamic_cast<QDropEvent*>(event))
        {
            //
            //  Ignore attempts by the dropper to delete the dropped file after
            //  RV accepts the drop. NOTE: on windows MoveAction means the
            //  explorere will delete the file after the drop. on mac,
            //  MoveAction seems to stand in for CopyAction sometimes, so just
            //  treat everything as a copy.
            //
            if (de->proposedAction() == Qt::MoveAction
                || de->proposedAction() == Qt::TargetMoveAction)
            {
                de->setDropAction(Qt::CopyAction);
            }
        }

        bool handled = false;
        DragDropEvent::Type type;
        int w = m_width;
        int h = m_height;

        bool leave = event->type() == QEvent::DragLeave;
        string ename = "dragdrop--";

        switch (event->type())
        {
        case QEvent::DragEnter:
            type = DragDropEvent::Enter;
            ename += "enter";
            break;
        case QEvent::DragMove:
            type = DragDropEvent::Move;
            ename += "move";
            break;
        case QEvent::DragLeave:
            type = DragDropEvent::Leave;
            ename += "leave";
            break;
        case QEvent::Drop:
            type = DragDropEvent::Release;
            ename += "release";
            break;
        default:
            break;
        }

        if (leave)
        {
            DragDropEvent e(ename, m_node, DragDropEvent::Leave,
                            DragDropEvent::File, "", 0, 0, 0, w, h);

            sendEvent(e);
            handled = e.handled;
        }
        else
        {
            QDropEvent* devent = static_cast<QDropEvent*>(event);

#if 0
        QStringList formats = devent->mimeData()->formats();

        for (int i=0; i < formats.size(); i++)
        {
            cout << "formats[" << i << "] = " << formats[i].toUtf8().constData() << endl;
        }
#endif

            if (devent->mimeData()->hasUrls())
            {
                QList<QUrl> urls = devent->mimeData()->urls();

                FileNameList files;
                FileNameList nonfiles;

                for (QList<QUrl>::iterator i = urls.begin(); i < urls.end();
                     ++i)
                {
                    if (i->scheme() == "" || i->path() == "")
                        continue;

                    if (i->scheme() == "file")
                    {
                        QString ppath = i->toLocalFile();
#ifdef PLATFORM_WINDOWS
                        QFileInfo ppFI = QFileInfo(ppath);
                        if (ppFI.isDir())
                        {
                            ppath += QString("/");
                        }
#endif
                        // #ifdef WIN32
                        // while (ppath[0] == '/') ppath.remove(0, 1);
                        // #endif
                        files.push_back(
                            pathConform(ppath.toUtf8().constData()));
                    }
                    else
                    {
                        nonfiles.push_back(i->toString().toUtf8().constData());
                    }
                }

                SequenceNameList seqs;
                if (files.size() && QFileInfo(files[0].c_str()).isDir())
                //
                //  Don't try to form sequences out of list of directories
                //
                {
                    seqs = files;
                }
                else
                    seqs = sequencesInFileList(files, GlobalExtensionPredicate);

                for (int i = 0; i < seqs.size(); i++)
                {
                    DragDropEvent e(ename, m_node, type, DragDropEvent::File,
                                    seqs[i], 0, devent->pos().x(),
                                    h - devent->pos().y() - 1, w, h);

                    sendEvent(e);
                    if (e.handled)
                        handled = true;
                }

                for (int i = 0; i < nonfiles.size(); i++)
                {
                    DragDropEvent e(ename, m_node, type, DragDropEvent::URL,
                                    nonfiles[i], 0, devent->pos().x(),
                                    h - devent->pos().y() - 1, w, h);

                    sendEvent(e);
                    if (e.handled)
                        handled = true;
                }
            }
            else if (devent->mimeData()->hasText())
            {
                DragDropEvent e(ename, m_node, type, DragDropEvent::Text,
                                devent->mimeData()->text().toUtf8().constData(),
                                0, devent->pos().x(), h - devent->pos().y() - 1,
                                w, h);

                sendEvent(e);
                if (e.handled)
                    handled = true;
            }
        }

        return handled;
    }

    bool QTTranslator::sendTabletEvent(QEvent* qevent) const
    {
        QTabletEvent* event = static_cast<QTabletEvent*>(qevent);
        QEvent::Type type = event->type();
        Qt::KeyboardModifiers mods = m_modifiers;
        Qt::MouseButtons ebuttons = qbuttons(m_lastButtons);
        bool handled = false;

        QPoint p = m_widget->mapToGlobal(QPoint(0, 0));
        double xoff = p.x();
        double yoff = p.y();

        const double gx = event->hiResGlobalX() - xoff;
        const double gy =
            double(m_widget->height()) - (event->hiResGlobalY() - yoff);
        const double pressure = event->pressure();
        const double tpressure = event->tangentialPressure();
        const double rot = event->rotation();
        const int xtilt = event->xTilt();
        const int ytilt = event->yTilt();
        const int z = event->z();

        TabletEvent::TabletKind kind;
        TabletEvent::TabletDevice device;

        string n;

        switch (event->device())
        {
        case QTabletEvent::NoDevice:
            device = TabletEvent::NoTableDevice;
            n = "generic-tablet-device-";
            break;
        case QTabletEvent::Puck:
            device = TabletEvent::PuckDevice;
            n = "puck-";
            break;
        case QTabletEvent::Stylus:
            device = TabletEvent::StylusDevice;
            n = "stylus-";
            break;
        case QTabletEvent::Airbrush:
            device = TabletEvent::AirBrushDevice;
            n = "airbrush-";
            break;
        case QTabletEvent::FourDMouse:
            device = TabletEvent::FourDMouseDevice;
            n = "mouse4D-";
            break;
        case QTabletEvent::RotationStylus:
            device = TabletEvent::RotationStylusDevice;
            n = "rotating-stylus-";
            break;
        default:
            break;
        }

        switch (event->pointerType())
        {
        case QTabletEvent::UnknownPointer:
            kind = TabletEvent::UnknownTabletKind;
            break;
        case QTabletEvent::Pen:
            kind = TabletEvent::PenKind;
            n += "pen-";
            break;
        case QTabletEvent::Cursor:
            kind = TabletEvent::CursorKind;
            n += "cursor-";
            break;
        case QTabletEvent::Eraser:
            kind = TabletEvent::EraserKind;
            n += "eraser-";
            break;
        }

        bool b1 = m_b1;
        bool b2 = m_b2;
        bool b3 = m_b3;

        m_x = gx * m_xscale + m_xoffset;
        m_y = gy * m_yscale + m_yoffset;

        if (type == QEvent::TabletPress)
        {
            m_pushx = m_x;
            m_pushy = m_y;
        }

        n += pointerString(b1, b2, b3, type, mods, ebuttons, m_tabletPushed);

        switch (type)
        {
        case QEvent::TabletPress:
            //
            //  Note: Qt's autograb doesn't seem to work with tablets,
            //  so we grab/release manually here.
            //
            m_tabletPushed = true;
            m_widget->grabMouse();
            break;
        case QEvent::TabletRelease:
            m_tabletPushed = false;
            m_widget->releaseMouse();
            break;
        default:
            break;
        }

        TabletEvent e(n, m_node, modifiers(mods), m_x, m_y, m_width, m_height,
                      // m_lastx, m_lasty,
                      m_pushx, m_pushy, buttons(), kind, device, gx, gy,
                      pressure, tpressure, rot, xtilt, ytilt, z);

        sendEvent(e);
        handled = e.handled;

        m_lastButtons = buttons();
        m_lastType = type;
        m_lastMods = m_modifiers;
        m_b1 = b1;
        m_b2 = b2;
        m_b3 = b3;
        m_firstTime = false;
        // m_lastx       = m_x;
        // m_lasty       = m_y;

        return true;
    }

    void QTTranslator::setScaleAndOffset(float x, float y, float sx,
                                         float sy) const
    {
        m_xoffset = x;
        m_yoffset = y;
        m_xscale = sx;
        m_yscale = sy;
    }

    void QTTranslator::setRelativeDomain(float w, float h) const
    {
        m_width = w;
        m_height = h;
    }

} // namespace Rv
