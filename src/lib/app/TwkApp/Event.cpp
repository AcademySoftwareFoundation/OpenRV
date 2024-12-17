//******************************************************************************
// Copyright (c) 2004 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkApp/Event.h>
#include <TwkApp/EventNode.h>

namespace TwkApp
{
    using namespace std;

    void Event::output(ostream& o) const
    {
        o << "Event: " << m_name << " from "
          << ((m_sender) ? m_sender->name() : "<unknown>");
    }

    void ModifierEvent::output(ostream& o) const
    {
        Event::output(o);
        o << " [";
        if (m_modifiers & Shift)
            o << "Shift";
        if (m_modifiers & Control)
            o << "|Control";
        if (m_modifiers & Alt)
            o << "|Alt";
        if (m_modifiers & Meta)
            o << "|Meta";
        if (m_modifiers & Super)
            o << "|Super";
        if (m_modifiers & CapLock)
            o << "|CapLock";
        if (m_modifiers & NumLock)
            o << "|NumLock";
        if (m_modifiers & ScrollLock)
            o << "|ScrollLock";
        o << "]";
    }

    void KeyEvent::output(ostream& o) const
    {
        ModifierEvent::output(o);
        o << " '" << char(m_key) << "' " << hex << "0x" << m_key << dec;
    }

    void PointerEvent::output(ostream& o) const
    {
        ModifierEvent::output(o);
        o << " (" << m_x << "/" << m_w << "," << m_y << "/" << m_h
          << ") start: (" << m_px << "," << m_py << ")";
    }

} // namespace TwkApp
