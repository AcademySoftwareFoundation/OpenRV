//
//  Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#ifdef PLATFORM_DARWIN

#include <RvCommon/MetalDesktopVideoDevice.h>
#include <RvCommon/MetalView.h>
#include <RvCommon/QTMetalVideoDevice.h>
#include <RvCommon/QTTranslator.h>

#include <QScreen>
#include <QGuiApplication>

#include <cstdlib>

namespace Rv
{
    using namespace std;

    namespace
    {
        //  Set RV_METAL_DEBUG_PRESENT to trace presentation-output placement and
        //  per-frame presents. A black second display is almost always either a
        //  window that never became visible on the intended screen, or a present
        //  that is being skipped; these logs distinguish the two.
        bool debugPresent()
        {
            static const bool on = getenv("RV_METAL_DEBUG_PRESENT") != nullptr;
            return on;
        }
    } // namespace

    MetalDesktopVideoDevice::MetalDesktopVideoDevice(TwkApp::VideoModule* module, const std::string& name, int screen,
                                                     const TwkGLF::GLVideoDevice* shareDevice)
        : DesktopVideoDevice(module, name, screen, shareDevice)
        , m_metalView(nullptr)
    {
    }

    MetalDesktopVideoDevice::~MetalDesktopVideoDevice() { close(); }

    void MetalDesktopVideoDevice::open(const StringVector& args)
    {
        //  Passive Metal presentation surface: a null doc + presentationMode
        //  means the MetalView never drives the session and never triggers the
        //  main-window GL fallback. It is composited into and presented by this
        //  device (via the inherited transfer()/transfer2() and syncBuffers()).
        m_metalView = new MetalView(/*doc*/ nullptr, /*parent*/ nullptr, /*noResize*/ true, /*presentationMode*/ true);

        //  The MetalView owns its QTMetalVideoDevice; assigning it to the base
        //  m_viewDevice lets the inherited transfer()/transfer2() composite into
        //  its GL FBO exactly as they do for the GL ScreenView path.
        setViewDevice(m_metalView->videoDevice());
        m_translator = new QTTranslator(this, m_metalView);

        QRect g = screenGeometry();
        m_metalView->move(g.x(), g.y());
        m_metalView->setGeometry(g);

        if (useFullScreen())
        {
            m_metalView->setWindowState(Qt::WindowFullScreen);
        }
        else
        {
            m_metalView->setWindowState(Qt::WindowNoState);
        }

        //  Re-apply: the window-state change above resets the geometry.
        m_metalView->setGeometry(g);

        //  Show the window: this is what makes it visible on the target screen
        //  AND triggers MetalView::showEvent() -> initialize(), which attaches
        //  the CALayer to the native NSView. Without it the presentation surface
        //  never gets a layer and never presents.
        m_metalView->show();

        if (debugPresent())
        {
            const QList<QScreen*> screens = QGuiApplication::screens();
            const int idx = qtScreen();
            const QString wantName = (idx >= 0 && idx < screens.size()) ? screens[idx]->name() : QString("<invalid>");
            QScreen* actual = m_metalView->screen();
            cout << "INFO: MetalDesktopVideoDevice::open: intended screen " << idx << " '" << wantName.toStdString() << "'; computed geom "
                 << g.x() << "," << g.y() << " " << g.width() << "x" << g.height() << "; fullscreen=" << (useFullScreen() ? 1 : 0) << endl;
            cout << "INFO: MetalDesktopVideoDevice::open: actual placed screen '"
                 << (actual ? actual->name().toStdString() : std::string("<null>")) << "' geom " << m_metalView->geometry().x() << ","
                 << m_metalView->geometry().y() << " " << m_metalView->geometry().width() << "x" << m_metalView->geometry().height()
                 << " visible=" << (m_metalView->isVisible() ? 1 : 0) << " windowState=" << int(m_metalView->windowState()) << endl;
        }

        //  Prime the offscreen GL context + FBO so the inherited transfer()'s
        //  fboID() guard passes on the first frame (QTMetalVideoDevice::fboID()
        //  reports 0 until its context/FBO has been created).
        if (m_viewDevice)
        {
            m_viewDevice->makeCurrent();
        }
    }

    void MetalDesktopVideoDevice::close()
    {
        //  The MetalView owns its QTMetalVideoDevice (== base m_viewDevice), so
        //  detach the base pointer WITHOUT deleting it, then delete the view
        //  (whose destructor frees the device and its IOSurface/interop
        //  resources). This differs from the base DesktopVideoDevice::close(),
        //  which does own m_viewDevice.
        setViewDevice(nullptr);

        delete m_metalView;
        m_metalView = nullptr;

        delete m_translator;
        m_translator = nullptr;
    }

    bool MetalDesktopVideoDevice::isOpen() const { return m_metalView != nullptr; }

    void MetalDesktopVideoDevice::makeCurrent() const
    {
        if (m_viewDevice)
        {
            m_viewDevice->makeCurrent();
        }
    }

    void MetalDesktopVideoDevice::redraw() const
    {
        //  There is no QOpenGLWidget auto-composite here, so the frame already
        //  composited into the QTMetalVideoDevice FBO by transfer()/transfer2()
        //  must be presented explicitly.
        if (m_viewDevice && m_metalView && m_metalView->isVisible())
        {
            m_viewDevice->syncBuffers();
        }
    }

    void MetalDesktopVideoDevice::redrawImmediately() const { redraw(); }

    void MetalDesktopVideoDevice::syncBuffers() const
    {
        if (m_viewDevice && m_metalView && m_metalView->isVisible())
        {
            if (debugPresent())
            {
                cout << "INFO: MetalDesktopVideoDevice::syncBuffers[screen " << qtScreen() << "]: presenting" << endl;
            }
            m_viewDevice->syncBuffers();
        }
        else if (debugPresent())
        {
            //  Shows which guard failed: a black external output whose view never
            //  became visible lands here.
            cout << "INFO: MetalDesktopVideoDevice::syncBuffers[screen " << qtScreen()
                 << "]: SKIPPED present (viewDevice=" << (m_viewDevice ? 1 : 0) << " metalView=" << (m_metalView ? 1 : 0)
                 << " visible=" << (m_metalView && m_metalView->isVisible() ? 1 : 0) << ")" << endl;
        }
    }

} // namespace Rv

#endif // PLATFORM_DARWIN
