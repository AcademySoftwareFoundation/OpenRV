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

namespace Rv {

using namespace std;
using namespace TwkGLF;
using namespace TwkApp;

//----------------------------------------------------------------------

class ScreenView : public QOpenGLWidget
{
  public:
    ScreenView(const QSurfaceFormat& fmt,
               QWidget* parent,
               QOpenGLWidget* share,
               Qt::WindowFlags flags)
        : QOpenGLWidget(parent, flags)
        {
            // NOTE_QT: setAutoBUfferSwap does not exist anymore.
            //setAutoBufferSwap(false);
            setFormat(fmt);
            if (share)
            {
                context()->setShareContext(share->context());
            }
        }

        ~ScreenView() {}
    };

    //----------------------------------------------------------------------


QTDesktopVideoDevice::QTDesktopVideoDevice(VideoModule* m,
                                           const std::string& name,
                                           int screen,
                                           const QTGLVideoDevice* share)
    : DesktopVideoDevice(m, name, share),
      m_screen(screen),
      m_translator(0),
      m_view(0),
      m_x(0),
      m_y(0)
{
    const QList<QScreen*> screens = QGuiApplication::screens();
    if (screen < 0 || screen >= screens.size())
    {
        // Handle invalid screen index.
        // TODO_QT
        std::cout << "Screen index is out of range" << std::endl;
    }

    const QRect rect = screens[screen]->geometry();

    ostringstream str;
    str << rect.width() << " x " << rect.height();
    m_videoFormats.push_back(VideoFormat(rect.width(), rect.height(), 1.0, 1.0, 0.0, str.str()));
    addDefaultDataFormats();
}

QTDesktopVideoDevice::~QTDesktopVideoDevice()
{
    close();
}


void
QTDesktopVideoDevice::setWidget(QOpenGLWidget* widget)
{
    m_view = widget;
    m_translator = new QTTranslator(this, m_view);
}

void 
QTDesktopVideoDevice::makeCurrent() const 
{ 
    if (m_view) m_view->makeCurrent();
}

void 
QTDesktopVideoDevice::redraw() const
{
    if (m_view)
    {
        const QDesktopWidget* desktop = qApp->desktop();
        QRect rect = desktop->screenGeometry(screen);
        ostringstream str;
        str << rect.width() << " x " << rect.height();
        m_videoFormats.push_back(
            VideoFormat(rect.width(), rect.height(), 1.0, 1.0, 0.0, str.str()));
        addDefaultDataFormats();
    }

    QTDesktopVideoDevice::~QTDesktopVideoDevice() { close(); }

    void QTDesktopVideoDevice::setWidget(QGLWidget* widget)
    {
        ScopedLock lock(m_mutex);
        m_view->update();
    }

    void QTDesktopVideoDevice::makeCurrent() const
    {
        if (m_view)
            m_view->makeCurrent();
    }

    void QTDesktopVideoDevice::redraw() const
    {
        ScopedLock lock(m_mutex);
        makeCurrent();
        // NOTE_QT: swapBuffer is under the context now.
        m_view->context()->swapBuffers(m_view->context()->surface());
    }
}

void 
QTDesktopVideoDevice::open(const StringVector& args)
{
    //
    //  always make the fullscreen device synced
    //

    // NOTE_QT: QSurfaceFormat replace QGLFormat.

    QSurfaceFormat fmt = shareDevice()->widget()->format();
    fmt.setSwapInterval(m_vsync ? 1 : 0);
    ScreenView* s = new ScreenView(fmt, 0, shareDevice()->widget(), Qt::Window);
    setWidget(s);
    setViewDevice(new QTGLVideoDevice(0, "local view", s));

    const QList<QScreen*> screens = QGuiApplication::screens();
    QRect g = screens[m_screen]->geometry();
    widget()->move(g.x(), g.y());
    widget()->setWindowState(Qt::WindowFullScreen);
    widget()->show();

#ifdef PLATFORM_WINDOWS
    // Work around for QGLWidget::showFullScreen() windows specific issue
    // introduced in Qt 5.15.3
    widget()->setGeometry(g);
#endif
}

bool
QTDesktopVideoDevice::isOpen() const
{
    return m_view != 0;
}

void 
QTDesktopVideoDevice::close()
{
    delete m_view;
    delete m_viewDevice;
    delete m_translator;
    m_view = 0;
    m_viewDevice = 0;
    m_translator = 0;
}

VideoDevice::Resolution
QTDesktopVideoDevice::resolution() const
{
    const QList<QScreen*> screens = QGuiApplication::screens();
    QRect g = screens[m_screen]->geometry();
    return Resolution(g.width(), g.height(), 1.0, 1.0);
}

size_t 
QTDesktopVideoDevice::width() const
{
    const QList<QScreen*> screens = QGuiApplication::screens();
    QRect g = screens[m_screen]->geometry();
    return g.width();
}

size_t 
QTDesktopVideoDevice::height() const
{
    const QList<QScreen*> screens = QGuiApplication::screens();
    QRect g = screens[m_screen]->geometry();
    return g.height();
}

VideoDevice::VideoFormat
QTDesktopVideoDevice::format() const
{
    const QList<QScreen*> screens = QGuiApplication::screens();
    QRect g = screens[m_screen]->geometry();
    ostringstream str;
    str << g.width() << " x " << g.height();
    const float pa = 1.0;
    const float ps = 1.0;
    const float rate = 60.0;
    return VideoFormat(g.width(), g.height(), pa, ps, rate, str.str());
}

VideoDevice::Offset 
QTDesktopVideoDevice::offset() const
{
    return Offset(0,0);
}

VideoDevice::Timing 
QTDesktopVideoDevice::timing() const
{
    return format();
}

#ifdef PLATFORM_WINDOWS
TwkApp::VideoDevice::ColorProfile
QTDesktopVideoDevice::colorProfile() const
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
        GetICMProfile (hdc, &pathLen, NULL);
        char* path = new char[pathLen];

        if (GetICMProfile (hdc, &pathLen, path))
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
            m_view->updateGL();
        }
        else
        {
            redraw();
        }
    }

    void QTDesktopVideoDevice::syncBuffers() const
    {
        if (m_view)
        {
            ScopedLock lock(m_mutex);
            makeCurrent();
            m_view->swapBuffers();
        }
    }

    void QTDesktopVideoDevice::open(const StringVector& args)
    {
        //
        //  always make the fullscreen device synced
        //

        QGLFormat fmt = shareDevice()->widget()->format();
        fmt.setSwapInterval(m_vsync ? 1 : 0);
        ScreenView* s =
            new ScreenView(fmt, 0, shareDevice()->widget(), Qt::Window);
        setWidget(s);
        setViewDevice(new QTGLVideoDevice(0, "local view", s));

        QRect g = QApplication::desktop()->screenGeometry(m_screen);
        widget()->move(g.x(), g.y());
        widget()->setWindowState(Qt::WindowFullScreen);
        widget()->show();

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

    VideoDevice::Resolution QTDesktopVideoDevice::resolution() const
    {
        QRect g = QApplication::desktop()->screenGeometry(m_screen);
        return Resolution(g.width(), g.height(), 1.0, 1.0);
    }

    size_t QTDesktopVideoDevice::width() const
    {
        QRect g = QApplication::desktop()->screenGeometry(m_screen);
        return g.width();
    }

    size_t QTDesktopVideoDevice::height() const
    {
        QRect g = QApplication::desktop()->screenGeometry(m_screen);
        return g.height();
    }

    VideoDevice::VideoFormat QTDesktopVideoDevice::format() const
    {
        QRect g = QApplication::desktop()->screenGeometry(m_screen);
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
        QDesktopWidget* desktop = QApplication::desktop();
        WId id = desktop->screen(m_screen)->effectiveWinId();
        HDC hdc = GetDC(reinterpret_cast<HWND>(id));

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
            ReleaseDC(reinterpret_cast<HWND>(id), hdc);
        }
        else
        {
            m_colorProfile = ColorProfile();
        }

        delete path;
        ReleaseDC(hwnd, hdc);
    }
#endif

} // namespace Rv
