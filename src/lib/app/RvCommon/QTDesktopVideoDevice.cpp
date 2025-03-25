//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifdef PLATFORM_WINDOWS
#include <GL/glew.h>
#include <GL/wglew.h>
#include <Wingdi.h>
#include <Shlwapi.h>
#include <lcms2.h>
#endif
#include <RvCommon/QTDesktopVideoDevice.h>
#include <TwkGLF/GLFBO.h>
#include <QtGui/QtGui>
#include <QtWidgets/QApplication>
#include <QScreen>

#define DEBUG_NO_FULLSCREEN

namespace Rv
{

    using namespace std;
    using namespace TwkGLF;
    using namespace TwkApp;

    //----------------------------------------------------------------------

    class ScreenView : public QOpenGLWidget
    {
    public:
        ScreenView(const QSurfaceFormat& fmt, QWidget* parent,
                   QOpenGLWidget* share, Qt::WindowFlags flags)
            : QOpenGLWidget(parent, flags)
        {
            // NOTE_QT: setAutoBUfferSwap does not exist anymore.
            // setAutoBufferSwap(false);

            m_shared = share;

            setFormat(fmt);
        }

    protected:
        void initializeGL() override
        {
            TRACE_SCOPE("ScreenView::initializeGL")

            QOpenGLWidget::initializeGL();

            if (m_shared && context() && context()->isValid())
            {
                context()->setShareContext(m_shared->context());
            }
        }

        void paintGL() override
        {
            TRACE_SCOPE("ScreenView::paintGL")

            TWK_GLDEBUG_CTXNAME("ScreenView");

            TWK_GLDEBUG;
            //            glClearColor(0, 1, 0, 1);
            //            glClear(GL_COLOR_BUFFER_BIT);
            TWK_GLDEBUG;
        }

        ~ScreenView() {}

    private:
        QOpenGLWidget* m_shared = nullptr;
    };

    //----------------------------------------------------------------------

    QTDesktopVideoDevice::QTDesktopVideoDevice(VideoModule* m,
                                               const std::string& name,
                                               int screen,
                                               const QTGLVideoDevice* share)
        : DesktopVideoDevice(m, name, share)
        , m_screen(screen)
        , m_translator(0)
        , m_view(0)
        , m_x(0)
        , m_y(0)
    {
        const QList<QScreen*> screens = QGuiApplication::screens();
        if (screen < 0 || screen >= screens.size())
        {
            // Handle invalid screen index.
            // TODO_QT
            std::cout << "Screen index is out of range" << std::endl;
        }

        const QRect rect = screenGeometry();

        ostringstream str;
        str << rect.width() << " x " << rect.height();
        m_videoFormats.push_back(
            VideoFormat(rect.width(), rect.height(), 1.0, 1.0, 0.0, str.str()));
        addDefaultDataFormats();
    }

    QTDesktopVideoDevice::~QTDesktopVideoDevice() { close(); }

    void QTDesktopVideoDevice::setWidget(QOpenGLWidget* widget)
    {
        m_view = widget;
        m_translator = new QTTranslator(this, m_view);
    }

    void QTDesktopVideoDevice::makeCurrent() const
    {
        if (m_view)
            m_view->makeCurrent();
    }

    void QTDesktopVideoDevice::redraw() const
    {
        if (m_view)
        {
            ScopedLock lock(m_mutex);
            QSize s = m_view->size();
            m_view->update();
        }
    }

    void QTDesktopVideoDevice::redrawImmediately() const
    {
        if (m_view && m_view->isVisible())
        {
            ScopedLock lock(m_mutex);
            TWK_GLDEBUG;
            m_view->update();
            TWK_GLDEBUG;
        }
        else
        {
            TWK_GLDEBUG;
            redraw();
            TWK_GLDEBUG;
        }
    }

    void QTDesktopVideoDevice::syncBuffers() const
    {
        TRACE_SCOPE("QTDesktopVideoDevice::syncBuffers");
        if (m_view)
        {
            ScopedLock lock(m_mutex);
            //            TWK_GLDEBUG;
            makeCurrent();
            //           TWK_GLDEBUG;
            // NOTE_QT: swapBuffer is under the context now.
            m_view->context()->swapBuffers(m_view->context()->surface());
            //          TWK_GLDEBUG;
        }
    }

    void QTDesktopVideoDevice::open(const StringVector& args)
    {
        //
        //  always make the fullscreen device synced
        //

        // NOTE_QT: QSurfaceFormat replace QGLFormat.
        TWK_GLDEBUG;

        QSurfaceFormat fmt = shareDevice()->widget()->format();
        fmt.setSwapInterval(m_vsync ? 1 : 0);
        ScreenView* s =
            new ScreenView(fmt, 0, shareDevice()->widget(), Qt::Window);
        setWidget(s);

        TWK_GLDEBUG;

        auto viewDevice = new QTGLVideoDevice(0, "local view", s);
        setViewDevice(viewDevice);

        TWK_GLDEBUG;

        const QList<QScreen*> screens = QGuiApplication::screens();
        QRect g = screenGeometry();
        widget()->move(g.x(), g.y());
        widget()->setGeometry(g);

        if (useFullScreen())
            widget()->setWindowState(Qt::WindowFullScreen);
        else
            widget()->setWindowState(Qt::WindowNoState);

        TWK_GLDEBUG;

        widget()->show();

        TWK_GLDEBUG;

#ifdef PLATFORM_WINDOWS
        // Work around for QGLWidget::showFullScreen() windows specific issue
        // introduced in Qt 5.15.3
        widget()->setGeometry(g);
#endif
    }

    bool QTDesktopVideoDevice::isOpen() const { return m_view != 0; }

    void QTDesktopVideoDevice::close()
    {
        delete m_view;
        delete m_viewDevice;
        delete m_translator;
        m_view = 0;
        m_viewDevice = 0;
        m_translator = 0;
    }

    bool QTDesktopVideoDevice::useFullScreen() const
    {
#ifdef DEBUG_NO_FULLSCREEN
        return false;
#else
        return true;
#endif
    }

    QRect QTDesktopVideoDevice::screenGeometry() const
    {
        const QList<QScreen*> screens = QGuiApplication::screens();
        QRect g = screens[m_screen]->geometry();

#ifdef DEBUG_NO_FULLSCREEN
        // in the no-fullscreen debug mode, use 800x600 resolution in a simple
        // window.
        int offset = 30;
        g = QRect(g.x() + offset, g.y() + offset, 800, 600);
#endif
        return g;
    }

    VideoDevice::Resolution QTDesktopVideoDevice::resolution() const
    {
        QRect g = screenGeometry();

        return Resolution(g.width(), g.height(), 1.0, 1.0);
    }

    size_t QTDesktopVideoDevice::width() const
    {
        QRect g = screenGeometry();
        return g.width();
    }

    size_t QTDesktopVideoDevice::height() const
    {
        QRect g = screenGeometry();
        return g.height();
    }

    VideoDevice::VideoFormat QTDesktopVideoDevice::format() const
    {
        QRect g = screenGeometry();
        ostringstream str;
        str << g.width() << " x " << g.height();
        const float pa = 1.0;
        const float ps = 1.0;
        const float rate = 60.0;
        return VideoFormat(g.width(), g.height(), pa, ps, rate, str.str());
    }

    VideoDevice::Offset QTDesktopVideoDevice::offset() const
    {
        return Offset(0, 0);
    }

    VideoDevice::Timing QTDesktopVideoDevice::timing() const
    {
        return format();
    }

#ifdef PLATFORM_WINDOWS
    TwkApp::VideoDevice::ColorProfile QTDesktopVideoDevice::colorProfile() const
    {
        //
        // XXX The following steps are not Unicode safe
        //

        // Get the context for this screen
        const QList<QScreen*> screens = QGuiApplication::screens();

        // Ensure the screen index is valid.
        if (m_screen < 0 || m_screen >= screens.size())
        {
            m_colorProfile = ColorProfile();
            return m_colorProfile;
        }

        QScreen* targetScreen = screens[m_screen];
        QWindow* windowOnTargetScreen = nullptr;
        const QList<QWindow*> windows = QGuiApplication::topLevelWindows();

        // Check all windows to find one on the target screen.
        for (QWindow* window : windows)
        {
            if (window->screen() == targetScreen)
            {
                windowOnTargetScreen = window;
            }
        }

        if (!windowOnTargetScreen)
        {
            // Return empty profile if no window is found on the screen.
            m_colorProfile = ColorProfile();
            return m_colorProfile;
        }

        HWND hwnd = reinterpret_cast<HWND>(windowOnTargetScreen->winId());
        HDC hdc = GetDC(hwnd);

        if (hdc)
        {
            // Look for the profile path
            unsigned long pathLen;
            GetICMProfile(hdc, &pathLen, NULL);
            char* path = new char[pathLen];

            if (GetICMProfile(hdc, &pathLen, path))
            {
                // If we found a profile lets set the type,
                // url, and description

                m_colorProfile.type = ICCProfile;

                unsigned long maxLen = 2084;
                char* url = new char[maxLen];
                UrlCreateFromPath(path, url, &maxLen, NULL);
                m_colorProfile.url = url;

                char desc[256];
                cmsHPROFILE profile = cmsOpenProfileFromFile(path, "r");
                cmsGetProfileInfoASCII(profile, cmsInfoDescription, "en", "US",
                                       desc, 256);
                m_colorProfile.description = desc;

                delete url;
            }

            delete path;
            ReleaseDC(hwnd, hdc);
        }
        else
        {
            m_colorProfile = ColorProfile();
        }

        return m_colorProfile;
    }
#endif

    std::vector<VideoDevice*> QTDesktopVideoDevice::createDesktopVideoDevices(
        TwkApp::VideoModule* module, const QTGLVideoDevice* shareDevice)
    {
        std::vector<VideoDevice*> devices;

        const auto screens = QGuiApplication::screens();
        for (int screen = 0; screen < screens.size(); screen++)
        {
            const QScreen* w = screens[screen];
            QString name = QString("%1 %2 %3")
                               .arg(w->manufacturer())
                               .arg(w->model())
                               .arg(w->name());

            QTDesktopVideoDevice* sd = new QTDesktopVideoDevice(
                module, name.toUtf8().constData(), screen, shareDevice);

            devices.push_back(sd);
        }

        return devices;
    }
} // namespace Rv
