//
//  Copyright (c) 2023 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//

#include <RvCommon/DisplayLink.h>
#include <RvCommon/CGDesktopVideoDevice.h>
#include <iostream>
#include <IPCore/Session.h>
using namespace std;

namespace Rv
{

    static CVReturn CVDisplayLinkCallback(CVDisplayLinkRef displayLink,
                                          const CVTimeStamp* inNow,
                                          const CVTimeStamp* inOutputTime,
                                          CVOptionFlags flagsIn,
                                          CVOptionFlags* flagsOut,
                                          void* displayLinkContext)
    {
        DisplayLink* link = (DisplayLink*)displayLinkContext;
        link->displayLinkEvent(inNow, inOutputTime);
        return kCVReturnSuccess;
    }

    DisplayLink::DisplayLink(QObject* parent)
        : QObject(parent)
        , m_displayLink(0)
        , m_isActive(false)
        , m_session(0)
        , m_serial(0)
    {
        // create display link for the main display
        CVDisplayLinkCreateWithCGDisplay(kCGDirectMainDisplay, &m_displayLink);
        if (m_displayLink)
        {
            // set the current display of a display link.
            CVDisplayLinkSetCurrentCGDisplay(m_displayLink,
                                             kCGDirectMainDisplay);

            // set the renderer output callback function
            CVDisplayLinkSetOutputCallback(m_displayLink,
                                           &CVDisplayLinkCallback, this);
        }
    }

    DisplayLink::~DisplayLink()
    {
        if (m_displayLink)
        {
            stop();
            CVDisplayLinkRelease(m_displayLink);
            m_displayLink = NULL;
        }
    }

    bool DisplayLink::isValid() const { return m_displayLink != 0; }

    bool DisplayLink::isActive() const { return m_isActive; }

    void DisplayLink::start(IPCore::Session* s,
                            const CGDesktopVideoDevice* cgdevice)
    {
        if (m_displayLink && !m_isActive)
        {
            CVDisplayLinkSetCurrentCGDisplay(m_displayLink,
                                             cgdevice->cgDisplay());
            CVDisplayLinkStart(m_displayLink);
            m_isActive = true;
            m_session = s;
        }
    }

    void DisplayLink::stop()
    {
        if (m_displayLink && m_isActive)
        {
            CVDisplayLinkStop(m_displayLink);
            m_isActive = false;

            //
            //  Make sure that a waiting render is allowed to finish
            //

            if (m_session->waitingOnSync())
            {
                m_session->setNextVSyncOffset(0, m_serial);
            }
        }
    }

    void DisplayLink::displayLinkEvent(const CVTimeStamp* now,
                                       const CVTimeStamp* ts)
    {
        if (ts && now)
        {
            const double secs = double(ts->videoTime - now->videoTime)
                                / double(now->videoTimeScale);
            m_serial++;
            m_session->setNextVSyncOffset(secs, m_serial);
        }
    }

} // namespace Rv
