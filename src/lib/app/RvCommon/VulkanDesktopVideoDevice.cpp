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
        //  The base constructor advertised RGB8. This device presents through a
        //  10-bit Vulkan swapchain, so re-advertise at the depth actually
        //  delivered -- the depth is only ever used to build the description
        //  string, but that string is what Preferences, the top-view toolbar
        //  and humanReadableID() show.
        //
        //  Claiming 10 here is sound: this class is only instantiated when
        //  DesktopVideoDevice::shouldUseVulkanPresentation() is true, which
        //  already required the 10-bit surface-format probe to succeed.
        //
        //  addDefaultDataFormats() appends the same six stereo modes in the
        //  same order at any depth, so the indices are unchanged -- and the
        //  persisted "dataFormat" preference is an index, so a user's stereo
        //  choice survives a GL <-> Vulkan device rebuild.
        //
        m_dataFormats.clear();
        addDefaultDataFormats(10);
    }

    VulkanDesktopVideoDevice::~VulkanDesktopVideoDevice() { close(); }

    void VulkanDesktopVideoDevice::open(const StringVector& args)
    {
        //
        //  A passive Vulkan presentation surface. The null doc is what makes it
        //  passive: VulkanWindow::render() returns at `!session`,
        //  requestGLFallback() returns at `!m_doc`, and presentationAllowed()
        //  guards on it -- so this window never drives the session and never
        //  drags the main window into a GL fallback. It is composited into and
        //  presented by this device, via the inherited transfer()/transfer2()
        //  and syncBuffers() below.
        //
        m_vulkanView = new VulkanView(/*doc*/ nullptr, /*parent*/ nullptr, /*noResize*/ true);

        //
        //  The VulkanView owns its QTVulkanVideoDevice; handing it to the base
        //  m_viewDevice lets the inherited transfer()/transfer2() composite into
        //  its GL FBO exactly as they do for the GL ScreenView path.
        //
        //  Note this does not go through setViewWidget(): that takes a
        //  QOpenGLWidget and there is none here, so the base m_view stays null
        //  and the translator has to be installed by hand.
        //
        setViewDevice(m_vulkanView->videoDevice());

        //
        //  Never take focus or activation. This is a second-display output
        //  surface, not something the user interacts with: a focusable
        //  top-level here fights the main window for activation, and the
        //  resulting WindowActivate storm saturates the event loop -- which is
        //  what stopped annotation strokes from ever receiving their drag
        //  events. WA_ShowWithoutActivating keeps show() from stealing
        //  activation; WindowDoesNotAcceptFocus keeps the window manager from
        //  handing it back later.
        //
        m_vulkanView->setAttribute(Qt::WA_ShowWithoutActivating, true);
        m_vulkanView->setWindowFlag(Qt::WindowDoesNotAcceptFocus, true);

        //
        //  The base class allocates a translator alongside its ScreenView (see
        //  setViewWidget), so one is created here for parity. It is inert:
        //  DesktopVideoDevice::translator() has no callers, and the view is
        //  built without an event widget so it produces no events of its own.
        //
        m_translator = new QTTranslator(this, m_vulkanView);

        //
        //  Place the window before showing it. VulkanView's constructor already
        //  realized the platform window, but the swapchain is only built on
        //  first expose -- so the geometry set here is what decides which
        //  screen it lands on.
        //
        const QRect g = screenGeometry();
        m_vulkanView->move(g.x(), g.y());
        m_vulkanView->setGeometry(g);

        m_vulkanView->setWindowState(useFullScreen() ? Qt::WindowFullScreen : Qt::WindowNoState);

        m_vulkanView->setGeometry(g);

        //
        //  show() both makes the window visible on the target screen and drives
        //  the expose that creates the Vulkan surface and swapchain. Without it
        //  the presentation surface never initializes and never presents.
        //
        m_vulkanView->show();

        //
        //  Prime the offscreen GL context and FBO so the inherited transfer()'s
        //  readiness guard passes on the first frame: QTVulkanVideoDevice::fboID()
        //  reports 0 until the context and FBO have been created, and transfer()
        //  returns early while it does.
        //
        if (m_viewDevice)
        {
            m_viewDevice->makeCurrent();
        }
    }

    void VulkanDesktopVideoDevice::close()
    {
        //
        //  The VulkanView owns its QTVulkanVideoDevice (== the base
        //  m_viewDevice), so detach the base pointer WITHOUT deleting it, then
        //  delete the view -- whose destructor frees the device and its
        //  Vulkan/interop resources. Chaining to DesktopVideoDevice::close()
        //  here would delete the device a second time.
        //
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
        //
        //  Deliberately empty -- do NOT present from here.
        //
        //  Note that Session::askForRedraw() cannot actually reach this: it
        //  casts the output device to TwkGLF::GLVideoDevice, and every desktop
        //  output device derives from TwkGLF::GLBindableVideoDevice, a sibling
        //  class. The override is defence, not a hot path -- but the behaviour
        //  it defends against is real, because redraw() is also reachable from
        //  DesktopVideoDevice::syncBuffers() and from arbitrary callers,
        //  including from inside a mouse-motion handler while an annotation
        //  stroke is being drawn with the main view's GL context current.
        //
        //  Presenting here means QTVulkanVideoDevice::syncBuffers(), which
        //  makes its own offscreen context current and does not restore the
        //  previous one. That silently steals the context from whatever was
        //  mid-draw: the first stroke point lands, then every later GL call
        //  goes to the presentation device's context and the stroke stops. It
        //  would also run a swapchain present per motion event.
        //
        //  Nothing is lost by doing nothing. askForRedraw() redraws the control
        //  device, which schedules VulkanWindow::render(); that renders the
        //  frame, composites into this device through the inherited transfer(),
        //  and then presents this device in-frame via syncBuffers(). A doc-less
        //  presentation window additionally presents once on expose, so it is
        //  never left blank.
        //
        //  The base does effectively the same thing: its m_view->update() only
        //  asks Qt to composite a QOpenGLWidget whose paintGL() is empty.
        //
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
