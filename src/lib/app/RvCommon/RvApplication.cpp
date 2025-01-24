//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <RvCommon/QTUtils.h>
#include <RvCommon/RvApplication.h>
#include <RvCommon/RvConsoleWindow.h>
#include <RvCommon/RvDocument.h>
#include <RvCommon/RvNetworkDialog.h>
#include <RvCommon/RvPreferences.h>
#include <RvCommon/RvProfileManager.h>
#include <RvCommon/DesktopVideoDevice.h>
#include <RvCommon/RvWebManager.h>
#include <RvCommon/GLView.h> // WINDOWS: include AFTER other stuff
#include <RvCommon/QTGLVideoDevice.h>
#include <RvCommon/DesktopVideoModule.h>
#include <QtCore/QtCore>
#include <QtGui/QtGui>
#include <QtNetwork/QtNetwork>
#include <QtCore/QTimer>
#include <IPCore/Application.h>
#include <RvApp/Options.h>
#include <TwkQtBase/QtUtil.h>
#include <TwkQtCoreUtil/QtConvert.h>
#include <IPBaseNodes/StackGroupIPNode.h>
#include <IPBaseNodes/StackIPNode.h>
#include <MuTwkApp/MuInterface.h>
#include <TwkDeploy/Deploy.h>
#include <TwkUtil/sgcHop.h>
#include <TwkUtil/User.h>
#include <TwkUtil/Clock.h>
#include <QTAudioRenderer/QTAudioRenderer.h>
#include <iostream>
#include <iterator>
#include <thread>
#include <stl_ext/stl_ext_algo.h>
#include <arg.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#ifdef PLATFORM_WINDOWS
#include <process.h>
#endif

#include <QtCore/qlogging.h>
#include <QtCore/QStandardPaths>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QTextEdit>

#ifndef PLATFORM_WINDOWS
extern char** environ;
#endif

#ifdef PLATFORM_DARWIN
extern void (*sessionFromUrlPointer)(std::string);
extern void (*putUrlOnMacPasteboardPointer)(std::string, std::string);
extern void (*registerEventHandlerPointer)();
#endif

static void init()
{
    //
    //  This has to be outside of RvApplication::RvApplication() for
    //  some reason on Linux.
    //

    Q_INIT_RESOURCE(RvCommon);
}

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

// RV lazy build third party optional customization
#if defined(RV_LAZY_BUILD_THIRD_PARTY_CUSTOMIZATION)
extern void rvLazyBuildThirdPartyCustomization();
#endif

namespace Rv
{
    using namespace std;
    using namespace TwkApp;
    using namespace IPCore;
    using namespace boost;
    using namespace TwkQtCoreUtil;

    class RecursionLock
    {
    public:
        RecursionLock() { _recursionLocked = true; };

        ~RecursionLock() { _recursionLocked = false; };

        static bool _recursionLocked;

        static bool locked() { return _recursionLocked; };
    };

    bool RecursionLock::_recursionLocked = false;

    void myMsgHandlerQT5(QtMsgType t, const QMessageLogContext& context,
                         const QString& qmsg)
    {
        //
        //  Since printing errors may generate errors, don't allow
        //  recursion.  At worst we might lose some error messages.
        //
        if (RecursionLock::locked())
            return;
        RecursionLock l;

        string msg = qmsg.toUtf8().constData();

        // Do not report Java script errors unless requested
        static bool reportQWebEngineJavaScripErrors =
            getenv("RV_REPORT_QWEBENGINE_JAVA_SCRIPT_ERRORS") != nullptr;
        if (context.category && (strncmp(context.category, "js", 2) == 0)
            && !reportQWebEngineJavaScripErrors)
        {
            return;
        }

        switch (t)
        {
        case QtDebugMsg:
        {
            // We always report Qt debug messages except for known issues.
            // In which case the following environment variable can be used to
            // check whether those known messages are still reported or not by
            // Qt.
            static const bool reportQtDebugMessages =
                getenv("RV_REPORT_QT_DEBUG_MESSAGES") != nullptr;

            // "DEBUG: Release of profile requested but WebEnginePage still not
            // deleted. Expect troubles !"
            const bool knownMessage =
                (std::string(msg).find("Release of profile requested but "
                                       "WebEnginePage still not deleted")
                 != std::string::npos);

            if (!knownMessage || reportQtDebugMessages)
            {
                cout << "DEBUG: " << msg << endl;
            }
        }
        break;
        case QtWarningMsg:
            //
            //  We get these spurious errors from Qt with some tablets.  They
            //  seem to indicate no real problem and slow down interaction.
            //
            if (std::string(msg).find("This tablet device is unknown")
                == std::string::npos)
            {
                cout << "WARNING: " << msg << endl;
            }
            break;
        case QtCriticalMsg:
        case QtFatalMsg:
        {
            // Qt warning about KVO observers. Qt have gotten rid of the check
            // for this error in Qt 6.0. "ERROR: has active key-value observers
            //(KVO)!
            //        These will stop working now that the window is recreated,
            //        and will result in exceptions when the observers are
            //        removed. Break in QCocoaWindow::recreateWindowIfNeeded to
            //        debug."
            const bool ignoreError =
                std::string(msg).find(
                    "has active key-value observers (KVO)! These will stop "
                    "working now that the window is recreated")
                != std::string::npos;

            if (!ignoreError)
            {
                cerr << "ERROR: " << msg << endl;
            }
        }
        break;
        default:
            cout << "INFO: " << msg << endl;
            break;
        }

        assert(t != QtFatalMsg);
    }

    class RvProxyFactory : public QNetworkProxyFactory
    {
    public:
        RvProxyFactory();
        virtual ~RvProxyFactory();

        virtual QList<QNetworkProxy>
        queryProxy(const QNetworkProxyQuery& query);

        bool hasProxy() { return m_hasProxy; }

    private:
        QNetworkProxy m_proxy;
        bool m_hasProxy;
    };

    RvProxyFactory::RvProxyFactory()
        : QNetworkProxyFactory()
        , m_hasProxy(false)
    {
        if (getenv("RV_NETWORK_PROXY_HOST"))
        {
            m_proxy.setType(QNetworkProxy::HttpProxy);
            m_proxy.setHostName(getenv("RV_NETWORK_PROXY_HOST"));

            if (getenv("RV_NETWORK_PROXY_PORT"))
                m_proxy.setPort(atoi(getenv("RV_NETWORK_PROXY_PORT")));
            if (getenv("RV_NETWORK_PROXY_USER"))
                m_proxy.setUser(getenv("RV_NETWORK_PROXY_USER"));
            if (getenv("RV_NETWORK_PROXY_PASSWORD"))
                m_proxy.setPassword(getenv("RV_NETWORK_PROXY_PASSWORD"));

            m_hasProxy = true;
        }
        else if (getenv("RV_NETWORK_PROXY_DISABLE"))
        {
            m_proxy.setType(QNetworkProxy::NoProxy);
        }
        else
        {
            m_proxy.setType(QNetworkProxy::DefaultProxy);
        }
    }

    QList<QNetworkProxy>
    RvProxyFactory::queryProxy(const QNetworkProxyQuery& query)
    {
        QList<QNetworkProxy> l;
        l.append(m_proxy);
        return l;
    }

    RvProxyFactory::~RvProxyFactory() {}

    static void setEnvVar(const string& var, const string& val)
    {
#ifdef WIN32
        ostringstream str;
        str << var << "=" << val;
        putenv(str.str().c_str());
#else
        setenv(var.c_str(), val.c_str(), 1);
#endif
    }

    RvApplication::RvApplication(int argc, char** argv)
        : QObject()
        , RvConsoleApplication()
        , m_newTimer(0)
        , m_lazyBuildTimer(0)
        , m_console(0)
        , m_prefDialog(0)
        , m_profileDialog(0)
        , m_aboutAct(0)
        , m_prefAct(0)
        , m_networkAct(0)
        , m_quitAct(0)
        , m_macMenuBar(0)
        , m_macRVMenu(0)
        , m_networkDialog(0)
        , m_webManager(0)
        , m_presentationMode(false)
        , m_presentationDevice(0)
        , m_executableNameCaps(UI_APPLICATION_NAME)
        , m_desktopModule(0)
        , m_dispatchAtomicInt(0)
    {

#ifdef PLATFORM_DARWIN
        sessionFromUrlPointer = sessionFromUrl;
#endif

        //  Proxy support
        //
        RvProxyFactory* factory = new RvProxyFactory();
        if (factory->hasProxy())
        {
            QNetworkProxyFactory::setApplicationProxyFactory(factory);
        }

        IPCore::AudioRenderer::addModuleInitFunc(
            IPCore::QTAudioRenderer::addQTAudioModule<RvApplication>,
            "Platform Audio");

        qInstallMessageHandler(myMsgHandlerQT5);
        init();

        pthread_mutex_init(&m_deleteLock, 0);
        m_timer = new QTimer(this);
        m_timer->setObjectName("RvApplication::m_timer");
        connect(m_timer, SIGNAL(timeout()), this, SLOT(heartbeat()));
        m_timer->start(int(1.0 / 120.0 * 1000.0));
        m_fireTimer.start();

        m_lazyBuildTimer = new QTimer();
        connect(m_lazyBuildTimer, SIGNAL(timeout()), this, SLOT(lazyBuild()));
        m_lazyBuildTimer->setSingleShot(true);
        m_lazyBuildTimer->start();

        m_aboutAct = new QAction(tr("About " UI_APPLICATION_NAME "..."), this);
        m_aboutAct->setStatusTip(
            tr("Information about this version of " UI_APPLICATION_NAME));
        m_aboutAct->setMenuRole(QAction::AboutRole);
        connect(m_aboutAct, SIGNAL(triggered()), this, SLOT(about()));

        m_prefAct = new QAction(tr("Preferences..."), this);
        m_prefAct->setStatusTip(tr("Configure RV for this computer"));
        m_prefAct->setMenuRole(QAction::PreferencesRole);
        connect(m_prefAct, SIGNAL(triggered()), this, SLOT(prefs()));

        QAction* sep1 = new QAction(tr("sep1"), this);
        sep1->setMenuRole(QAction::ApplicationSpecificRole);
        sep1->setSeparator(true);

        QAction* sep2 = new QAction(tr("sep2"), this);
        sep2->setMenuRole(QAction::ApplicationSpecificRole);
        sep2->setSeparator(true);

        m_networkAct = new QAction(tr("Network..."), this);
        m_networkAct->setStatusTip(
            tr("Allow remote communication with this " UI_APPLICATION_NAME));
        m_networkAct->setMenuRole(QAction::ApplicationSpecificRole);
        connect(m_networkAct, SIGNAL(triggered()), this,
                SLOT(showNetworkDialog()));

        m_quitAct = new QAction(tr("Quit " UI_APPLICATION_NAME), this);
#ifndef PLATFORM_DARWIN
        m_quitAct->setShortcut(QKeySequence("Ctrl+Q"));
#endif
        m_quitAct->setStatusTip(
            tr("Exit all " UI_APPLICATION_NAME " Sessions"));
        m_quitAct->setMenuRole(QAction::QuitRole);
        connect(m_quitAct, SIGNAL(triggered()), this, SLOT(quitAll()));

#if defined(PLATFORM_DARWIN)
        m_macMenuBar = new QMenuBar(0);
        // NOTE: the UI_APPLICATION_NAME string below isn't what's visible as
        // main menu. The main menu visible string comes from the
        // 'src/bin/nsapp/RV/Info.plist' file.
        m_macRVMenu = m_macMenuBar->addMenu(UI_APPLICATION_NAME);
        m_macRVMenu->addAction(m_aboutAct);
        if (m_networkAct)
            m_macRVMenu->addAction(m_networkAct);
        m_macRVMenu->addAction(m_prefAct);
        m_macRVMenu->addAction(m_quitAct);

        //
        //  As of Qt4.6 they conflate open url events with file open
        //  events, but the url they provide in that event is mangled
        //  so we re-register our url handler here to override their's.
        //
        //  Of course, as of Qt4.7, they moved their registration deeper
        //  actually into the event loop, so there's no way to get 'behind'
        //  it with out own registration.
        //
        //  if (registerEventHandlerPointer) (*registerEventHandlerPointer)();
#endif

        //
        //  Font Awesome icon font
        //
        //  See: http://fortawesome.github.io/Font-Awesome/icons/
        //
        //  NOTE: this is expensive and may be better done by FontLoader
        //  which supposedly can do this in a separate thread. The
        //  downside is that there can be a race condition between use of
        //  the font and its availability in that case
        //
        //  The load time on debug build on tast machine was 0.012s
        //

        QFile res(":/fonts/fontawesome-webfont.ttf");
        if (!res.open(QIODevice::ReadOnly))
        {
            cerr << "ERROR: Font awesome font could not be loaded!";
        }
        else
        {
            QByteArray fontData(res.readAll());
            res.close();
            int loaded = QFontDatabase::addApplicationFontFromData(fontData);
            if (loaded == -1)
            {
                cerr << "ERROR: Font awesome font could not be loaded!";
            }
        }
        m_dispatchTimer = new QTimer(this);
        m_dispatchTimer->setSingleShot(true);
        connect(m_dispatchTimer, SIGNAL(timeout()), this,
                SLOT(dispatchTimeout()));
    }

    RvApplication::~RvApplication()
    {
        delete m_timer;
        delete m_newTimer;
        delete m_console;
        delete m_networkDialog;
        delete m_lazyBuildTimer;
        m_console = 0;
        m_timer = 0;
        m_lazyBuildTimer = 0;
        pthread_mutex_destroy(&m_deleteLock);
    }

    int RvApplication::parseInFiles(int argc, char* argv[])
    {
        //  cerr << "Application::parseInFiles " << argc << " files" << endl;
        Rv::Options& opts = Rv::Options::sharedOptions();

        for (int i = 0; i < argc; i++)
        {
            string newS = mapFromVar(argv[i]);

            //  cerr << "    " << newS << endl;
            opts.inputFiles.push_back(newS);
        }
        return 0;
    }

    void RvApplication::lazyBuild()
    {
        if (!m_console)
            m_console = new RvConsoleWindow();

// RV lazy build third party optional customization
#if defined(RV_LAZY_BUILD_THIRD_PARTY_CUSTOMIZATION)
        rvLazyBuildThirdPartyCustomization();
#endif
    }

    RvConsoleWindow* RvApplication::console()
    {
        if (!m_console)
            m_console = new RvConsoleWindow();
        return m_console;
    }

    void RvApplication::showNetworkDialog()
    {
        networkWindow()->show();
        networkWindow()->raise();
    }

    RvWebManager* RvApplication::webManager()
    {
        if (!m_webManager)
        {
            m_webManager = new RvWebManager(this);
        }

        return m_webManager;
    }

    RvNetworkDialog* RvApplication::networkWindow()
    {
        if (!m_networkDialog)
            m_networkDialog = new RvNetworkDialog(0);
        TwkApp::Document* doc = TwkApp::Document::activeDocument();

        if (doc)
        {
            Rv::Session* session = static_cast<Rv::Session*>(doc);
            RvDocument* rvDoc = (RvDocument*)session->opaquePointer();

            QSize ws = m_networkDialog->size();
            QSize s = rvDoc->size();
            QPoint p = rvDoc->pos();

            m_networkDialog->move(p.x() + s.width() / 2 - ws.width() / 2,
                                  p.y() + s.height() / 2 - ws.height() / 2);
        }

        return m_networkDialog;
    }

    static void callRunCreateSession(void* a)
    {
        RvApplication* app = reinterpret_cast<RvApplication*>(a);
        app->runCreateSession();
    }

    void RvApplication::about()
    {
        bool isReleaseBuild = strcmp(RELEASE_DESCRIPTION, "RELEASE") == 0;

        ostringstream headerComment;

        headerComment << "<h3>";
        if (!isReleaseBuild)
            headerComment << "<i><font color=red>";
        headerComment << RELEASE_DESCRIPTION << " Version";
        if (!isReleaseBuild)
            headerComment << "</font></i>";
        headerComment << "</h3>";

        ostringstream date;
        TWK_DEPLOY_SHOW_PROGRAM_BANNER(date);

        vector<char> temp;
        temp.reserve(2048);
        sprintf(
            temp.data(), "<h1>%s</h1><h2>%d.%d.%d (%s)</h2> %s <p>%s %s </p>",
            UI_APPLICATION_NAME, TWK_DEPLOY_MAJOR_VERSION(),
            TWK_DEPLOY_MINOR_VERSION(), TWK_DEPLOY_PATCH_LEVEL(), GIT_HEAD,
            headerComment.str().c_str(), UI_APPLICATION_NAME, COPYRIGHT_TEXT);

        const TwkApp::Document* doc = TwkApp::Document::activeDocument();
        QWidget* parent = 0;
        if (doc)
            parent = (RvDocument*)doc->opaquePointer();

        QMessageBox* msgBox =
            new QMessageBox("About " UI_APPLICATION_NAME, QString(temp.data()),
                            QMessageBox::Information, 0, 0, 0, parent,
                            Qt::WindowTitleHint | Qt::WindowSystemMenuHint);
        msgBox->setAttribute(Qt::WA_DeleteOnClose);
        QIcon icon = msgBox->windowIcon();
        QSize size = icon.actualSize(QSize(64, 64));
        msgBox->setIconPixmap(icon.pixmap(size));
        msgBox->setStandardButtons(QMessageBox::Close);

        QGridLayout* grid = static_cast<QGridLayout*>(msgBox->layout());

        QDialogButtonBox* buttonBox = msgBox->findChild<QDialogButtonBox*>();
        grid->removeWidget(buttonBox);
        buttonBox->setCenterButtons(true);

        grid->addItem(
            new QSpacerItem(1, 12, QSizePolicy::Fixed, QSizePolicy::Fixed),
            grid->rowCount(), 0);
        grid->addWidget(buttonBox, grid->rowCount(), 0, 1, -1);

        msgBox->show();
        msgBox->exec();
    }

    void RvApplication::quitAll()
    {
        TwkApp::Application::Documents docs = documents();

        for (size_t i = 0; i < docs.size(); i++)
        {
            Rv::Session* s = static_cast<Rv::Session*>(docs[i]);
            RvDocument* rvDoc = (RvDocument*)s->opaquePointer();
            rvDoc->close();
        }
    }

    void RvApplication::runCreateSession()
    {
        if (m_newSessions.size())
        {
            // Make sure that the RV application has been initialized
            // Otherwise wait for the next occurence of the newSession timer to
            // prevent a crash.
            if (!TwkApp::muProcess())
            {
                return;
            }

            Rv::Options& opts = Rv::Options::sharedOptions();

            StringVector all;

            for (int i = 0; i < m_newSessions.size(); i++)
            {
                StringVector* sv = m_newSessions[i];
                copy(sv->begin(), sv->end(), back_inserter(all));
                delete sv;
            }

            m_newSessions.clear();
            if (m_newTimer)
                m_newTimer->stop();

            newSessionFromFiles(all);
        }
    }

    RvDocument*
    RvApplication::rebuildSessionFromFiles(Rv::RvSession* s,
                                           const StringVector& files)
    {
        //
        //  If we have new files, clear the session.  If we don't have
        //  new files, don't clear it, since we have have built a
        //  session "by hand" ie via mu.
        //
        //  Code, like the shotgrid module, that want's to clear the
        //  session with each processed URL, should clear the session
        //  explicitly.
        //
        if (files.size() && s->sources().size())
            s->clear();

        Rv::Options& opts = Rv::Options::sharedOptions();

        Rv::RvDocument* doc = (RvDocument*)s->opaquePointer();

        if (opts.nomb && doc->menuBarShown())
            doc->toggleMenuBar();

        try
        {
            s->readUnorganizedFileList(files, true);
        }
        catch (const std::exception& exc)
        {
            cerr << "ERROR: " << exc.what() << endl;
        }

        return doc;
    }

    RvDocument* RvApplication::newSessionFromFiles(const StringVector& files)
    {
        DB("RvApplication::newSessionFromFiles()");
        Rv::RvDocument* doc = new Rv::RvDocument;
        // doc->ensurePolished();
        Rv::RvSession* s = doc->session();

        s->queryAndStoreGLInfo();

        Rv::Options& opts = Rv::Options::sharedOptions();

        DB("xl " << opts.xl);
        doc->setAggressiveSizing(opts.xl);
        if (opts.nomb && doc->menuBarShown())
            doc->toggleMenuBar();

        rebuildSessionFromFiles(s, files);

        //
        //  Since we stopped giving Qt a style sheet, it doesn't seem to
        //  account for the menubar and it makes the window too small to
        //  hold both the menubar and the image.  This "fixes" that.
        //  In general, the menubar does not seem to report the correct
        //  size until this point.
        //

        if (s->userHasSetViewSize())
        {
            doc->center();
        }

        //
        //  Allow command line placement
        //

        if (opts.x != -1 || opts.y != -1)
        {
            if (opts.screen != -1
                && QApplication::desktop()->isVirtualDesktop())
            {
                QRect r = QApplication::desktop()->screenGeometry(opts.screen);
                opts.x += r.x();
                opts.y += r.y();
            }

            if (opts.width != -1 && opts.height != -1)
            {
                doc->setGeometry(opts.x, opts.y, opts.width, opts.height);
            }
            else
            {
                doc->move(opts.x, opts.y);
            }
        }

        int screen = QApplication::desktop()->screenNumber(QCursor::pos());
        if (opts.screen != -1)
            screen = opts.screen;

        int oldX = doc->pos().x();
        int oldY = doc->pos().y();

        int oldScreen =
            QApplication::desktop()->screenNumber(QPoint(oldX, oldY));

        if (screen != -1 && QApplication::desktop()->isVirtualDesktop()
            && screen != oldScreen)
        //
        //  The application is going to come up on the wrong screen, so figure
        //  out our our relative position on the current screen, and move to the
        //  same relative position on the correct screen.
        //
        {
            QRect rnew = QApplication::desktop()->screenGeometry(screen);
            QRect rold = QApplication::desktop()->screenGeometry(oldScreen);

            int xoff = oldX - rold.x();
            int yoff = oldY - rold.y();

            doc->move(rnew.x() + xoff, rnew.y() + yoff);
        }

        if (opts.fullscreen && !doc->isFullScreen())
        {
            doc->toggleFullscreen(true);

            if (opts.width != -1 && opts.height != -1)
            {
                doc->setGeometry(opts.x, opts.y, opts.width, opts.height);
            }
        }

        doc->show();

#ifndef PLATFORM_LINUX
        doc->raise();
#endif

        if (videoModules().empty())
        {
            doc->view()->makeCurrent();

            try
            {
                addVideoModule(m_desktopModule = new DesktopVideoModule(
                                   0, doc->view()->videoDevice()));
            }
            catch (...)
            {
                cerr << "ERROR: DesktopVideoModule failed" << endl;
            }

            if (!getenv("RV_SKIP_LOADING_VIDEO_OUTPUT_PLUGINS"))
            {

                // Load audio/video output plugins
                loadOutputPlugins("TWK_OUTPUT_PLUGIN_PATH");
            }
            else
            {
                cout << "WARNING: Skipping loading of video output plugins"
                     << endl;
            }
        }

        doc->session()->graph().setPhysicalDevices(videoModules());

        //
        //  The above re-created the set of display groups, we know what screen
        //  we're on (video device) so make sure the primary display group is
        //  correct.
        //
        doc->session()->graph().setPrimaryDisplayGroup(
            doc->view()->videoDevice());

        if (RvApp()->documents().size() == 1 && opts.present)
        {
            setPresentationMode(true);
        }

        //
        //  All modes are toggled, command line processed, etc, so tell users:
        //
        s->userGenericEvent("session-initialized", "");

        if (s->loadTotal() == 0)
        //
        //  We will not be loading media at all, so send
        //  after-progressive-loading event.
        //
        {
            s->userGenericEvent("after-progressive-loading", "");
        }

        return doc;
    }

    VideoModule* RvApplication::primaryVideoModule() const
    {
        return m_desktopModule;
    }

    void RvApplication::openVideoModule(VideoModule* m) const
    {
        if (!m->isOpen())
        {
            RvDocument* doc = reinterpret_cast<RvDocument*>(
                documents().front()->opaquePointer());
            doc->view()->makeCurrent();
            m->open();
            //
            //  The open() may have added video devices, so make sure each
            //  device has a corresponding display group.
            //
            IPGraph& graph = doc->session()->graph();
            const VideoModule::VideoDevices& devs = m->devices();

            for (size_t i = 0; i < devs.size(); i++)
            {
                TwkApp::VideoDevice* device = devs[i];

                if (0 == graph.findDisplayGroupByDevice(device))
                {
                    ostringstream str;
                    str << "displayGroup" << graph.displayGroups().size();
                    string name = str.str(); // windows

                    graph.addDisplayGroup(graph.newDisplayGroup(name, device));
                }
            }
        }
    }

    void RvApplication::createNewSessionFromFiles(const StringVector& files)
    {
        StringVector* sv = new StringVector(files.size());
        if (!files.empty())
            copy(files.begin(), files.end(), sv->begin());

        if (m_newSessions.empty())
        {
            if (!m_newTimer)
            {
                m_newTimer = new QTimer(this);
                m_newTimer->setObjectName("m_newTimer");
                connect(m_newTimer, SIGNAL(timeout()), this,
                        SLOT(runCreateSession()));
            }

            m_newTimer->start(int(1.0 / 192.0 * 1000.0));
        }

        m_newSessions.push_back(sv);
    }

    RvApplication::DispatchID
    RvApplication::dispatchToMainThread(DispatchCallback callback)
    {
        std::lock_guard<DispatchMutex> guard(m_dispatchMutex);

        auto dispatchID = m_nextDispatchID++;
        m_dispatchMap[dispatchID] = callback;

        QMetaObject::invokeMethod(m_dispatchTimer, "start",
                                  Qt::QueuedConnection, Q_ARG(int, 0));

        return dispatchID;
    }

    void RvApplication::undispatchToMainThread(DispatchID dispatchID,
                                               double maxDuration)
    {
        {
            std::lock_guard<DispatchMutex> guard(m_dispatchMutex);

            auto iter = m_dispatchMap.find(dispatchID);
            if (iter != m_dispatchMap.end())
                m_dispatchMap.erase(iter);
        }

        TwkUtil::SystemClock clock;
        auto t0 = clock.now();

        while ((clock.now() - t0) < maxDuration)
        {
            if (!isDispatchExecuting(dispatchID))
                return;

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        std::cout << "ERROR: Max undispatch time reached." << std::endl;
        return;
    }

    bool RvApplication::isDispatchExecuting(DispatchID dispatchID)
    {
        std::lock_guard<DispatchMutex> guard(m_dispatchMutex);

        return m_executingMap.find(dispatchID) != m_executingMap.end();
    }

    void RvApplication::dispatchTimeout()
    {
        // Fix for error when loading multiple sources with the newly created
        // API call AddsourcesVerbose() dispatchTimeout() was being called by
        // another thread resulting in a crash.
        int atomicInt = ++m_dispatchAtomicInt;
        if (atomicInt == 1)
        {
            HOP_PROF_FUNC();

            {
                std::lock_guard<DispatchMutex> guard(m_dispatchMutex);
                m_dispatchMap.swap(m_executingMap);
            }

            for (auto iter : m_executingMap)
            {
                HOP_PROF("RvApplication::dispatchTimeout() : Execute 1");
                iter.second(iter.first);
            }

            {
                std::lock_guard<DispatchMutex> guard(m_dispatchMutex);
                m_executingMap.clear();
            }
        }
        m_dispatchAtomicInt--;
    }

    void RvApplication::heartbeat()
    {
        double secs = m_fireTimer.elapsed();

        if (false)
            if (secs > 1.0 / 30.0)
            {
                cout << "WARNING: last timer fired " << secs << " seconds ago ("
                     << (1.0 / secs) << " fps)" << endl;
            }

        m_fireTimer.stop();
        m_fireTimer.start();

        timerCB();
    }

    void RvApplication::stopTimer()
    {
        // cout << "stop timer" << endl;
        m_timer->stop();
    }

    void RvApplication::startTimer()
    {
        // cout << "start timer" << endl;
        m_timer->start(int(1.0 / 192.0 * 1000.0));
    }

    RvProfileManager* RvApplication::profileManager()
    {
        if (!m_profileDialog)
        {
            m_profileDialog = new RvProfileManager(this);
        }

        return m_profileDialog;
    }

    RvPreferences* RvApplication::prefDialog()
    {
        if (!m_prefDialog)
        {
            Rv::Session* session = Rv::Session::currentSession();
            assert(session);
            RvDocument* rvDoc = (RvDocument*)session->opaquePointer();
            m_prefDialog = new RvPreferences(rvDoc);
            QRect docGeom = rvDoc->geometry();
            int docX = docGeom.left() + int(docGeom.width() / 2);
            int docY = docGeom.top() + int(docGeom.height() / 2);

            int x = docX - int(m_prefDialog->width() / 2);
            int y = docY - int(m_prefDialog->height() / 2);
            m_prefDialog->move(x, y);
        }

        return m_prefDialog;
    }

    void RvApplication::prefs()
    {
        if (isInPresentationMode())
        {
            const TwkApp::Document* doc = TwkApp::Document::activeDocument();
            QWidget* parent = 0;
            if (doc)
                parent = (RvDocument*)doc->opaquePointer();
            QMessageBox box(parent);
            box.setWindowTitle(tr(UI_APPLICATION_NAME ": Preferences"));
            box.setText(tr("Presentation Mode is Enabled -- Preferences cannot "
                           "be shown.\nDisable Presentation Mode?"));
            // the detailed box mess up the buttons on OS X with CSS
            // just getting rid of it fixes it
            // box.setDetailedText(tr("Preferences cannot be shown until "
            //"presentation mode is disabled."));
            box.setWindowModality(Qt::WindowModal);
            QPushButton* b1 =
                box.addButton(tr("Cancel"), QMessageBox::RejectRole);
            QPushButton* b2 = box.addButton(tr("Disable Presentation Mode"),
                                            QMessageBox::AcceptRole);
#ifdef PLATFORM_LINUX
            box.setIconPixmap(QPixmap(qApp->applicationDirPath()
                                      + QString(RV_ICON_PATH_SUFFIX))
                                  .scaledToHeight(64));
#else
            box.setIcon(QMessageBox::Critical);
#endif
            box.exec();

            if (box.clickedButton() == b1)
                return;
            setPresentationMode(false);
        }

        RvPreferences* prefs = prefDialog();
        prefs->update();

        const TwkApp::Application::Documents& docs = documents();

        for (size_t i = 0; i < docs.size(); i++)
        {
            Rv::Session* s = static_cast<Rv::Session*>(docs[i]);
            s->userGenericEvent("preferences-show", "");
        }

        prefs->raise();
        prefs->show();
    }

    bool RvApplication::eventFilter(QObject* o, QEvent* event)
    {
        if (event->type() == QEvent::FileOpen)
        {
            QFileOpenEvent* fileEvent = static_cast<QFileOpenEvent*>(event);
            StringVector files(1);
            files.front() = fileEvent->file().toUtf8().data();
            if (!files.front().empty())
                createNewSessionFromFiles(files);
            else
            {
                //
                //  As of Qt 4.6, the FileOpen event can contain a URL
                //  instead of a filename.
                //
                string url = fileEvent->url().toString().toUtf8().data();
                cout << "WARNING: URL from file open event '" << url << "'"
                     << endl;
                if (!url.empty())
                    sessionFromUrl(url);
            }
        }

        return false;
    }

    void RvApplication::sessionFromUrl(string url)
    {
        //  cerr << "RvApplication::sessionFromUrl '" << url << "'" << endl;

        vector<char*> args;
        args.push_back((char*)"rvlink");
        parseURL(url.c_str(), args);
        char** newArgv = &(args[0]);
        int newArgc = args.size();

        //  for (int i = 0; i < newArgc; ++i) cerr << "    " << newArgv[i] <<
        //  endl;

        Rv::Options& opts = Rv::Options::sharedOptions();
        Rv::Options freshOpts, oldOpts = opts;
        opts = freshOpts;

        Rv::RvPreferences::loadSettingsIntoOptions(opts);

        //
        //  We want some settings to carry over ?
        //

        freshOpts.urlsReuseSession = oldOpts.urlsReuseSession;
        opts.delaySessionLoading = oldOpts.delaySessionLoading;
        opts.progressiveSourceLoading = oldOpts.progressiveSourceLoading;

        Rv::Options::manglePerSourceArgs(newArgv, newArgc);

        opts.inputFiles.clear();

        if (arg_parse(
                newArgc, newArgv, "", "", RV_ARG_EXAMPLES, "", "",
                RV_ARG_SEQUENCE_HELP, "", "", RV_ARG_SOURCE_OPTIONS(opts), "",
                "", "", ARG_SUBR(parseInFiles),
                "Input sequence patterns, images, movies, or directories ",
                RV_ARG_PARSE_OPTIONS(opts), NULL)
            < 0)
        {
            cerr << "ERROR: could not parse options from URL: '" << url << "'"
                 << endl;
            return;
        }

        RvApplication* rvapp = RvApp();

        if (!opts.initializeAfterParsing(&opts))
        {
            cerr << "ERROR: initializeAfterParsing failed" << endl;
            rvapp->console()->processTextBuffer();
            return;
        }

        TwkApp::Document* doc = TwkApp::Document::activeDocument();
        //  cerr << "sessionFromUrl reuse " << opts.urlsReuseSession << endl;
        if (!opts.urlsReuseSession || doc == 0)
        {
            rvapp->createNewSessionFromFiles(opts.inputFiles);
        }
        else
        {
            Rv::RvSession* session = static_cast<Rv::RvSession*>(doc);
            RvDocument* rvDoc = (RvDocument*)session->opaquePointer();

            rvapp->rebuildSessionFromFiles(session, opts.inputFiles);

            // if (session->frameBuffer()) session->frameBuffer()->redraw();
            if (opts.fullscreen && !rvDoc->isFullScreen())
                rvDoc->toggleFullscreen(true);
            rvDoc->show();

#ifndef PLATFORM_LINUX
            rvDoc->raise();
#endif
        }

        rvapp->processNetworkOpts(false);
    }

    bool decodeUrlString(string s, string& newUrl)
    {
        bool didDecodeSomething = false;

        newUrl = "";

        int len = s.size();
        for (int i = 0; i < len; ++i)
        {
            if ('%' == s[i] && i < len - 2 && isxdigit(s[i + 1])
                && isxdigit(s[i + 2]))
            //
            //  This is an escaped char.
            //
            {
                int cInt;
                sscanf(s.c_str() + i + 1, "%02x", &cInt);
                char c = cInt;
                newUrl.push_back(c);
                i += 2;
                didDecodeSomething = true;
            }
            //
            //  XXX Hack!  For some reason known only to bill gates,
            //  urls handled by windows (even ones that come from
            //  firefox) have a '/' tacked on the end.  So we discard it
            //  here.
            //
            else if (i != len - 1 || s[i] != '/')
                newUrl.push_back(s[i]);
        }
        //  cerr << "decoded URL: '" << s << "' -> '" << newUrl << "'" << endl;

        return didDecodeSomething;
    }

    //
    //  Parse a string with embedded whitespace into an argument list.
    //  Any embedded whitespace or other special character is assumed to
    //  be represented as "%xx" where "xx" are the appropriate 2 hex
    //  digits.
    //
    //  Supports on level of quoting, quote character is '
    //
    //  URL may be "baked" that is each byte of the contents transformed
    //  into a hex char pair.  In that case "unbake" it first.
    //

    static string rawURL(string bakedUrl)
    {
        static const char* bakedPrefix = "rvlink://baked/";
        static const char* rawPrefix = "rvlink://";

        if (!strncmp(bakedPrefix, bakedUrl.c_str(), strlen(bakedPrefix)))
        //
        //  URL is baked, so un-bake
        //
        {
            char* buf = new char[bakedUrl.size()];
            strcpy(buf, rawPrefix);

            char* bakedP = ((char*)(bakedUrl.c_str())) + strlen(bakedPrefix);
            char* rawP = buf + strlen(rawPrefix);
            char* lim = ((char*)(bakedUrl.c_str())) + bakedUrl.size() - -1;

            while (bakedP < lim)
            {
                if (isxdigit(*bakedP) && isxdigit(*(bakedP + 1)))
                {
                    unsigned int c;
                    sscanf(bakedP, "%02x", &c);
                    *(rawP++) = (char)c;
                    bakedP += 2;
                }
                else
                    ++bakedP;
            }
            *rawP = '\0';

            string bufs(buf);
            delete[] buf;
            return bufs;
        }
        else
        {
            string newUrl;
            decodeUrlString(bakedUrl, newUrl);
            return newUrl;
        }
    }

    void RvApplication::parseURL(const char* s, vector<char*>& av)
    {
        cerr << "INFO: received URL '" << s << "'" << endl;
        string newUrl(rawURL(s));
        vector<char*> avNew;

        cerr << "INFO: decoded URL '" << newUrl << "'" << endl;

        bool inArg = false, inQuote = false;
        int len = newUrl.size();
        string a;

        //
        //  Skip over the protocol 'name', ie everything up to and
        //  including the first ':'.
        //
        int i = 0;
        for (; i < len && ':' != newUrl[i]; ++i)
            ;
        ++i;
        //  Skip over slashes
        i += 2;

        for (; i < len; ++i)
        {
            if (inArg && !inQuote && isspace(newUrl[i]))
            //
            //  An unquoted whitespace char terminates the current arg.
            //
            {
                //  cerr << "adding '" << a << "'" << endl;
                avNew.push_back(strdup(a.c_str()));

                a.clear();
                inArg = false;
            }
            else if ('\'' == newUrl[i])
            {
                inQuote = (inQuote) ? false : true;
                inArg = true;
                //  cerr << "quote found, inQuote " << inQuote << endl;
            }
            else if (!isspace(newUrl[i]) || inQuote)
            {
                a.push_back(newUrl[i]);
                inArg = true;
            }
        }
        if (inArg && a.size())
        {
            //  cerr << "adding '" << a << "' " << endl;
            avNew.push_back(strdup(a.c_str()));
        }

        for (int i = 0; i < avNew.size(); ++i)
        {
            string s(avNew[i]);
            if (s == "-eval" || s == "-pyeval" && i < avNew.size() - 1)
            {
                //
                //  Check for allowed evals (used by shotgrid).
                //  XXX in the long run we need to change shotgrid to not use
                //  these.
                //
                string s2(avNew[i + 1]);
                string ok1("launchTimeline");
                string ok2("compareFromVersionIDs");
                string ok3("sessionFromVersionIDs");
                string ok4("shotgrid_review_app");

                if (s2.find(ok1) != string::npos || s2.find(ok2) != string::npos
                    || s2.find(ok3) != string::npos
                    || s2.find(ok4) != string::npos)
                {
                    //
                    // When using -eval, we need to bootstrap the tk-rv engine
                    // synchronously
                    //
                    setEnvVar("BOOTSTRAP_TK_ENGINE_SYNCHRO", "true");
                    av.push_back(avNew[i]);
                    av.push_back(avNew[i + 1]);
                }
                else
                    cerr << "ERROR: sorry -eval/pyeval are no longer allowed "
                            "in rvlink URLs for security reasons."
                         << endl;
                ++i;
            }
            else
                av.push_back(avNew[i]);
        }
    }

    void RvApplication::processNetworkOpts(bool startup)
    {
        Rv::Options& opts = Rv::Options::sharedOptions();

        if (startup && opts.networkOnStartup
            && !networkWindow()->serverRunning())
        {
            networkWindow()->toggleServer();
        }

        if (opts.connectHost || opts.network)
        {
            if (!networkWindow()->serverRunning())
            {
                networkWindow()->toggleServer();
            }
            if (networkWindow()->serverRunning() && opts.connectHost)
            {
                networkWindow()->connectByName(opts.connectHost,
                                               opts.connectPort);
            }
        }
    }

    static string encodeURL(string url, bool encodeEverything = false)
    {
        static bool first = true;
        ;
        static bool disallowed[256];

        if (first)
        {
            for (int i = 0; i < 128; disallowed[i++] = false)
                ;

            //  Non-ascii chars disallowed
            for (int i = 128; i < 256; disallowed[i++] = true)
                ;

            //  Control chars disallowed
            for (int i = 0; i < 32; disallowed[i++] = true)
                ;

            //  Reserved chars disallowed
            disallowed['$'] = true;
            disallowed['&'] = true;
            disallowed['+'] = true;
            disallowed[','] = true;
            disallowed['/'] = true;
            disallowed[':'] = true;
            disallowed[';'] = true;
            disallowed['='] = true;
            disallowed['?'] = true;
            disallowed['@'] = true;

            //  Unsafe chars disallowed
            disallowed[' '] = true;
            disallowed['"'] = true;
            disallowed['<'] = true;
            disallowed['>'] = true;
            disallowed['{'] = true;
            disallowed['}'] = true;
            disallowed['|'] = true;
            disallowed['\\'] = true;
            disallowed['^'] = true;
            disallowed['~'] = true;
            disallowed['['] = true;
            disallowed[']'] = true;
            disallowed['`'] = true;
            disallowed['%'] = true;

            first = false;
        }

        string newURL;
        char hexBuf[8];

        for (int i = 0; i < url.size(); ++i)
        {
            if (encodeEverything)
            {
                sprintf(hexBuf, "%02x", int(url[i]));
                newURL += string(hexBuf);
            }
            else if (disallowed[url[i]])
            {
                sprintf(hexBuf, "%%%02x", int(url[i]));
                newURL += string(hexBuf);
            }
            else
                newURL.push_back(url[i]);
        }
        return newURL;
    }

    string RvApplication::encodeCommandLineURL(int argc, char* argv[])
    {
        string prefix("rvlink://");
        string url;

        for (int i = 1; i < argc; ++i)
        {
            if (string("-encodeURL") != argv[i]
                && string("-bakeURL") != argv[i])
            {
                url = url + " " + argv[i];
            }
        }
        cerr << "INFO: command line for encoding '" << url << "'" << endl;
        url = encodeURL(url, false);
        return prefix + url;
    }

    string RvApplication::bakeCommandLineURL(int argc, char* argv[])
    {
        string prefix("rvlink://baked/");
        string url;

        for (int i = 1; i < argc; ++i)
        {
            if (string("-encodeURL") != argv[i]
                && string("-bakeURL") != argv[i])
            {
                url = url + " " + argv[i];
            }
        }
        cerr << "INFO: command line for baking '" << url << "'" << endl;
        url = encodeURL(url, true);
        return prefix + url;
    }

    void RvApplication::putUrlOnClipboard(string url, string title,
                                          bool doEncode)
    {
        QString qUrl(url.c_str());
        QStringList parts = qUrl.split("://");
        if (parts.size() != 2)
        {
            cerr << "ERROR: illegal url '" << url << "'" << endl;
            return;
        }
        //
        //  Bake rvlink urls, others are merely web-encoded.
        //
        string urlPart;
        if (doEncode)
        {
            if (parts[0] == "rvlink")
                urlPart = "baked/" + encodeURL(parts[1].toUtf8().data(), true);
            else
                urlPart = encodeURL(parts[1].toUtf8().data(), false);
        }
        else
            urlPart = parts[1].toUtf8().data();

        QString fullUrl(parts[0] + "://" + urlPart.c_str());

#if defined(PLATFORM_DARWIN)
        (*putUrlOnMacPasteboardPointer)(fullUrl.toUtf8().data(), title);
#elif defined(PLATFORM_WINDOWS)
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->clear(QClipboard::Clipboard);
        clipboard->setText(fullUrl, QClipboard::Clipboard);
#else
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->clear(QClipboard::Selection);
        clipboard->setText(fullUrl, QClipboard::Selection);
        clipboard->clear(QClipboard::Clipboard);
        clipboard->setText(fullUrl, QClipboard::Clipboard);
#endif
    }

    void RvApplication::initializeQSettings(string altPath)
    {
        QString home = QDir::homePath();

#if defined(PLATFORM_DARWIN)
        QString config(home + "/Library/Preferences");
        if (altPath.size())
            config = altPath.c_str();
        QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope,
                           config);
        QSettings::setPath(QSettings::NativeFormat, QSettings::SystemScope,
                           config);
#elif defined(PLATFORM_WINDOWS)
        QString config =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (altPath.size())
            config = altPath.c_str();
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, config);
        QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope,
                           config);
#else
        QString config(home + "/.config");
        if (altPath.size())
            config = altPath.c_str();
        QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope,
                           config);
        QSettings::setPath(QSettings::NativeFormat, QSettings::SystemScope,
                           config);
#endif
    }

    std::string RvApplication::queryDriverAttribute(const std::string& var)
    {
        return "";
    }

    void RvApplication::setDriverAttribute(const std::string& var,
                                           const std::string& val)
    {
    }

    void RvApplication::setPresentationMode(bool value)
    {
        // Note: prefDialog() initializes m_prefDialog if it isn't already
        // initialized. This is why we are checking m_prefDialog first.
        // Otherwise it is a total waste of time and resources to instantiate
        // the prefDialog here for this simple visibility check.
        if (m_prefDialog && prefDialog()->isVisible())
        {
            //
            //  Not allowed to set while prefs are up
            //

            return;
        }

        if (m_presentationMode != value)
        {
            m_presentationMode = value;
        }

        Rv::Options& opts = Rv::Options::sharedOptions();
        TwkApp::Document* doc = TwkApp::Document::activeDocument();
        Rv::Session* session = static_cast<Rv::Session*>(doc);
        RvDocument* rvDoc = (RvDocument*)session->opaquePointer();

        if (m_presentationMode)
        {
            if (VideoDevice* d = findPresentationDevice(opts.presentDevice))
            {
                // rvDoc->warnOnDriverVSync();

#if 0
            if (opts.vsync && !rvDoc->vsyncDisabled())
            {
                //
                //  Disable vsync
                //

                cout << "INFO: disabling vsync while in presentation mode" << endl;
                rvDoc->setVSync(false);
            }
#endif

                string optionArgs = setVideoDeviceStateFromSettings(d);
                rvDoc->view()->videoDevice()->makeCurrent();

                try
                {
                    if (!d->isOpen())
                    {
                        const DesktopVideoDevice* dd =
                            dynamic_cast<const DesktopVideoDevice*>(d);

                        if (dd
                            && dd->qtScreen()
                                   == qApp->desktop()
                                          ->QDesktopWidget::screenNumber(rvDoc))
                        {
                            TWK_THROW_EXC_STREAM(
                                "Cannot open presentation device for the same "
                                "screen the controller is on");
                        }

                        StringVector vargs;
                        algorithm::split(vargs, optionArgs,
                                         is_any_of(string(" \t\n\r")),
                                         token_compress_on);
                        d->open(vargs);
                    }

#ifdef PLATFORM_DARWIN
                    // cout << "INFO: disabling double buffer in controller" <<
                    // endl; rvDoc->setDoubleBuffer(false);
#endif

                    try
                    {
                        session->setOutputVideoDevice(d);
                    }
                    catch (std::exception& exc)
                    {
                        QWidget* parent = 0;
                        if (doc)
                            parent = (RvDocument*)doc->opaquePointer();
                        QMessageBox box(parent);
                        box.setWindowTitle(
                            tr(UI_APPLICATION_NAME ": Presentation Mode"));
                        QString baseText =
                            QString("The presentation device (%1/%2) is busy "
                                    "or cannot be opened.")
                                .arg(d->module()->name().c_str())
                                .arg(d->name().c_str());
                        QString detailedText =
                            QString(
                                UI_APPLICATION_NAME
                                " failed to open or bind the presentation "
                                "device (%1/%2)."
                                " Check to see if another program is using the "
                                "device"
                                " and that the parameters are valid in the "
                                "preferences."
                                " You can start RV with -noPrefs or "
                                "-resetPrefs if you"
                                " suspect that the preferences are corrupted.\n"
                                " The presentation device video module is "
                                "\"%1\","
                                " the device is \"%2\"")
                                .arg(d->module()->name().c_str())
                                .arg(d->name().c_str());
                        box.setText(baseText + "\n\n" + detailedText);
                        box.setWindowModality(Qt::WindowModal);
                        QPushButton* b0 =
                            box.addButton(tr("Ok"), QMessageBox::AcceptRole);
                        box.setIconPixmap(
                            QPixmap(qApp->applicationDirPath()
                                    + QString(RV_ICON_PATH_SUFFIX))
                                .scaledToHeight(64));
                        box.exec();

                        m_presentationMode = false;
                    }
                }
                catch (std::exception& exc)
                {
                    QWidget* parent = 0;
                    if (doc)
                        parent = (RvDocument*)doc->opaquePointer();
                    QMessageBox box(parent);
                    box.setWindowTitle(
                        tr(UI_APPLICATION_NAME ": Presentation Mode"));
                    QString baseText =
                        QString(
                            "The presentation device (%1/%2) failed to open.")
                            .arg(d->module()->name().c_str())
                            .arg(d->name().c_str());
                    box.setText(baseText + "\n\n" + exc.what());
                    box.setWindowModality(Qt::WindowModal);
                    QPushButton* b0 =
                        box.addButton(tr("Ok"), QMessageBox::AcceptRole);
                    box.setIconPixmap(QPixmap(qApp->applicationDirPath()
                                              + QString(RV_ICON_PATH_SUFFIX))
                                          .scaledToHeight(64));
                    box.exec();

                    m_presentationMode = false;
                }
            }
            else
            {
                cerr << "ERROR: presentation device not found." << endl;
                m_presentationMode = false;
            }
        }
        else
        {
            const VideoDevice* d = session->outputVideoDevice();

            if (d != session->controlVideoDevice())
            {
                const_cast<VideoDevice*>(d)->close();
#ifdef PLATFORM_DARWIN
                // rvDoc->setDoubleBuffer(true);
#endif
            }

            session->setOutputVideoDevice(session->controlVideoDevice());

#if 0
        if (opts.vsync && !rvDoc->vsyncDisabled())
        {
            //
            //  Re-enable vsync
            //

            rvDoc->setVSync(true);
        }
#endif
        }
    }

    string RvApplication::setVideoDeviceStateFromSettings(VideoDevice* d) const
    {
        Rv::Options& opts = Rv::Options::sharedOptions();
        ostringstream str;
        str << d->module()->name() << "/" << d->name();

        QString optFormat = opts.presentFormat;
        QString optData = opts.presentData;

        RV_QSETTINGS;
        settings.beginGroup(str.str().c_str());
        int vformat = settings.value("videoFormat", -1).toInt();
        int v4KTransport = settings.value("video4KTransport", -1).toInt();
        int dformat = settings.value("dataFormat", -1).toInt();
        int syncmode = settings.value("syncMode", -1).toInt();
        int syncsrc = settings.value("syncSource", -1).toInt();
        int dispMode = settings.value("displayMode", -1).toInt();
        int aformat = settings.value("audioFormat", -1).toInt();
        bool useaudio = settings.value("useAsAudioDevice", false).toBool();
        bool uselatency = settings.value("useLatencyForAudio", false).toBool();
        bool swapStereoEyes = settings.value("swapStereoEyes", false).toBool();
        double xl = settings.value("fixedLatency", double(0.0)).toDouble();
        double fl = settings.value("frameLatency", double(0.0)).toDouble();
        QString options = settings.value("additionalOptions", "").toString();
        settings.endGroup();

        if (opts.presentAudio != -1)
            useaudio = opts.presentAudio != 0;

        if (optFormat != "")
        {
            //
            //  Look for exact match first
            //
            int foundVFormat = -1;
            for (size_t i = 0; i < d->numVideoFormats(); i++)
            {
                QString desc = d->videoFormatAtIndex(i).description.c_str();

                if (desc == optFormat)
                {
                    foundVFormat = i;
                    break;
                }
            }
            if (foundVFormat != -1)
                vformat = foundVFormat;
            else
            {
                //
                //  No exact match found, so look for approximate match
                //
                for (size_t i = 0; i < d->numVideoFormats(); i++)
                {
                    QString desc = d->videoFormatAtIndex(i).description.c_str();

                    if (desc.contains(optFormat))
                    {
                        vformat = i;
                        break;
                    }
                }
            }
        }

        if (optData != "")
        {
            //
            //  Look for exact match first
            //
            int foundDFormat = -1;
            for (size_t i = 0; i < d->numDataFormats(); i++)
            {
                QString desc = d->dataFormatAtIndex(i).description.c_str();

                if (desc == optData)
                {
                    foundDFormat = i;
                    break;
                }
            }
            if (foundDFormat != -1)
                dformat = foundDFormat;
            else
            {
                //
                //  No exact match found, so look for approximate match
                //
                for (size_t i = 0; i < d->numDataFormats(); i++)
                {
                    QString desc = d->dataFormatAtIndex(i).description.c_str();

                    if (desc.contains(optData))
                    {
                        dformat = i;
                        break;
                    }
                }
            }
        }

        if (vformat >= 0 && vformat < d->numVideoFormats())
            d->setVideoFormat(vformat);
        if (v4KTransport >= 0 && v4KTransport < d->numVideo4KTransports())
            d->setVideo4KTransport(v4KTransport);
        if (aformat >= 0 && aformat < d->numAudioFormats())
            d->setAudioFormat(aformat);
        if (dformat >= 0 && dformat < d->numDataFormats())
            d->setDataFormat(dformat);
        if (syncmode >= 0 && syncmode < d->numSyncModes())
            d->setSyncMode(syncmode);
        if (syncsrc >= 0 && syncsrc < d->numSyncSources())
            d->setSyncSource(syncsrc);
        if (dispMode >= 0 && dispMode < int(VideoDevice::NotADisplayMode))
            d->setDisplayMode((VideoDevice::DisplayMode)dispMode);
        d->setAudioOutputEnabled(useaudio);
        d->setUseLatencyForAudio(uselatency);
        d->setFixedLatency(xl);
        d->setFrameLatency(fl);
        d->setSwapStereoEyes(swapStereoEyes);
        return options.toUtf8().constData();
    }

    bool RvApplication::isInPresentationMode() { return m_presentationMode; }

    int RvApplication::findVideoModuleIndexByName(const string& name) const
    {
        const VideoModules& mods = videoModules();

        for (size_t i = 0; i < mods.size(); i++)
        {
            if (mods[i]->name() == name)
                return i;
        }

        return -1;
    }

    TwkApp::VideoDevice*
    RvApplication::findPresentationDevice(const std::string& dpath) const
    {
        QString p(dpath.c_str());
        QStringList parts = p.split("/");

        //
        //  Allow for case where the device name itself contained a "/"
        //
        if (parts.size() > 2)
        {
            QStringList newParts;
            newParts.append(parts[0]);
            parts.removeAt(0);
            newParts.append(parts.join("/"));
            parts = newParts;
        }

        if (parts.size() == 2)
        {
            int mindex =
                findVideoModuleIndexByName(parts[0].toUtf8().constData());

            if (mindex >= 0)
            {
                VideoModule* m = videoModules()[mindex].get();
                openVideoModule(m);
                const VideoModule::VideoDevices& devs = m->devices();
                string dname = parts[1].toUtf8().constData();

                for (size_t i = 0; i < devs.size(); i++)
                {
                    if (devs[i]->name() == dname)
                    {
                        return devs[i];
                    }
                }
            }
        }

        return 0;
    }

} // namespace Rv
