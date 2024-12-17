//
//  Copyright (c) 2023 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//

#ifndef AVFDISPLAYLINK_H
#define AVFDISPLAYLINK_H

#include <QtCore/qobject.h>
#include <QuartzCore/CVDisplayLink.h>

namespace IPCore
{
    class Session;
}

namespace Rv
{

    class CGDesktopVideoDevice;

    class DisplayLink : public QObject
    {
        Q_OBJECT
    public:
        explicit DisplayLink(QObject* parent = 0);
        virtual ~DisplayLink();
        bool isValid() const;
        bool isActive() const;

    public Q_SLOTS:
        void start(IPCore::Session*, const CGDesktopVideoDevice*);
        void stop();

    Q_SIGNALS:
        void tick(const CVTimeStamp& ts);

    public:
        void displayLinkEvent(const CVTimeStamp*, const CVTimeStamp*);

    private:
        CVDisplayLinkRef m_displayLink;
        bool m_isActive;
        IPCore::Session* m_session;
        size_t m_serial;
    };

} // namespace Rv

#endif // DISPLAYLINK_H

#ifdef check
#undef check
#endif
