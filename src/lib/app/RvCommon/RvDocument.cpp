//
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//

#include <RvCommon/RvDocument.h>
#include <RvCommon/RvApplication.h>
#include <RvCommon/RvPreferences.h>
#include <RvCommon/RvProfileManager.h>
#include <RvCommon/RvSourceEditor.h>
#include <RvCommon/RvTopViewToolBar.h>
#include <RvCommon/RvBottomViewToolBar.h>
#include <RvCommon/DesktopVideoModule.h>
#include <RvCommon/DesktopVideoDevice.h>
#include <RvCommon/InitGL.h>
// #include <RvCommon/QTFrameBuffer.h>
#include <RvCommon/QTUtils.h>
#include <RvCommon/RvConsoleWindow.h>
#include <RvCommon/RvNetworkDialog.h>
#include <RvCommon/TwkQTAction.h>
#ifdef PLATFORM_WINDOWS
#include <GL/glew.h>
#endif
#include <RvCommon/GLView.h> // WINDOWS: include AFTER other stuff
#include <RvCommon/QTGLVideoDevice.h>
#include <QtGui/QtGui>
#include <QtNetwork/QtNetwork>
#include <IPCore/AudioRenderer.h>
#include <IPCore/ImageRenderer.h>
#include <RvApp/RvSession.h>
#include <IPCore/ImageRenderer.h>
#include <IPBaseNodes/SourceIPNode.h>
#include <TwkApp/Action.h>
#include <TwkApp/Application.h>
#include <TwkApp/Event.h>
#include <TwkApp/Menu.h>
#include <TwkMath/Vec4.h>
#include <TwkMath/Iostream.h>
#include <TwkDeploy/Deploy.h>
#include <TwkUtil/File.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stl_ext/string_algo.h>

#include <QtWidgets/QMessageBox>
#include <QtWidgets/QDesktopWidget>

#ifdef PLATFORM_LINUX
#include <QtX11Extras/QX11Info>
#include <X11/Xlib.h>
#endif

#ifdef PLATFORM_DARWIN
#include <RvCommon/DisplayLink.h>
#include <RvCommon/CGDesktopVideoDevice.h>
#endif

namespace Rv
{
    using namespace std;
    using namespace TwkMath;
    using namespace IPCore;

#if 0
#define DB_ICON 0x01
#define DB_ALL 0xff

//  #define DB_LEVEL        (DB_ALL & (~ DB_ICON))
#define DB_LEVEL DB_ALL

#define DB(x) cerr << x << endl
#define DBL(level, x)     \
    if (level & DB_LEVEL) \
    cerr << x << endl
#else
#define DB(x)
#define DBL(level, x)
#endif

    inline QString utf8(const std::string& us)
    {
        return QString::fromUtf8(us.c_str());
    }

    static int sessionCount = 0;

    namespace
    {
        bool workAroundActionLeak = false;
    };

    RvDocument::RvDocument()
        : QMainWindow()
        , TwkUtil::Notifier()
        , m_rvMenu(0)
        , m_lastPopupAction(0)
        , m_menuBarHeight(0)
        , m_mainPopup(0)
        , m_menuBarDisable(false)
        , m_menuBarShown(true)
        , m_startupResize(false)
        , m_aggressiveSizing(false)
        , m_menuTimer(0)
        , m_menuExecuting(0)
        , m_userMenu(0)
        , m_userPopup(0)
        , m_watcher(0)
        , m_resetPolicyTimer(0)
        , m_session(0)
        , m_currentlyClosing(false)
        , m_closeEventReceived(false)
        , m_vsyncDisabled(false)
        , m_oldGLView(0)
        , m_glView(0)
        , m_sourceEditor(0)
        , m_displayLink(0)
    {
        DB("RvDocument constructed");

        if (getenv("RV_WORKAROUND_ACTION_LEAK"))
        {
            workAroundActionLeak = true;
        }
        else
        {
#ifdef PLATFORM_DARWIN
            workAroundActionLeak = true;
#else
            workAroundActionLeak = false;
#endif
        }

#if !defined(PLATFORM_DARWIN)
        setMenuBar(new QMenuBar(0));
#endif

        const TwkApp::Application::Documents& docs = TwkApp::App()->documents();

        setWindowIcon(
            QIcon(qApp->applicationDirPath() + QString(RV_ICON_PATH_SUFFIX)));

        Rv::Options& opts = Options::sharedOptions();

        bool resetGLPrefs = false;
        m_startupResize =
            (opts.width == -1 && opts.height == -1 && opts.startupResize);

        checkDriverVSync();

        m_viewContainerWidget = new QWidget(this);
        m_centralWidget = new QWidget(m_viewContainerWidget);
        m_topViewToolBar = new RvTopViewToolBar(m_viewContainerWidget);
        m_bottomViewToolBar = new RvBottomViewToolBar(m_viewContainerWidget);
        QVBoxLayout* vlayout = new QVBoxLayout(m_viewContainerWidget);

        vlayout->addWidget(m_topViewToolBar);
        vlayout->addWidget(m_centralWidget);
        vlayout->addWidget(m_bottomViewToolBar);

        vlayout->setSpacing(0);
        vlayout->setContentsMargins(0, 0, 0, 0);

        m_topViewToolBar->makeActiveFromSettings();
        m_bottomViewToolBar->makeActiveFromSettings();

        //
        //

        if (docs.empty())
        {
            m_glView = new GLView(
                this, 0, this,
                opts.stereoMode && !strcmp(opts.stereoMode, "hardware"),
                opts.vsync != 0 && !m_vsyncDisabled, true, opts.dispRedBits,
                opts.dispGreenBits, opts.dispBlueBits, opts.dispAlphaBits,
                !m_startupResize);

            //
            //  Make the GL context valid ASAP so we can query it later
            //

            if (!m_glView->isValid())
            {
                delete m_glView;
                // cout << "ERROR: reverting to known GL state" << endl;
                // cout << "ERROR: check the rendering preferences" << endl;
                m_glView = new GLView(this, 0, this);
                resetGLPrefs = true;
            }
        }
        else
        {
            RvSession* s = static_cast<RvSession*>(docs.front());
            RvDocument* rvDoc = (RvDocument*)s->opaquePointer();
            m_glView = new GLView(
                this, rvDoc->view(), this,
                opts.stereoMode && !strcmp(opts.stereoMode, "hardware"),
                opts.vsync != 0 && !m_vsyncDisabled,
                true, // double buffer
                opts.dispRedBits, opts.dispGreenBits, opts.dispBlueBits,
                opts.dispAlphaBits, !m_startupResize);

            if (!m_glView->isValid())
            {
                delete m_glView;
                // cout << "ERROR: reverting to known GL state" << endl;
                // cout << "ERROR: check the rendering preferences" << endl;
                m_glView = new GLView(this, rvDoc->view(), this);
                resetGLPrefs = true;
            }
        }

        m_stackedLayout = new QStackedLayout(m_centralWidget);
        m_stackedLayout->setStackingMode(QStackedLayout::StackAll);
        m_stackedLayout->addWidget(m_glView);

        setCentralWidget(m_viewContainerWidget);

        // QTFrameBuffer* fb = m_glView->frameBuffer();
        m_glView->setFocus(Qt::OtherFocusReason);
        // qApp->installEventFilter(m_glView);

        m_resetPolicyTimer = new QTimer(this);
        m_resetPolicyTimer->setSingleShot(true);
        connect(m_resetPolicyTimer, SIGNAL(timeout()), this,
                SLOT(resetSizePolicy()));

        m_frameChangedTimer = new QTimer(this);
        m_frameChangedTimer->setSingleShot(true);
        connect(m_frameChangedTimer, SIGNAL(timeout()), this,
                SLOT(frameChanged()));

        m_menuTimer = new QTimer(this);
        m_menuTimer->setInterval(50);
        connect(m_menuTimer, SIGNAL(timeout()), this, SLOT(buildMenu()));

        m_glView->makeCurrent();
        initializeGLExtensions();

        m_session = new RvSession;
        // m_session->setFrameBuffer(fb);

#ifdef PLATFORM_DARWIN
        m_displayLink = new DisplayLink(this);
        if (m_displayLink)
            m_session->useExternalVSyncTiming(true);
#endif

        // RvApp()->addVideoDevice(m_glView->videoDevice());
        m_session->setControlVideoDevice(m_glView->videoDevice());

        m_session->setRendererType("Composite");
        m_session->setOpaquePointer(this);
        m_session->makeActive();
        m_session->postInitialize();
        char nm[64];
        sprintf(nm, "session%03d", sessionCount++);
        m_session->setEventNodeName(nm);
        setObjectName(QString("rv-") + QString(nm));

        mergeMenu(m_session->menu());

        setAttribute(Qt::WA_DeleteOnClose);
        setAttribute(Qt::WA_QuitOnClose);

        m_topViewToolBar->setSession(m_session);
        m_bottomViewToolBar->setSession(m_session);

        // 10.5 looks better without it (no longer has the 100% white top)
        // setAttribute(Qt::WA_MacBrushedMetal);

        m_session->addNotification(this, IPCore::Session::updateMessage());
        m_session->addNotification(this,
                                   IPCore::Session::updateLoadingMessage());
        m_session->addNotification(this,
                                   IPCore::Session::fullScreenOnMessage());
        m_session->addNotification(this,
                                   IPCore::Session::fullScreenOffMessage());
        m_session->addNotification(this,
                                   IPCore::Session::stereoHardwareOnMessage());
        m_session->addNotification(this,
                                   IPCore::Session::stereoHardwareOffMessage());
        m_session->addNotification(this, TwkApp::Document::deleteMessage());
        m_session->addNotification(this,
                                   TwkApp::Document::menuChangedMessage());
        m_session->addNotification(this, TwkApp::Document::activeMessage());
        m_session->addNotification(this,
                                   TwkApp::Document::filenameChangedMessage());
        m_session->addNotification(this,
                                   IPCore::Session::audioUnavailbleMessage());
        m_session->addNotification(
            this, IPCore::Session::eventDeviceChangedMessage());
        m_session->addNotification(this, IPCore::Session::stopPlayMessage());

        m_session->playStartSignal().connect(boost::bind(
            &RvDocument::playStartSlot, this, std::placeholders::_1));
        m_session->playStopSignal().connect(boost::bind(
            &RvDocument::playStopSlot, this, std::placeholders::_1));
        m_session->physicalVideoDeviceChangedSignal().connect(
            boost::bind(&RvDocument::physicalVideoDeviceChangedSlot, this,
                        std::placeholders::_1));

        if (resetGLPrefs)
            resetGLStateAndPrefs();

#ifdef PLATFORM_LINUX

        int op_ret, ev_ret, er_ret;

        bool haveNV = XQueryExtension(QX11Info::display(), "NV-GLX", &op_ret,
                                      &ev_ret, &er_ret);

        if (!haveNV)
        {
            cerr << endl;
            cerr << "ERROR:******* NV-GLX Extension Missing ***********"
                 << endl;
            cerr << "    If you're using an Nvidia card, please install"
                 << endl;
            cerr << "    the optimized NVIDIA binary driver." << endl;
            cerr << "    If you're using an ATI card, please be aware " << endl;
            cerr << "    that RV has not been tested with ATI cards." << endl;
            cerr << "**************************************************"
                 << endl;
            cerr << endl;
        }
#endif

        if (opts.noBorders)
        {
            Qt::WindowFlags flags = windowFlags();
            setWindowFlags(flags | Qt::FramelessWindowHint);
        }
    }

    RvDocument::~RvDocument()
    {
        if (IPCore::debugProfile)
        {
            Rv::Options& opts = Options::sharedOptions();
            // RvPreferences* prefs = RvApp()->prefDialog();
            // prefs->loadSettingsIntoOptions(opts);

            QString dt = QDateTime::currentDateTime().toString("yyMMddhhmmss");
            QString host = QHostInfo::localHostName();
            host.replace(".", "_");

            ostringstream out;
            out << "rv_";
            out << TWK_DEPLOY_MAJOR_VERSION();
            out << "_";
            out << TWK_DEPLOY_MINOR_VERSION();
            out << "_";
            out << TWK_DEPLOY_PATCH_LEVEL();
            out << "_";
            out << host.toUtf8().constData();
            out << "_";
            out << dt.toUtf8().constData();
            out << ".rvprof";

            ofstream file(UNICODE_C_STR(out.str().c_str()));

            if (file)
            {
                file << "# RV playback debug data" << endl;
                file << "# version " << TWK_DEPLOY_MAJOR_VERSION() << "."
                     << TWK_DEPLOY_MINOR_VERSION() << "."
                     << TWK_DEPLOY_PATCH_LEVEL() << endl;
                file << "# -------------------------------------------" << endl;
                file << "# PLATFORM = " << PLATFORM_STRING << endl;
                file << "# COMPILER = " << COMPILER_STRING << endl;
                file << "# ARCH = " << ARCH_STRING << endl;
                file << "# CXXFLAGS = " << CXXFLAGS_STRING << endl;
                file << "# HEAD = " << GIT_HEAD << endl;
                file << "# RELEASE = " << RELEASE_DESCRIPTION << endl;
                file << "# " << endl;
                file << "# options" << endl;

                opts.output(file);

                file << "# -------------------------------------------" << endl;

                m_session->dumpProfilingToFile(file);

                cout << "INFO: wrote profiling file" << endl;
            }
            else
            {
                cout << "ERROR: failed to write profiling file" << endl;
            }
        }

#ifdef PLATFORM_DARWIN
        if (m_displayLink && m_displayLink->isValid()
            && m_displayLink->isActive())
        {
            m_displayLink->stop();
            delete m_displayLink;
            m_displayLink = 0;
        }
#endif

        lazyDeleteGLView();

        purgeMenus();
        m_session->setOutputVideoDevice(0);
        m_session->setControlVideoDevice(0);
        m_session->removeNotification(this);

        if (m_sourceEditor)
            m_sourceEditor->hide();
        delete m_sourceEditor;

        if (RvNetworkDialog* d = RvApp()->networkWindow())
        {
            d->sessionDeleted(m_session->name().c_str());
        }

        m_currentlyClosing = true;

        if (RvApp()->isInPresentationMode())
        {
            RvApp()->setPresentationMode(false);
        }

        DB("RvDocument::~RvDocument docs " << RvApp()->documents().size());

        if (RvApp()->documents().size() == 1)
        {
            //
            //  Then this is the last document, so shutdown network
            //

            if (RvNetworkDialog* d = RvApp()->networkWindow())
            {
                if (d->serverRunning())
                    d->shutdownServer();
                d->close();
            }

            if (RvApp()->console())
                RvApp()->console()->close();
            if (RvApp()->prefDialog())
                RvApp()->prefDialog()->close();
            if (RvApp()->profileManager())
                RvApp()->profileManager()->close();

            RvSettings::cleanupGlobalSettings();
        }

        delete m_session;

        delete m_menuTimer;
        delete m_watcher;
        delete m_userMenu;
        delete m_userPopup;
        m_session = 0;
    }

    QMenuBar* RvDocument::mb()
    {
#if defined(PLATFORM_DARWIN)
        return RvApp()->macMenuBar();
#else
        return menuBar();
#endif
    }

    void RvDocument::physicalVideoDeviceChangedSlot(
        const TwkApp::VideoDevice* device)
    {
#ifdef PLATFORM_DARWIN
        if (m_displayLink && m_displayLink->isActive())
        {
            playStopSlot("");
            playStartSlot("");
        }
#endif
    }

    void RvDocument::playStartSlot(const std::string&)
    {
        //
        //  Unlike the notifier messages, the boost playStartSignal will
        //  only be emitted when playback is occuring. The notifier signal
        //  happens when ever updating (including animation) is
        //  happening. We only want DisplayLink with actual playback
        //

#ifdef PLATFORM_DARWIN
        if (CGDesktopVideoDevice* cgdevice =
                dynamic_cast<CGDesktopVideoDevice*>(
                    m_glView->videoDevice()->physicalDevice()))
        {
            if (m_displayLink)
                m_displayLink->start(m_session, cgdevice);
        }
#endif
    }

    void RvDocument::playStopSlot(const std::string&)
    {
#ifdef PLATFORM_DARWIN
        if (m_displayLink)
            m_displayLink->stop();
#endif
    }

    bool RvDocument::receive(Notifier* originator, Notifier* sender,
                             MessageId m, MessageData* data)
    {
        //
        //  Don't respond to messages if we're closing.
        //
        if (m_currentlyClosing)
            return true;

        if (m == IPCore::Session::updateLoadingMessage())
        {
            if (m_session->loadCount() == 0)
                setDocumentDisabled(true, true);
            m_session->update(5.0 /*5 secs*/,
                              true /* update in any playing mode*/);
            qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
            if (m_session->loadCount() >= m_session->loadTotal() - 1)
                setDocumentDisabled(false, false);
        }
        else if (m == IPCore::Session::updateMessage())
        {
            view()->videoDevice()->redraw();
        }
        else if (m == IPCore::Session::eventDeviceChangedMessage())
        {
            if (m_session->eventVideoDevice() && m_glView->videoDevice())
            {
                m_glView->videoDevice()->translator().setRelativeDomain(
                    m_session->eventVideoDevice()->width(),
                    m_session->eventVideoDevice()->height());
            }
        }
        else if (m == TwkApp::Document::filenameChangedMessage())
        {
            string t = stl_ext::basename(m_session->fileName());
            setWindowTitle(utf8(t));
        }
        else if (m == IPCore::Session::fullScreenOnMessage())
        {
            toggleFullscreen();
        }
        else if (m == IPCore::Session::fullScreenOffMessage())
        {
            toggleFullscreen();
        }
        else if (m == IPCore::Session::stereoHardwareOffMessage())
        {
            setStereo(false);
        }
        else if (m == IPCore::Session::stereoHardwareOnMessage())
        {
            setStereo(true);
        }
        else if (m == TwkApp::Document::deleteMessage())
        {
            m_session = 0;
        }
        else if (m == TwkApp::Document::menuChangedMessage())
        {
            if (!m_menuExecuting)
                mergeMenu(m_session->menu());
            else
                setBuildMenu();
        }
        else if (m == TwkApp::Document::activeMessage())
        {
#if defined(PLATFORM_DARWIN)
            //
            //  On Darwin, we may need to rebuild the main menu when
            //  focus changes, because different sesssions can have
            //  different menu structures.  On other OSes, it's a
            //  waste of time.
            //
            static RvDocument* lastActiveDocument = this;
            if (lastActiveDocument != this)
            {
                lastActiveDocument = this;
                setBuildMenu();
            }
#endif
            m_glView->setFocus(Qt::OtherFocusReason);
        }
        else if (m == IPCore::Session::audioUnavailbleMessage())
        {
            QMessageBox box(this);
            box.setWindowTitle(tr(UI_APPLICATION_NAME ": Audio Failure"));
            QString baseText = tr("Audio Device is Currently Unavailable");
            QString detailedText =
                tr("Another program (maybe another copy of RV?) "
                   "is blocking use of the audio device. "
                   "If you quit the other program you may be "
                   "able to use the audio again if you restart RV.\n "
                   "You might also try other audio devices in the preferences "
                   "audio tab. "
#ifdef PLATFORM_LINUX
                   "For example, the ALSA \"default\" and \"dmix\" "
                   "devices allow sharing of audio."
#endif
                );
            box.setText(baseText + "\n\n" + detailedText);
            box.setWindowModality(Qt::WindowModal);
            // QPushButton* b1 = box.addButton(tr("Keep Trying"),
            // QMessageBox::AcceptRole);
            QPushButton* b2 =
                box.addButton(tr("Turn Off Audio"), QMessageBox::AcceptRole);

#ifdef PLATFORM_LINUX
            // Show the RV Icon-- otherwise the user has no idea where
            // this came from if RV isn't up yet.
            box.setIconPixmap(QPixmap(qApp->applicationDirPath()
                                      + QString(RV_ICON_PATH_SUFFIX))
                                  .scaledToHeight(64));
#else
            box.setIcon(QMessageBox::Critical);
#endif

            box.exec();

            if (box.clickedButton() == b2)
            {
                IPCore::AudioRenderer::setAudioNever(true);
            }
            // else if (box.clickedButton() == b1)
            // {
            //     Rv::AudioRenderer::setNoAudio(true);
            // }

            IPCore::AudioRenderer::reset();
        }

        return true;
    }

    void RvDocument::resetGLStateAndPrefs()
    {
        QMessageBox box(this);
        box.setWindowTitle(
            tr(UI_APPLICATION_NAME ": Invalid Display Configuration"));
        QString baseText = tr("Display Configuration is Invalid");
        QString detailedText =
            tr("The display configuration in the preferences is\n"
               "incompatible with this driver and/or graphics "
               "hardware.\n" UI_APPLICATION_NAME
               " will use the default display configuration to continue.\n"
#ifdef PLATFORM_LINUX
               "You can check available display possibilities\n"
               "using the glxinfo command with the -t option from a shell\n"
               "or use the driver configuration UI.\n"
#endif
               "You have a choice: reset the preference automatically\n"
               "or use the Rendering preferences to change it manually.\n");

        box.setText(baseText + "\n\n" + detailedText);
        box.setWindowModality(Qt::WindowModal);
        QPushButton* b1 =
            box.addButton(tr("Reset Automatically"), QMessageBox::RejectRole);
        QPushButton* b2 = box.addButton(tr("Change Preferences Manually"),
                                        QMessageBox::AcceptRole);

#ifdef PLATFORM_LINUX
        // Show the RV Icon-- otherwise the user has no idea where
        // this came from if RV isn't up yet.
        box.setIconPixmap(
            QPixmap(qApp->applicationDirPath() + QString(RV_ICON_PATH_SUFFIX))
                .scaledToHeight(64));
#else
        box.setIcon(QMessageBox::Critical);
#endif

        box.exec();

        if (box.clickedButton() == b2)
        {
            RvApp()->prefs();
            RvApp()->prefDialog()->tabWidget()->setCurrentIndex(2);
            RvApp()->prefDialog()->raise();
        }
        else
        {
            Rv::Options& opts = Options::sharedOptions();
            opts.dispRedBits = 0;
            opts.dispGreenBits = 0;
            opts.dispBlueBits = 0;
            opts.dispAlphaBits = 0;

            {
                RV_QSETTINGS;
                settings.beginGroup("Display");
                settings.setValue("dispRedBits", 0);
                settings.setValue("dispGreenBits", 0);
                settings.setValue("dispBlueBits", 0);
                settings.setValue("dispAlphaBits", 0);
                settings.endGroup();
            }

            RvApp()->prefDialog()->update();
            RvApp()->prefDialog()->write();
            RvApp()->prefDialog()->raise();
        }
    }

    void RvDocument::enableActions(bool b, QMenu* menu)
    {
        QList<QAction*> actions = menu->actions();

        for (size_t i = 0; i < actions.size(); i++)
        {
            QAction* a = actions[i];
            if (a->menu())
                enableActions(b, a->menu());

            a->setEnabled(b);
        }
    }

    void RvDocument::lazyDeleteGLView()
    {
        delete m_oldGLView;
        m_oldGLView = 0;
    }

    void RvDocument::resetSizePolicy()
    {
        m_glView->setMinimumContentSize(64, 64);
        m_glView->setMinimumSize(QSize(64, 64));
        m_glView->setSizePolicy(
            QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
    }

    void RvDocument::setDocumentDisabled(bool b, bool menuBarOnly)
    {
        // if (!menuBarOnly) setDisabled(b);

        m_menuBarDisable = b;

        QList<QAction*> actions = mb()->actions();
        for (size_t i = 0; i < actions.size(); i++)
        {
            QAction* a = actions[i];
            if (a->menu())
                enableActions(!b, a->menu());
            a->setEnabled(!b);
        }
    }

    void RvDocument::rebuildGLView(bool stereo, bool vsync, bool doubleBuffer,
                                   int red, int green, int blue, int alpha)
    {
        //
        //  On the mac, we need to carefully replace the content GL widget so
        //  that Qt doesn't resize the dock widgets and everything
        //  else. Unforunately, when the content widget was the GLView and we
        //  put a new one in, we had no core dump. When we do this manually and
        //  delete the old widget it dumps core. The problem seems to be random
        //  events which are going to be sent this event processing round.
        //
        //  The workaround is to lazily delete the old GLView. I have a feeling
        //  the main window class does something similar itself which is why
        //  our code didn't dump core before. At the bottom of this function
        //  there is a temp timer that fires in 100ms to delete it. Or if this
        //  function is called again before that time its deleted.
        //

        lazyDeleteGLView();

        GLView* oldGLView = m_glView;
        Qt::KeyboardModifiers cur =
            m_glView->videoDevice()->translator().currentModifiers();
        oldGLView->stopProcessingEvents();

        GLView* newGLView = new GLView(this, view(), this, stereo, vsync,
                                       doubleBuffer, red, green, blue, alpha);

        newGLView->setContentSize(oldGLView->sizeHint().width(),
                                  oldGLView->sizeHint().height());

        newGLView->setMinimumSize(oldGLView->minimumSizeHint().width(),
                                  oldGLView->minimumSizeHint().height());

        bool resetGLPrefs = false;

        size_t ow = centralWidget()->width();
        size_t oh = centralWidget()->height();

        if (!newGLView->isValid())
        {
            delete newGLView;
            newGLView = new GLView(this, view(), this);
            resetGLPrefs = true;
        }

        m_stackedLayout->addWidget(newGLView);
        m_stackedLayout->removeWidget(oldGLView);
        m_glView = newGLView;
        m_glView->show();
        m_glView->setFocus(Qt::OtherFocusReason);

        m_topViewToolBar->setDevice(m_glView->videoDevice());

        bool same =
            m_session->outputVideoDevice() == m_session->controlVideoDevice();
        m_session->setEventVideoDevice(0);
        m_session->setOutputVideoDevice(0);
        m_session->setControlVideoDevice(m_glView->videoDevice());
        if (same)
            m_session->setOutputVideoDevice(m_glView->videoDevice());

        m_glView->videoDevice()->sendEvent(TwkApp::RenderContextChangeEvent(
            "gl-context-changed", m_glView->videoDevice()));

        if (resetGLPrefs)
            resetGLStateAndPrefs();

        if (DesktopVideoModule* m = RvApp()->desktopVideoModule())
        {
            const TwkApp::VideoModule::VideoDevices& devices = m->devices();

            for (size_t i = 0; i < devices.size(); i++)
            {
                if (DesktopVideoDevice* d =
                        dynamic_cast<DesktopVideoDevice*>(devices[i]))
                {
                    d->setShareDevice(m_glView->videoDevice());
                }
            }
        }

        m_glView->videoDevice()->translator().setCurrentModifiers(cur);
        m_oldGLView = oldGLView;
        m_oldGLView->hide();
        QTimer::singleShot(100, this, SLOT(lazyDeleteGLView()));
    }

    void RvDocument::setStereo(bool b)
    {
        const bool vsync = m_glView->format().swapInterval() == 1;
        const bool stereo = m_glView->format().stereo();
        const bool dbl = m_glView->format().doubleBuffer();
        const int red = m_glView->format().redBufferSize();
        const int green = m_glView->format().greenBufferSize();
        const int blue = m_glView->format().blueBufferSize();
        const int alpha = m_glView->format().alphaBufferSize();
        if (b != stereo)
            rebuildGLView(b, vsync, dbl, red, green, blue, alpha);
    }

    void RvDocument::setVSync(bool b)
    {
        if (m_vsyncDisabled)
            return;
        const bool vsync = m_glView->format().swapInterval() == 1;
        const bool stereo = m_glView->format().stereo();
        const bool dbl = m_glView->format().doubleBuffer();
        const int red = m_glView->format().redBufferSize();
        const int green = m_glView->format().greenBufferSize();
        const int blue = m_glView->format().blueBufferSize();
        const int alpha = m_glView->format().alphaBufferSize();
        if (b != vsync)
            rebuildGLView(stereo, b, dbl, red, green, blue, alpha);
    }

    void RvDocument::setDoubleBuffer(bool b)
    {
        bool vsync = m_glView->format().swapInterval() == 1;
        const bool stereo = m_glView->format().stereo();
        const int red = m_glView->format().redBufferSize();
        const int green = m_glView->format().greenBufferSize();
        const int blue = m_glView->format().blueBufferSize();
        const int alpha = m_glView->format().alphaBufferSize();

        if (!b)
            vsync = false;

        rebuildGLView(stereo, vsync, b, red, green, blue, alpha);
    }

    void RvDocument::setDisplayOutput(DisplayOutputType type)
    {
        const bool vsync = m_glView->format().swapInterval() == 1;
        const bool stereo = m_glView->format().stereo();
        const bool dbl = m_glView->format().doubleBuffer();
        int red = m_glView->format().redBufferSize();
        int green = m_glView->format().greenBufferSize();
        int blue = m_glView->format().blueBufferSize();
        int alpha = m_glView->format().alphaBufferSize();

        switch (type)
        {
        default:
        case OpenGLDefaultFormat:
            if (red == 0)
                return;
            red = 0;
            blue = 0;
            green = 0;
            alpha = 0;
            break;
        case OpenGL8888:
            if (red == 8)
                return;
            red = 8;
            blue = 8;
            green = 8;
            alpha = 8;
            break;
        case OpenGL1010102:
            if (red == 10)
                return;
            red = 10;
            green = 10;
            blue = 10;
            alpha = 2;
            break;
        }

        rebuildGLView(stereo, vsync, dbl, red, green, blue, alpha);
    }

    GLView* RvDocument::view() const { return m_glView; }

    void RvDocument::center()
    {
        QDesktopWidget* desktop = QApplication::desktop();
        int screen = desktop->screenNumber(this);
        QRect ssize = desktop->availableGeometry(screen);

        int sw = ssize.width();
        int sh = ssize.height();

        int x = ssize.left() + int((sw - frameGeometry().width()) / 2);
        int y = ssize.top() + int((sh - frameGeometry().height()) / 2);
        DB("center moving to x " << x << " y " << y);
        move(x, y);
    }

    void RvDocument::resizeView(int w, int h)
    {
        if (!m_session)
            return;

        //
        //  Fit the window on to the screen so that all of its pixels are
        //  visible. If the contents is smaller than the screen, fit the
        //  window to the content size. If not, fit the window to the
        //  screen and leave some margin on each side so that the user can
        //  move the window.
        //

#ifndef PLATFORM_LINUX
        const int oversizeMargin = 0;
#else
        const int oversizeMargin = 60;
#endif

        const int overlap = 20;

        //
        //  Get all top level windows so we can avoid placing this one on
        //  top of an existing one.
        //

        QDesktopWidget* desktop = QApplication::desktop();
        int screen = desktop->screenNumber(this);

        QRect ssize = desktop->availableGeometry(screen);
        QRect ts = desktop->screenGeometry(screen);
        float sw = float(ssize.width());
        float sh = float(ssize.height());

        if (h == 0)
            h = w;
        if (w == 0)
            w = h;
        if (w == 0)
        {
            w = 1024;
            h = 720;
        }

        m_session->userRenderEvent("layout");
        IPCore::Session::Margins margins =
            m_session->controlVideoDevice()->margins();
        float mw = int(margins.left + margins.right);
        float mh = int(margins.top + margins.bottom);

        sw -= mw;
        sh -= mh;

//
//  We'd like to compensate for the frame geometry (wm
//  decorations) in our size request, but values returned are
//  not reliable on linx.
//
#ifndef PLATFORM_LINUX
        int frameW = int(frameGeometry().width() - width());
        int frameH = int(frameGeometry().height() - height());
        DB("resizeView frame w " << frameW << " h " << frameH);

        sw -= frameW;
        sh -= frameH;
#endif

//
//  We'd like to compensate for the menubar height but it does
//  not report it's size reliably on linux.  On mac, we don't
//  care.
//
#ifdef PLATFORM_WINDOWS
        sh -= (menuBarShown()) ? mb()->height() : 0;
#endif

        float aspect = float(w) / float(h);

        if (w >= sw - oversizeMargin)
        {
            w = int(sw - oversizeMargin);
            if (!m_aggressiveSizing)
                h = int(float(w) / aspect);
        }

        if (h > sh - oversizeMargin)
        {
            h = int(sh - oversizeMargin);
            if (!m_aggressiveSizing)
                w = int(float(h) * aspect);
        }

        h += int(mh);
        w += int(mw);

        m_glView->setContentSize(w, h);
        m_glView->setMinimumContentSize(w, h);
        m_glView->updateGeometry();

        const int dh = m_glView->height() - h;
        const int dw = m_glView->width() - w;

        if (dh || dw)
        {
            resize(width() - dw, height() - dh);
            m_glView->setContentSize(w, h);
            m_glView->setMinimumContentSize(w, h);
            m_glView->updateGeometry();
        }

        m_resetPolicyTimer->start();
    }

    void RvDocument::resizeToFit(bool placement, bool firstTime)
    {
        DB("resizeToFit first " << firstTime);
        if (!m_session)
            return;

        //
        //  Fit the window on to the screen so that all of its pixels are
        //  visible. If the contents is smaller than the screen, fit the
        //  window to the content size. If not, fit the window to the
        //  screen and leave some margin on each side so that the user can
        //  move the window.
        //

#ifndef PLATFORM_LINUX
        const int oversizeMargin = 0;
#else
        const int oversizeMargin = 60;
#endif

        const int overlap = 20;

        //
        //  Get all top level windows so we can avoid placing this one on
        //  top of an existing one.
        //

        const TwkApp::Application::Documents& docs = RvApp()->documents();
        vector<RvDocument*> windows;

        for (unsigned int i = 0; i < docs.size(); i++)
        {
            windows.push_back(
                reinterpret_cast<RvDocument*>(docs[i]->opaquePointer()));
        }

        float w = 0;
        float h = 0;

        QDesktopWidget* desktop = QApplication::desktop();
        int screen = desktop->screenNumber(this);

        QRect ssize = desktop->availableGeometry(screen);
        DB("resizeToFit available geom w " << ssize.width() << " h "
                                           << ssize.height());
        QRect ts = desktop->screenGeometry(screen);
        float sw = float(ssize.width());
        float sh = float(ssize.height());

        /*
          if (m_session->graph().viewNode())
          {
          DB("resizeToFit using view node");
          IPNode *n = m_session->graph().viewNode();
          if (n->inputs().size()) { DB ("resizeToFit selecting input"); n =
          n->inputs()[0]; }

          IPNode::ImageStructureInfo info =
          n->imageStructureInfo(m_session->graph().contextForFrame(m_session->currentFrame()));
          w = int(info.width);
          h = int(info.height);
          DB ("resizeToFit node " << n->name() << " w " << w << " h " << h <<
          endl);
          }
        */
        if (m_session->rootNode())
        {
            DB("resizeToFit using root node");
            //
            //  Find source nodes in eval path
            //
            IPNode::MetaEvalInfoVector evalInfos;
            IPNode::MetaEvalClosestByTypeName collector(evalInfos,
                                                        "RVSourceGroup");
            m_session->rootNode()->metaEvaluate(
                m_session->graph().contextForFrame(m_session->currentFrame()),
                collector);
            if (evalInfos.size())
            {
                IPNode::ImageStructureInfo info =
                    evalInfos[0].node->imageStructureInfo(
                        m_session->graph().contextForFrame(
                            m_session->currentFrame()));
                w = int(info.width);
                h = int(info.height);
                DB("resizeToFit w " << info.width << " h " << info.height);
            }
        }
        else if (!m_session->isEmpty())
        {
            DB("resizeToFit using first source");
            IPGraph::NodeVector nodes;
            m_session->graph().findNodesByTypeName(nodes, "RVSourceGroup");

            if (!nodes.empty())
            {
                IPNode* node = nodes.front();
                IPNode::ImageRangeInfo range = nodes.front()->imageRangeInfo();
                IPNode::ImageStructureInfo info = node->imageStructureInfo(
                    m_session->graph().contextForFrame(range.start));
                w = int(info.width);
                h = int(info.height);
                DB("resizeToFit source info w " << info.width << " h "
                                                << info.height);
            }
        }
        if (w <= 0 || h <= 0)
        {
            DB("resizeToFit bad initial dimensions: w " << w << " h " << h);
            w = 854;
            h = 480;
        }
        DB("resizeToFit initial w " << w << " h " << h);

        if (h == 0)
            h = w;
        if (w == 0)
            w = h;

        if (w == 0)
        {
            w = 854;
            h = 480;
        }

        m_session->userRenderEvent("layout");
        Session::Margins margins = m_glView->videoDevice()->margins();
        float mw = int(margins.left + margins.right);
        float mh = int(margins.top + margins.bottom);

        ensurePolished();

        sw -= mw;
        sh -= mh;

        //
        //  We'd like to compensate for the frame geometry (wm
        //  decorations) in our size request, but values returned are
        //  not reliable on linx.
        //
#ifndef PLATFORM_LINUX
        int frameW = int(frameGeometry().width() - width());
        int frameH = int(frameGeometry().height() - height());
        DB("resizeToFit frame w " << frameW << " h " << frameH);

        sw -= frameW;
        sh -= frameH;
#endif

        //
        //  We'd like to compensate for the presence of the top and/or
        //  bottom toolbars as well.
        //
        if (m_topViewToolBar->isVisible())
        {
            sh -= m_topViewToolBar->height();
        }
        if (m_bottomViewToolBar->isVisible())
        {
            sh -= m_bottomViewToolBar->height();
        }

        //
        //  We'd like to compensate for the menubar height but it does
        //  not report it's size reliably on linux.  On mac, we don't
        //  care.
        //
        mb()->ensurePolished();
        DB("resizeToFit mb()->height() " << mb()->height());
#ifdef PLATFORM_WINDOWS
        sh -= (menuBarShown()) ? mb()->height() : 0;
        // sh -= (menuBarShown()) ? 26 : 0;
#endif

        float aspect = float(w) / float(h);

        if (w >= sw - oversizeMargin)
        {
            w = sw - oversizeMargin;
            if (!m_aggressiveSizing)
                h = w / aspect;
        }

        if (h > sh - oversizeMargin)
        {
            h = sh - oversizeMargin;
            if (!m_aggressiveSizing)
                w = h * aspect;
        }

        h += mh;
        w += mw;

        DB("resizeToFit final target w " << w << " h " << h);

        m_glView->setContentSize(int(w), int(h));
        m_glView->setMinimumContentSize(int(w), int(h));
        m_glView->updateGeometry();

        DB("resizeToFit resulting size w " << m_glView->width() << " h "
                                           << m_glView->height());

        const int dh = m_glView->height() - int(h);
        const int dw = m_glView->width() - int(w);

        //
        //  WHY? Dunno
        //

        DB("resizeToFit dw " << dw << " dh " << dh);
        if (!firstTime && (dh || dw))
        // if ((dh || dw))
        {
            resize(width() - dw, height() - dh);
            m_glView->setContentSize(int(w), int(h));
            m_glView->setMinimumContentSize(int(w), int(h));
            m_glView->updateGeometry();
        }
        DB("resizeToFit final resulting size w " << m_glView->width() << " h "
                                                 << m_glView->height());

        m_resetPolicyTimer->start();

        if (placement)
        {
            int x = ssize.left() + int((sw - w) / 2);
            int y = ssize.top() + int((sh - h) / 2);

            //
            //  Keep moving the window until its in a decent spot. This may
            //  move it to a partially occluded position, but it will not
            //  cover an existing window's frame. This mimics (kind-of) what
            //  Cocoa does.
            //

            for (int i = 0; i < windows.size(); i++)
            {
                RvDocument* d = windows[i];

                if (d != this)
                {
                    int dx = d->x() - x;
                    int dy = d->y() - y;

                    if (abs(dx) < overlap || abs(dy) < overlap)
                    {
                        x += overlap;
                        y += overlap;
                        int i = -1; // start over
                    }
                }
            }

            x = max(ssize.left(), x);
            // y = max (frameH, y);
            y = max(ssize.top(), y);
            DB("resizeToFit, moving to " << x << " " << y);
            move(x, y);
        }
    }

    void RvDocument::toggleFullscreen(bool firstTime)
    {
        DB("toggleFullScreen currently " << isFullScreen());
        QDesktopWidget* desktop = QApplication::desktop();
        int screen = desktop->screenNumber(this);
        QRect ssize = desktop->screenGeometry(screen);
        int sw = ssize.width();
        int sh = ssize.height();

        resetSizePolicy();

        if (m_topViewToolBar)
        {
            m_topViewToolBar->setFullscreen(!isFullScreen());
        }

        if (isFullScreen())
        {
            ssize = desktop->availableGeometry(screen);
            showNormal();
        }
        else
        {
            if (firstTime)
            {
                setWindowState(windowState() ^ Qt::WindowFullScreen);
            }
            else
            {
                showFullScreen();
                setAcceptDrops(true);
            }
        }

        m_glView->setFocus(Qt::OtherFocusReason);
        activateWindow();
        raise();
        //
        //  On windows, sometimes returning from full-screen leaves us
        //  with a black screen unless we do the below.
        //
        m_session->askForRedraw();
    }

    QRect RvDocument::childrenRect()
    {
        QRect mr = mb()->geometry();
        QRect vr = m_glView->geometry();

        DB("RvDocument::childrenRect mb"
           << " shown " << menuBarShown() << " vis " << mb()->isVisible()
           << " w" << mr.width() << " h " << mr.height()
           << " view "
              " w"
           << vr.width() << " vh " << vr.height());

        QRect r = QMainWindow::childrenRect();
        DB("RvDocument::childrenRect r w " << r.width() << " h " << r.height());

        //
        //  We don't want the menubar to drive the width of the window.
        //  And on macos, it doesn't come into it at all.
        //
        r.setWidth(vr.width());
#ifndef PLATFORM_DARWIN
        r.setHeight((menuBarShown()) ? (mr.height() + vr.height())
                                     : vr.height());
#else
        r.setHeight(vr.height());
#endif
        return r;
    }

    void RvDocument::toggleMenuBar()
    {
        DB("RvDocument::toggleMenuBar, currently " << m_menuBarShown);
        //
        //  Can't use "isVisible()" here, because visible state depends
        //  on the state of the mb's ancestors.
        //
        if (m_menuBarShown)
        {
            // m_menuBarHeight = mb()->height();
            mb()->hide();
            m_menuBarShown = false;
            // if (!isFullScreen()) resize(ww, wh - m_menuBarHeight);
        }
        else
        {
            mb()->show();
            m_menuBarShown = true;
            // if (!isFullScreen()) resize(ww, wh + m_menuBarHeight);
        }
    }

    //----------------------------------------------------------------------
    //
    //  Menus
    //

    void RvDocument::menuActivated()
    {
        m_menuExecuting++;

        if (!m_menuBarDisable && isEnabled())
        {
            if (TwkQTAction* a = dynamic_cast<TwkQTAction*>(sender()))
            {
                if (const QAction* qa = dynamic_cast<const QAction*>(sender()))
                {
                    m_lastPopupAction = qa;
                }

                //
                //  There can be no action if the user set it to nil.
                //  In that case just ignore it.
                //

                if (const TwkApp::Action* action = a->item()->action())
                {
                    //
                    //  Won't throw
                    //

                    a->doc()->session()->makeCurrentSession();
                    action->execute(session(), TwkApp::Event("menu", 0));
                }
            }
            else
            {
                cout << "ERROR: menuActivated() failed to get TwkQTAction"
                     << endl;
            }
        }

        m_menuExecuting--;
    }

    void RvDocument::aboutToShowMenu()
    {
        if (QMenu* menu = dynamic_cast<QMenu*>(sender()))
        {
            QList<QAction*> actions = menu->actions();

            for (int i = 0; i < actions.size(); i++)
            {
                if (TwkQTAction* a = dynamic_cast<TwkQTAction*>(actions.at(i)))
                {
                    TwkApp::Menu::Item* item = (TwkApp::Menu::Item*)a->item();
                    TwkApp::Menu::StateFunc* S = item->stateFunc();

                    if (item && ((S && !S->error()) || !S))
                    {
                        Session* d =
                            a->doc()
                                ->session(); // TwkApp::Document::activeDocument();
                        d->makeCurrentSession();
                        int state = S ? (S->error() ? -1 : S->state()) : 0;

                        switch (state)
                        {
                        case 0:
                        case 1:
                            a->setEnabled(true);
                            a->setChecked(false);
                            a->setCheckable(false);
                            break;
                        case 2:
                            a->setEnabled(true);
                            a->setCheckable(true);
                            a->setChecked(true);
                            break;
                        case 3:
                            a->setEnabled(true);
                            //[nsitem setState: NSMixedState];
                            break;
                        case -1:
                            a->setEnabled(false);
                            break;
                        }

                        if (S && S->error())
                        {
                            a->setEnabled(false);
                            a->setIcon(
                                colorAdjustedIcon(":images/del_32x32.png"));
                            a->setText(QString("ERROR : ") + a->text());
                        }
                    }
                }
            }
        }
        else
        {
        }
    }

    class runningTimer
    {
    public:
        void init()
        {
            goTime = totalTime = 0.0;
            t.start();
        };

        void go() { goTime = t.elapsed(); };

        void stop() { totalTime += t.elapsed() - goTime; };

        float total() { return totalTime; };

        TwkUtil::Timer t;
        float goTime;
        float totalTime;
    };

    runningTimer rt;

    static string buildShortcutString(string keyText)
    {
        string ret;

        if (keyText.find(' ') != string::npos)
        {
            vector<string> tokens;
            stl_ext::tokenize(tokens, keyText);
            bool control = false;
            bool meta = false;
            bool shift = false;
            bool alt = false;
            bool rarrow = false;
            bool larrow = false;
            bool uarrow = false;
            bool darrow = false;
            string key;

            for (int i = 0; i < tokens.size(); i++)
            {
                const string& t = tokens[i];

                // #ifdef PLATFORM_DARWIN
                //                     if      (t == "control") meta    = true;
                //                     else if (t == "command") control = true;
                // #else
                if (t == "command")
                    meta = true;
                else if (t == "control")
                    control = true;
                // #endif

                else if (t == "alt")
                    alt = true;
                else if (t == "shift")
                    shift = true;
                else if (t == "meta")
                    meta = true;
                else if (t == "rightArrow")
                    rarrow = true;
                else if (t == "leftArrow")
                    larrow = true;
                else if (t == "upArrow")
                    uarrow = true;
                else if (t == "downArrow")
                    darrow = true;
                else
                {
                    for (int j = i; j < tokens.size(); ++j)
                    {
                        if (j != i)
                            key += " ";
                        key += tokens[j];
                    }
                    break;
                }
            }

            if (control)
                ret += "Ctrl+";
            if (meta)
                ret += "Meta+";
            if (alt)
                ret += "Alt+";
            if (shift || (key.size() == 1 && isupper(key[0])))
                ret += "Shift+";

            if (larrow)
                ret += "Left";
            else if (rarrow)
                ret += "Right";
            else if (uarrow)
                ret += "Up";
            else if (darrow)
                ret += "Down";
            else
                ret += key;
        }
        else
        {
            if (keyText.size() == 1 && isupper(keyText[0]))
            {
                ret = "Shift+" + keyText;
            }
            else
                ret = keyText;
        }

        return ret;
    }

    void RvDocument::convert(QMenu* qmenu, const TwkApp::Menu* menu,
                             bool shortcuts)
    {
        for (int i = 0; i < menu->items().size(); i++)
        {
            const TwkApp::Menu::Item* item = menu->items()[i];

            if (item->subMenu())
            {
                QMenu* subMenu = qmenu->addMenu(utf8(item->title()));
                subMenu->setFont(qmenu->font());
                //  rt.go();
                connect(subMenu, SIGNAL(aboutToShow()), this,
                        SLOT(aboutToShowMenu()));
                //  rt.stop();
                convert(subMenu, item->subMenu(), shortcuts);
                subMenu->menuAction()->setMenuRole(QAction::NoRole);
            }
            else if (item->title() == "_")
            {
                qmenu->addSeparator();
            }
            else
            {
                TwkQTAction* a = new TwkQTAction(item, this, qmenu);

                if ((item->stateFunc() && item->stateFunc()->error())
                    || (item->action() && item->action()->error()))
                {
                    a->setText(QString("ERROR : ") + utf8(item->title()));
                    a->setIcon(colorAdjustedIcon(":images/del_32x32.png"));
                    a->setEnabled(false);
                }
                else
                {
                    a->setText(utf8(item->title()));
                }

                if (shortcuts)
                {
                    string sc = buildShortcutString(item->key());
                    a->setShortcut(QString(sc.c_str()));
                    if (sc.size() == 1
                        && QString("") == a->shortcut().toString())
                    {
                        //
                        //  setShortcut takes a QKeySequence, and when
                        //  we make one out of a single key in a
                        //  Qstring, it doesn't work for some keys (like
                        //  "`", so we give it the key instead.  This
                        //  seems to work for all ascii keys.
                        //
                        a->setShortcut(sc[0]);
                    }
                }

                //  rt.go();
                connect(a, SIGNAL(triggered()), this, SLOT(menuActivated()));
                //  rt.stop();
                a->setMenuRole(QAction::NoRole);
                qmenu->addAction(a);
            }
        }
    }

    void RvDocument::disconnectActions(const QList<QAction*>& actions)
    {
        for (int i = 0; i < actions.size(); ++i)
        {
            QAction* a = actions[i];

            if (QMenu* m = a->menu())
            {
                disconnectActions(m->actions());
                m->disconnect();
            }
            else
                a->disconnect();
        }
    }

    void RvDocument::purgeMenus()
    {
        //
        //  I could not find (in the git trail) any record of why we treat the
        //  mac differently here, but this difference results in what appears to
        //  be a memory leak on linux/windows, where the total number of QMenus
        //  and QActions increases without bound.  The below change eliminates
        //  the leak and unifies the behavior on all platforms.
        //
        // #ifdef PLATFORM_DARWIN
        //

        if (workAroundActionLeak)
        {
            QObjectList children = mb()->children(); // copy

            for (int i = 0; i < children.size(); i++)
            {
                //  Skip this menu if it is the "RV" menu (mac or not)
                //
                if (QMenu* menu = dynamic_cast<QMenu*>(children[i]))
                {
#if defined(PLATFORM_DARWIN)
                    if (menu != RvApp()->macRVMenu() && menu != m_rvMenu)
#else
                    if (menu != m_rvMenu)
#endif
                    {
                        delete menu;
                    }
                }
            }
        }
        // #else
        else
        {
            if (mb()->actions().size() > 0)
            {
                QList<QAction*> actionsMinusFirstOne = mb()->actions().mid(1);
                disconnectActions(actionsMinusFirstOne);
            }

            mb()->clear();
        }
        // #endif

        delete m_mainPopup;
        m_mainPopup = 0;
        m_lastPopupAction = 0;
    }

    void RvDocument::setBuildMenu() { m_menuTimer->start(); }

    void RvDocument::buildMenu()
    {
        if (!m_menuExecuting && m_session)
        {
            mergeMenu(m_session->menu());
            m_menuTimer->stop();
        }
    }

    void RvDocument::mergeMenu(const TwkApp::Menu* menu, bool shortcuts)
    {
        purgeMenus();

        if (!menu)
        {
            return;
        }

        //
        //  This is the only way to overcome the accelerators on the Mac. You
        //  just can't have any shortcuts. Even if the menu is disabled the
        //  shortcuts still function. This was a problem with the pure
        //  Cocoa version as well.
        //

        if (m_menuBarDisable)
            shortcuts = false;

#if !defined(PLATFORM_DARWIN)
        if (!m_rvMenu || !workAroundActionLeak)
        {
            m_rvMenu = mb()->addMenu(UI_APPLICATION_NAME);
            m_rvMenu->addAction(RvApp()->aboutAction());
            m_rvMenu->addSeparator();
            m_rvMenu->addAction(RvApp()->prefAction());
            m_rvMenu->addSeparator();
            if (RvApp()->networkAction())
                m_rvMenu->addAction(RvApp()->networkAction());
            m_rvMenu->addSeparator();
            m_rvMenu->addAction(RvApp()->quitAction());
            m_rvMenu->menuAction()->font().setBold(true);
            m_rvMenu->setObjectName(UI_APPLICATION_NAME " Menu");
        }
#endif

        if (m_mainPopup)
            m_mainPopup->clear();
        else
            m_mainPopup = new QMenu(this);

        m_mainPopup->setObjectName("Main Popup");

#ifdef PLATFORM_DARWIN
        QFont f = m_mainPopup->font();
        f.setPointSize(12);
        m_mainPopup->setFont(f);
#endif

        //  rt.init();
        for (int i = 0; i < menu->items().size(); i++)
        {
            const TwkApp::Menu::Item* item = menu->items()[i];

            if (item->subMenu())
            {
                QString title = utf8(item->title());

                QMenu* menu = mb()->addMenu(title);
                //  rt.go();
                connect(menu, SIGNAL(aboutToShow()), this,
                        SLOT(aboutToShowMenu()));
                //  rt.stop();
                convert(menu, item->subMenu(), shortcuts);

                QMenu* pmenu = m_mainPopup->addMenu(title);
#ifdef PLATFORM_DARWIN
                pmenu->setFont(f);
#endif
                //  rt.go();
                connect(pmenu, SIGNAL(aboutToShow()), this,
                        SLOT(aboutToShowMenu()));
                //  rt.stop();
                convert(pmenu, item->subMenu(), shortcuts);
            }
        }
        //  cerr << "    total connect calls: " << rt.total() << " seconds" <<
        //  endl;
    }

    void RvDocument::popupMenu(TwkApp::Menu* menu, QPoint point, bool shortcuts)
    {
        //
        //  This is the only way to overcome the accelerators on the Mac. You
        //  just can't have any shortcuts. Even if the menu is disabled the
        //  shortcuts still function. This was a problem with the pure
        //  Cocoa version as well.
        //

        if (m_menuBarDisable)
            shortcuts = false;

        if (m_userMenu)
            delete m_userMenu;
        m_userMenu = menu;

        if (m_userPopup)
            m_userPopup->clear();
        else
            m_userPopup = new QMenu(this);

        m_userPopup->setObjectName("User Popup");

#ifdef PLATFORM_DARWIN
        QFont f = m_userPopup->font();
        f.setPointSize(12);
        m_userPopup->setFont(f);
#endif

        convert(m_userPopup, menu, shortcuts);
        connect(m_userPopup, SIGNAL(aboutToShow()), this,
                SLOT(aboutToShowMenu()));
        m_userPopup->popup(point);
    }

    void RvDocument::frameChanged()
    {
        //
        //  We'll get here if the frame changed LAST render, so the
        //  currently visible frame is not the same as the previous. The
        //  reason this is in a timer is that we don't want to process
        //  this stuff until after the swapbuffers. Otherwise its a waste
        //  of time.
        //

        lazyDeleteGLView();
        if (m_session)
            m_session->userGenericEvent("frame-changed", "");
    }

    void RvDocument::viewSizeChanged(int w, int h)
    {
        if (m_session)
        {
            m_session->userRenderEvent("view-size-changed", "");
            m_session->deviceSizeChanged(m_glView->videoDevice());
        }
    }

    static bool firstEvent = true;
    static bool restrictActivation = false;

    bool RvDocument::event(QEvent* e)
    {
        if (firstEvent)
        {
            if (!getenv("RV_DO_NOT_RESTRICT_ACTIVATION"))
                restrictActivation = true;
            firstEvent = false;
        }

        if (e->type() == QEvent::WindowActivate)
        {
            DB("event: WindowActivate "
               << m_session->name() << ", currently active "
               << Rv::Session::activeSession()->name());
            //
            //  We should only have to call makeActive() if we are not
            //  currently the active session, but in that case Qt seems
            //  to sometimes take away our keyboard focus  (even when
            //  activeWindow() returns true).
            //
            if (m_session) //&& m_session != Rv::Session::activeSession())
            {
                m_session->makeActive();
            }
            if (!restrictActivation)
                activateWindow();
            DB("event: WindowActivate isActiveWindow " << isActiveWindow());
        }

        bool result = QMainWindow::event(e);
        return result;
    }

    static bool waitingForFirstPaint = true;

    void RvDocument::changeEvent(QEvent* event)
    {
#if 0
    DB ("changeEvent type " << event->type() << " active " << isActiveWindow() << 
            " paint completed " << view()->firstPaintCompleted());
    DB ("changeEvent current gl size w " << m_glView->width() << " h " << m_glView->height());
#endif
#if PLATFORM_LINUX
        //
        //  For some reason KDE wants our main window to come up
        //  "lowered" ie beneath the other windows.  This is the
        //  only way I've found to counter that.
        if (waitingForFirstPaint && view() && view()->firstPaintCompleted())
        {
            activateWindow();
            raise();
            waitingForFirstPaint = false;
        }
#endif
    }

    void RvDocument::closeEvent(QCloseEvent* event)
    {
        //
        //  Unfortunately, I don't remember exactly why I added the
        //  m_closeEventReceived check below.  It interferes with the new
        //  "before-session-deletion" check which allows Mu modes to
        //  setReturnContent("not ok") on the event to interupt the close
        //  process, so I'm taking it out for now.
        //

        //    if (! m_closeEventReceived)
        {
            string ok =
                m_session->userGenericEvent("before-session-deletion", "");

            if (ok == "")
                event->accept();
            else
                event->ignore();

            m_closeEventReceived = true;
        }
    }

    void RvDocument::moveEvent(QMoveEvent* event)
    {
        if (m_session)
            m_session->askForRedraw();
    }

    void RvDocument::addWatchFile(const string& path)
    {
        if (!m_watcher)
        {
            m_watcher = new QFileSystemWatcher();

            connect(m_watcher, SIGNAL(fileChanged(const QString&)), this,
                    SLOT(watchedFileChanged(const QString&)));

            connect(m_watcher, SIGNAL(directoryChanged(const QString&)), this,
                    SLOT(watchedFileChanged(const QString&)));
        }

        m_watcher->addPath(path.c_str());
    }

    void RvDocument::removeWatchFile(const string& path)
    {
        if (m_watcher)
        {
            m_watcher->removePath(path.c_str());
        }
    }

    void RvDocument::watchedFileChanged(const QString& path)
    {
        TwkApp::GenericStringEvent event(
            "file-changed", m_glView->videoDevice(), path.toUtf8().data());

        // cout << m_watcher->files().size() << endl;
        // cout << m_watcher->directories().size() << endl;

        // m_glView->frameBuffer()->sendEvent(event);
        m_glView->videoDevice()->sendEvent(event);
    }

    bool RvDocument::queryDriverVSync() const { return false; }

    void RvDocument::checkDriverVSync()
    {
        Rv::Options& opts = Options::sharedOptions();

        if (queryDriverVSync() && opts.vsync > 0)
        {
            QMessageBox box(this);
            box.setWindowTitle(tr(UI_APPLICATION_NAME ": VSync Conflict"));
            QString baseText =
                tr("Both the graphics driver and RV v-sync are "
                   "ON. " UI_APPLICATION_NAME " vsync is being disabled.");
            QString detailedText(
                "RV's vsync is being disabled for this session because "
                "the graphics driver's vsync is also ON. "
                "You can change this in the nvidia-settings or "
                "in " UI_APPLICATION_NAME "'s preferences. "
                "Running " UI_APPLICATION_NAME
                " with both vsyncs enabled causes incorrect "
                "playback. ");
            box.setText(baseText + "\n\n" + detailedText);
            box.setWindowModality(Qt::WindowModal);
            QPushButton* b1 = box.addButton(tr("Ok"), QMessageBox::AcceptRole);
            box.setIconPixmap(QPixmap(qApp->applicationDirPath()
                                      + QString(RV_ICON_PATH_SUFFIX))
                                  .scaledToHeight(64));
            box.exec();
            opts.vsync = 0;
            m_vsyncDisabled = true;
        }
    }

    void RvDocument::warnOnDriverVSync()
    {
        Rv::Options& opts = Options::sharedOptions();

        if (queryDriverVSync())
        {
            QMessageBox box(this);
            box.setWindowTitle(
                tr(UI_APPLICATION_NAME ": Driver VSync Conflict"));
            QString baseText = tr("The graphics driver v-sync is ON. "
                                  "This can result in bad playback performance "
                                  "in presentation mode.");
            QString detailedText(
                "The graphics driver's vsync is ON. "
                "You can change this in the nvidia-settings OpenGL section. "
                "Running " UI_APPLICATION_NAME
                " in presentation mode with the driver "
                "vsync enabled will cause incorrect "
                "playback. ");
            box.setWindowModality(Qt::WindowModal);
            QPushButton* b1 = box.addButton(tr("Ok"), QMessageBox::AcceptRole);
            box.setIconPixmap(QPixmap(qApp->applicationDirPath()
                                      + QString(RV_ICON_PATH_SUFFIX))
                                  .scaledToHeight(64));
            box.exec();
        }
    }

    void RvDocument::editSourceNode(const string& nodeName)
    {
        if (!m_sourceEditor)
        {
            m_sourceEditor = new RvSourceEditor(session());
        }

        m_sourceEditor->editNode(nodeName);
        m_sourceEditor->show();
    }

} // namespace Rv
