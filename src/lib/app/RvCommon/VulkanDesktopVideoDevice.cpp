//
//  Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#if defined(PLATFORM_LINUX) || defined(PLATFORM_WINDOWS)

#include <RvCommon/VulkanDesktopVideoDevice.h>
#include <RvCommon/VulkanView.h>
#include <RvCommon/QTVulkanVideoDevice.h>
#include <RvCommon/QTTranslator.h>

namespace Rv
{
    using namespace std;

    VulkanDesktopVideoDevice::VulkanDesktopVideoDevice(TwkApp::VideoModule* module, const std::string& name, int screen,
                                                       const QTGLVideoDevice* shareDevice)
        : DesktopVideoDevice(module, name, screen, shareDevice)
        , m_vulkanView(nullptr)
    {
        //
        //  Re-advertise at 10-bit (the base advertised RGB8); only built once
        //  the 10-bit probe has passed. The format indices are unchanged, so
        //  the persisted "dataFormat" preference survives a backend rebuild.
        //
        m_dataFormats.clear();
        addDefaultDataFormats(10);
    }

    VulkanDesktopVideoDevice::~VulkanDesktopVideoDevice() { close(); }

    void VulkanDesktopVideoDevice::open(const StringVector& args)
    {
        //  A null doc makes the view passive (see VulkanWindow.h).
        m_vulkanView = new VulkanView(/*doc*/ nullptr, /*parent*/ nullptr, /*noResize*/ true);

        //  The view owns this device. There is no QOpenGLWidget, so this
        //  bypasses setViewWidget() and installs the translator by hand.
        setViewDevice(m_vulkanView->videoDevice());

        //  A focusable second top-level fights the main window for activation.
        m_vulkanView->setAttribute(Qt::WA_ShowWithoutActivating, true);
        m_vulkanView->setWindowFlag(Qt::WindowDoesNotAcceptFocus, true);

        //  Inert, created for parity with the base class's setViewWidget().
        m_translator = new QTTranslator(this, m_vulkanView);

        //  Place before show(): the swapchain is built on first expose.
        const QRect g = screenGeometry();
        m_vulkanView->move(g.x(), g.y());
        m_vulkanView->setGeometry(g);

        m_vulkanView->setWindowState(useFullScreen() ? Qt::WindowFullScreen : Qt::WindowNoState);

        m_vulkanView->setGeometry(g);

        m_vulkanView->show();

        //  Create the FBO now so transfer()'s fboID() readiness check passes on
        //  the first frame.
        if (m_viewDevice)
        {
            m_viewDevice->makeCurrent();
        }
    }

    void VulkanDesktopVideoDevice::close()
    {
        //
        //  The view owns m_viewDevice, so do not chain to the base close(),
        //  which would delete it again. releaseFBOClones() needs the device
        //  current, so it runs before setViewDevice(nullptr).
        //
        releaseFBOClones();

        setViewDevice(nullptr);

        delete m_vulkanView;
        m_vulkanView = nullptr;

        delete m_translator;
        m_translator = nullptr;
    }

    bool VulkanDesktopVideoDevice::isOpen() const { return m_vulkanView != nullptr; }

    void VulkanDesktopVideoDevice::makeCurrent() const
    {
        if (m_viewDevice)
        {
            m_viewDevice->makeCurrent();
        }
    }

    void VulkanDesktopVideoDevice::redraw() const
    {
        //  Intentionally empty: presenting here would steal the current GL
        //  context mid-draw. The control device's render() presents this device
        //  in-frame via syncBuffers().
    }

    void VulkanDesktopVideoDevice::redrawImmediately() const { redraw(); }

    void VulkanDesktopVideoDevice::syncBuffers() const
    {
        if (m_viewDevice && m_vulkanView && m_vulkanView->isVisible())
        {
            m_viewDevice->syncBuffers();
        }
    }

} // namespace Rv

#endif // PLATFORM_LINUX || PLATFORM_WINDOWS
