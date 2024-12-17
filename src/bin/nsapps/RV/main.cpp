//******************************************************************************
// Copyright (c) 2008 Tweak Software. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <DarwinBundle/DarwinBundle.h>
#include <IOproxy/IOproxy.h>
#include <MovieProxy/MovieProxy.h>
#include <MovieSideCar/MovieSideCarIO.h>
#include <ImfHeader.h>
#include <ImfThreading.h>
#include <MovieFB/MovieFB.h>
#include <MovieProcedural/MovieProcedural.h>
#include <Mu/GarbageCollector.h>
#include <MuGL/GLModule.h>
#include <MuIO/IOModule.h>
#include <MuTwkApp/MuInterface.h>
#include <PyTwkApp/PyInterface.h>
#include <QtGui/QtGui>
#include <IPCore/Application.h>
#include <IPCore/NodeDefinition.h>
#include <RvApp/CommandsModule.h>
#include <RvApp/Options.h>
#include <RvApp/PyCommandsModule.h>
#include <RvCommon/QTUtils.h>
#include <RvCommon/RvApplication.h>
#include <RvCommon/RvConsoleWindow.h>
#include <RvCommon/RvDocument.h>
#include <RvCommon/RvPreferences.h>
#include <RvCommon/MuUICommands.h>
#include <RvPackage/PackageManager.h>
#include <RvCommon/PyUICommands.h>
#include <TwkQtCoreUtil/QtConvert.h>
#include <TwkApp/Document.h>
#include <TwkDeploy/Deploy.h>
#include <TwkExc/TwkExcException.h>
#include <TwkFB/IO.h>
#include <TwkFB/TwkFBThreadPool.h>
#include <TwkGLF/GLPixelBufferObjectPool.h>
#include <TwkMovie/MovieIO.h>
#include <TwkUtil/FourCC.h>
#include <TwkUtil/Daemon.h>
#include <TwkUtil/File.h>
#include <TwkUtil/FrameUtils.h>
#include <TwkUtil/PathConform.h>
#include <TwkUtil/SystemInfo.h>
#include <TwkUtil/ThreadName.h>
#include <TwkUtil/MemPool.h>
#include <arg.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <thread>
#include <signal.h>
#include <stdlib.h>
#include <stl_ext/thread_group.h>
#include <string.h>
#include <errno.h>
#include <vector>
#include <gc/gc.h>
#include <sys/sysctl.h>
#include <mach/mach_init.h>
#include <mach/thread_policy.h>
#include <mach/thread_act.h>
#include <RvCommon/GLView.h>

extern const char* rv_mac_dark;
extern const char* rv_mac_aqua;

// RV third party optional customization
#if defined(RV_THIRD_PARTY_CUSTOMIZATION)
extern void rvThirdPartyCustomization(TwkApp::Bundle& bundle, char* licarg);
#endif

//
//  Check that a version actually exists. If we're compiling opt error
//  out in cpp. Otherwise, just note the lack of version info.
//

#if MAJOR_VERSION == 99
#if NDEBUG
// #error ********* NO VERSION INFORMATION ***********
#else
#warning********* NO VERSION INFORMATION ***********
#endif
#endif

using namespace std;
using namespace TwkFB;
using namespace TwkMovie;
using namespace TwkUtil;

extern const char* TweakDark;

static void control_c_handler(int sig)
{
    cout << "INFO: stopped by user" << endl;
    exit(1);
}

static int rtPeriod = 240;
static int rtComputation = 720;
static int rtConstraint = 360;
static int rtPreemptable = 1;

static int set_realtime(int period, int computation, int constraint)
{
    struct thread_time_constraint_policy ttcpolicy;

    ttcpolicy.period = period;           // HZ/160
    ttcpolicy.computation = computation; // HZ/3300;
    ttcpolicy.constraint = constraint;   // HZ/2200;
    ttcpolicy.preemptible = 1;

    if (thread_policy_set(mach_thread_self(), THREAD_TIME_CONSTRAINT_POLICY,
                          (thread_policy_t)&ttcpolicy,
                          THREAD_TIME_CONSTRAINT_POLICY_COUNT)
        != KERN_SUCCESS)
    {
        return 0;
    }

    return 1;
}

void thread_priority_init()
{
    struct thread_time_constraint_policy ttcpolicy;
    int ret, bus_speed, mib[2] = {CTL_HW, HW_BUS_FREQ};
    size_t len;

    len = sizeof(bus_speed);

#if !defined(__aarch64__)
    int ret2 = sysctl(mib, 2, &bus_speed, &len, NULL, 0);
    if (ret2 < 0)
    {
        cout << "ERROR: unable to get system bus frequency: " << ret2 << endl;
        return;
    }
#else
    bus_speed = 100000000;
#endif

    //
    //  The period is the number of cycles over which we are guaranteed
    //  a number of clock cycles equal to the second argument (the
    //  computation). The third argument (the constraint) is the
    //  maximum number of cycles to complete.
    //
    //  from esound example: (160, 3300, 2200)
    //  cdaudio example: (120, 1440, 720)
    //  rv: (240, 720, 360)
    //
    //  My interpretation of this: The denominators below are
    //  Hz. So 240 (rtPeriod) means 240 times a second. The *minimum*
    //  number of slices we'll get is:
    //
    //      240 * (720 / 360) => 580 slices/sec
    //
    //  where 720 is the rtComputation and 360 is rtConstraint.
    //
    //  These will *not* be evenly spaced, but no slice will last
    //  longer than 1/360 seconds probably much shorter. However, if
    //  you add up all the time spent in a second in our slices it
    //  will be at least 1/720 seconds.
    //
    //

    ttcpolicy.period = bus_speed / rtPeriod;
    ttcpolicy.computation = bus_speed / rtComputation;
    ttcpolicy.constraint = bus_speed / rtConstraint;
    ttcpolicy.preemptible = rtPreemptable != 0;

    if (thread_policy_set(mach_thread_self(), THREAD_TIME_CONSTRAINT_POLICY,
                          (int*)&ttcpolicy, THREAD_TIME_CONSTRAINT_POLICY_COUNT)
        != KERN_SUCCESS)
    {
        cout << "INFO: running without real-time scheduling" << endl;
    }
    else
    {
        cout << "INFO: real-time thread priorities set (bus="
             << (bus_speed / 1000) << "kHz)" << endl;
    }
}

string scarfFile(const string& fileName)
{
    ifstream in(fileName.c_str());
    stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

//
//  We happen to know that Mu objects NEVER get stashed in 3rd party
//  static areas so let's just remove all of them.
//

static string qtver;

static int gc_filter(const char* name, void* ptr, size_t size)
{
    string s(name);

    if (s.find("/Library/Frameworks/") != string::npos
        || s.find("/System/Library/Frameworks/") != string::npos
        || s.find("/usr/lib/") != string::npos
        || s.find("imageformats/libq") != string::npos
        || s.find(qtver) != string::npos)
    {
        // cout << "SKIPPING: " << name << endl;
        return 0;
    }
    else
    {
        return 1;
    }
}

int main(int argc, char* argv[])
{
    if (!getenv("HOME"))
    {
        cerr << "ERROR: $HOME is not set in the environment and is required."
             << endl;
        exit(-1);
    }

    //
    //  As of OSX 10.7, interesting settings of the locale can cause crashes in
    //  apple core code.
    //
    if (getenv("LANG"))
    {
        std::string originalLocale = getenv("LANG");
        setenv("ORIGINALLOCAL", originalLocale.c_str(), 1);
    }
    else
    {
        setenv("ORIGINALLOCAL", "en", 1);
    }
    setenv("LANG", "C", 1);
    setenv("LC_ALL", "C", 1);

    // Qt 5.12.1 specific
    // Disable Qt Quick hardware rendering because QwebEngineView conflicts with
    // QGLWidget
    setenv("QT_QUICK_BACKEND", "software",
           0 /* changeFlag : Do not change the existing value */);

    // Qt 5.12.1 specific
    // Prevent Mac from automatically scaling app pixel coordinates in OpenGL
    setenv("QT_MAC_WANTS_BEST_RESOLUTION_OPENGL_SURFACE", "0",
           0); /* changeFlag : Do not change the existing value */

    // Prevent usage of native sibling widgets on Mac. This attribute can be
    // removed if GLView is changed to inherit from QOpenGLWidget.
    QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

    const bool noHighDPISupport = getenv("RV_QT_HDPI_SUPPORT") == nullptr;
    if (noHighDPISupport)
    {
        unsetenv("QT_SCALE_FACTOR");
        unsetenv("QT_SCREEN_SCALE_FACTORS");
        unsetenv("QT_AUTO_SCREEN_SCALE_FACTOR");
        unsetenv("QT_ENABLE_HIGHDPI_SCALING");
        unsetenv("QT_SCALE_FACTOR_ROUNDING_POLICY");
        unsetenv("QT_DEVICE_PIXEL_RATIO");
    }

    const char* pythonPath = getenv("PYTHONPATH");
    if (pythonPath)
        pythonPath = strdup(pythonPath);

#ifndef PLATFORM_WINDOWS
    //
    //  Check the per-process limit on open file descriptors and
    //  reset the soft limit to the hard limit.
    //
    struct rlimit rlim;
    getrlimit(RLIMIT_NOFILE, &rlim);
    rlim_t target = rlim.rlim_max;
    if (OPEN_MAX < rlim.rlim_max)
        target = OPEN_MAX;
    rlim.rlim_cur = target;
    setrlimit(RLIMIT_NOFILE, &rlim);
    getrlimit(RLIMIT_NOFILE, &rlim);
    if (rlim.rlim_cur < target)
    {
        cerr << "WARNING: unable to increase open file limit above "
             << rlim.rlim_cur << endl;
    }
#endif

    {
        string qt(QT_VERSION_STR);
        ostringstream ss;
        ss << "Versions/" << qt[0] << "/Qt";
        qtver = ss.str();
    }

    TwkFB::ThreadPool::initialize();
    TwkUtil::MemPool::initialize();

    string altPrefsPath;
    for (size_t i = 0; i < argc; i++)
    {
        if (i < argc - 1 && !strcmp(argv[i], "-prefsPath"))
            altPrefsPath = argv[i + 1];
    }
    Rv::RvApplication::initializeQSettings(altPrefsPath);

    TwkApp::DarwinBundle bundle("RV", MAJOR_VERSION, MINOR_VERSION,
                                REVISION_NUMBER,
                                true /*register rvlink protocol handler*/);

    bundle.setEnvVar("RV_PYTHONPATH_EXTERNAL", (pythonPath) ? pythonPath : "");

    //
    //  Expand any rvlink urls in argv
    //

    vector<char*> newArgv;

    for (int i = 0; i < argc; ++i)
    {
        if (0 == strncmp("rvlink://", argv[i], 9))
        {
            Rv::RvApplication::parseURL(argv[i], newArgv);
        }
        else
            newArgv.push_back(argv[i]);
    }
    argv = &(newArgv[0]);
    argc = newArgv.size();

    bool noPrefs = false;
    bool resetPrefs = false;

    for (size_t i = 0; i < argc; i++)
    {
        if (!strcmp(argv[i], "-noPrefs"))
            noPrefs = true;
        else if (!strcmp(argv[i], "-resetPrefs"))
            resetPrefs = true;

        else if (!strcmp(argv[i], "-bakeURL"))
        {
            string url = Rv::RvApplication::bakeCommandLineURL(argc, argv);
            cerr << "Baked URL: " << url << endl;
            exit(0);
        }
        else if (!strcmp(argv[i], "-encodeURL"))
        {
            string url = Rv::RvApplication::encodeCommandLineURL(argc, argv);
            cerr << "Encoded URL: " << url << endl;
            exit(0);
        }
        else if (!strcmp(argv[i], "-registerHandler"))
        {
            bundle.registerHandler();
            cerr << "INFO: registering '" << argv[0]
                 << "' as default rvlink protocol handler." << endl;
            exit(0);
        }
    }

    //
    //  Call libarg
    //

    Rv::Options& opts = Rv::Options::sharedOptions();
    if (resetPrefs)
        Rv::RvPreferences::resetPreferencesFile();
    if (!noPrefs)
        Rv::RvPreferences::loadSettingsIntoOptions(opts);
    else
        Rv::PackageManager::setIgnorePrefs(true);
    Rv::Options::manglePerSourceArgs(argv, argc);
    Rv::Options prefOpts = opts;

    vector<char*> nargv;
    TwkApp::DarwinBundle::removePSN(argc, argv, nargv);

    //
    //  Additional options. These are QT UI look  related.
    //

    // Required by the Live Review RV plugin React component's scroll bars
    std::vector<char*> arguments(argv, argv + argc);
    static char enableOverlayScrollbar[] = "--enable-features=OverlayScrollbar";
    arguments.emplace_back(enableOverlayScrollbar);
    argc = static_cast<int>(arguments.size());
    argv = &arguments[0];
    ;

    //
    //  Initialze IMF library for multi-threading
    //

    //
    //  Call the deploy functions
    //

    TWK_DEPLOY_APP_OBJECT dobj(MAJOR_VERSION, MINOR_VERSION, REVISION_NUMBER,
                               argc, argv, RELEASE_DESCRIPTION,
                               "HEAD=" GIT_HEAD);

    Imf::staticInitialize();
    TwkFB::GenericIO::init();    // Initialize TwkFB::GenericIO plugins statics
    TwkMovie::GenericIO::init(); // Initialize TwkMovie::GenericIO plugins
                                 // statics

    //
    // We handle the '--help' flag by changing it to '-help' for
    // argparse to work.
    //
    for (int i = 0; i < argc; ++i)
    {
        if (!strcmp("--help", argv[i]))
        {
            strcpy(argv[i], "-help");
            break;
        }
    }

    //
    //  Parse cmd line args
    //

    IPCore::Application::cacheEnvVars();

    int strictlicense = 0;
    int registerHandler = 0;
    char* prefsPath;
    int sleepTime = 0;

    if (arg_parse(
            nargv.size(), &nargv.front(), "", "", RV_ARG_EXAMPLES, "", "",
            RV_ARG_SEQUENCE_HELP, "", "", RV_ARG_SOURCE_OPTIONS(opts), "", "",
            "", ARG_SUBR(Rv::RvApplication::parseInFiles),
            "Input sequence patterns, images, movies, or directories ",
            RV_ARG_PARSE_OPTIONS(opts), "-strictlicense",
            ARG_FLAG(&strictlicense),
            "Exit rather than consume an RV license if no rvsolo licenses are "
            "available",
            "-prefsPath %S", &prefsPath,
            "Alternate path to preferences directory", "-registerHandler",
            ARG_FLAG(&registerHandler),
            "Register this executable as the default rvlink protocol handler "
            "(OS X only)",
            "-rt %d %d %d %d", &rtPeriod, &rtComputation, &rtConstraint,
            &rtPreemptable, "Real time parameters (default=%d %d %d %d)",
            rtPeriod, rtComputation, rtConstraint, rtPreemptable, "-sleep %d",
            &sleepTime,
            "Sleep (in seconds) before starting to allow attaching debugger",
            NULL)
        < 0)
    {
        exit(-1);
    }

    if (sleepTime > 0)
    {
        cout << "INFO: sleeping " << sleepTime << " seconds" << endl;
        std::this_thread::sleep_for(std::chrono::seconds(sleepTime));
        cout << "INFO: continuing after sleep" << endl;
    }

    if (opts.showVersion)
    {
        cout << MAJOR_VERSION << "." << MINOR_VERSION << "." << REVISION_NUMBER
             << endl;
        exit(0);
    }

    //
    //  Thread scheduling
    //

    thread_priority_init();

    //
    //  Desktop aware: being "desktop aware" may be causing problems
    //  on some systems
    //

    QApplication::setDesktopSettingsAware(opts.qtdesktop == 1);

    //
    //  Banners
    //

    TWK_DEPLOY_SHOW_PROGRAM_BANNER(cout);
    TWK_DEPLOY_SHOW_COPYRIGHT_BANNER(cout);
    TWK_DEPLOY_SHOW_LOCAL_BANNER(cout);

    //
    //  Get CPU info and tell the EXR library to use them all
    //

    if (opts.exrcpus > 0)
    {
        Imf::setGlobalThreadCount(opts.exrcpus);
    }
    else
    {
        Imf::setGlobalThreadCount(TwkUtil::SystemInfo::numCPUs() > 1
                                      ? (TwkUtil::SystemInfo::numCPUs() - 1)
                                      : 1);
    }

    //
    //  Application
    //

    QApplication* app = new QApplication(argc, argv);

    QTranslator* translator = new QTranslator();
    QLocale locale = QLocale(getenv("ORIGINALLOCAL"));
    if (translator->load(locale, QLatin1String("i18n"), "_",
                         QLatin1String(":/translations")))
    {
        app->installTranslator(translator);
    }

#ifdef PLATFORM_DARWIN
    //
    //  As of Qt 4.7, the qt.conf file in Contents/Resources no longer
    //  seems to get Qt to look in our plugins dir.  As a result plugins
    //  can be loaded from /Developer, etc, and chaos ensues.  The below
    //  seems to do the trick.
    //
    QDir dir(QCoreApplication::applicationDirPath());
    dir.cdUp();
    dir.cd("PlugIns");
    dir.cd("Qt");
    QCoreApplication::addLibraryPath(dir.absolutePath());

    dir.cdUp();
    dir.cdUp();
    dir.cd("MacOS");
    QCoreApplication::removeLibraryPath(dir.absolutePath());
    QCoreApplication::removeLibraryPath("/Developer/Applications/Qt/plugins");
#endif

    Rv::RvApplication* rvapp = new Rv::RvApplication(argc, argv);
    app->installEventFilter(rvapp);
    app->setQuitOnLastWindowClosed(true);

    //
    //  Get the bundle info so we can find resource files
    //
    //  init priorities are:
    //
    //      1) -init script
    //      2) $RV_INIT
    //      3) $HOME/.rvrc.mu
    //      4) $RV_HOME/scripts/rv/rvrc.mu
    //      5) ./rvrc.mu
    //

    if (getenv("RV_APP_RVIO"))
        bundle.setEnvVar("RV_APP_RVIO_SET_BY_USER", "true");
    else
        bundle.setEnvVar("RV_APP_RVIO", bundle.executableFile("rvio"));

    bundle.setEnvVar("RV_APP_RV", bundle.executableFile("RV"));
    bundle.setEnvVar("RV_APP_RV_SHORT_NAME", "RV");
    bundle.setEnvVar("RV_APP_MANUAL", bundle.resource("rv_manual", "pdf"));
    bundle.setEnvVar("RV_APP_MANUAL_HTML",
                     bundle.resource("rv_manual", "html"));
    bundle.setEnvVar("RV_APP_SDI_MANUAL",
                     bundle.resource("rvsdi_manual", "pdf"));
    bundle.setEnvVar("RV_APP_SDI_MANUAL_HTML",
                     bundle.resource("rvsdi_manual", "html"));
    bundle.setEnvVar("RV_APP_REFERENCE_MANUAL",
                     bundle.resource("rv_reference", "pdf"));
    bundle.setEnvVar("RV_APP_REFERENCE_MANUAL_HTML",
                     bundle.resource("rv_reference", "html"));
    bundle.setEnvVar("RV_APP_MU_MANUAL", bundle.resource("mu", "pdf"));
    bundle.setEnvVar("RV_APP_GTO_REFERENCE", bundle.resource("gto", "pdf"));
    bundle.setEnvVar("RV_APP_RELEASE_NOTES",
                     bundle.resource("rv_release_notes", "html"));
    bundle.setEnvVar("RV_APP_LICENSES_NOTES",
                     bundle.resource("rv_client_licenses", "html"));
    bundle.addPathToEnvVar("OIIO_LIBRARY_PATH", bundle.appPluginPath("OIIO"));

    //
    //  Find the init file
    //

    string muInitFile = bundle.rcfile("rvrc", "mu", "RV_INIT");
    string pyInitFile = bundle.rcfile("rvrc", "py", "RV_PYINIT");
    bundle.setEnvVar("RV_APP_INIT", muInitFile.c_str());
    bundle.setEnvVar("RV_APP_PYINIT", pyInitFile.c_str());

    // RV third party optional customization
#if defined(RV_THIRD_PARTY_CUSTOMIZATION)
    rvThirdPartyCustomization(bundle, opts.licarg);
#endif

    try
    {
        // TwkFB::GenericIO::loadPlugins("TWK_FB_PLUGIN_PATH");
        TwkFB::loadProxyPlugins("TWK_FB_PLUGIN_PATH");
        TwkMovie::loadProxyPlugins("TWK_MOVIE_PLUGIN_PATH");
    }
    catch (...)
    {
        cerr << "WARNING: a problem occured while loading image plugins."
             << endl;
        cerr << "         some plugins may not have been loaded." << endl;
    }

    TwkMovie::GenericIO::addPlugin(new MovieFBIO());
    TwkMovie::GenericIO::addPlugin(new MovieProceduralIO());

    //
    //  Compile the list of sequencable file extensions
    //

    TwkFB::GenericIO::compileExtensionSet(predicateFileExtensions());

    //
    //  QT starts here
    //

    if (!getenv("RV_DARK"))
        bundle.setEnvVar("RV_DARK", "", true);

    QString csstext;

    if (opts.qtcss)
    {
        string s = scarfFile(opts.qtcss);
        csstext = s.c_str();
    }
    else
    {
        csstext = QString(rv_mac_dark).arg(opts.fontSize1).arg(opts.fontSize2);
    }

    if (!opts.qtstyle || !strcmp(opts.qtstyle, "RV"))
    {
        bundle.setEnvVar("RV_DARK", "1", true);
    }

    app->setStyleSheet(csstext);

    try
    {
        TwkApp::initMu(0, gc_filter);
        TwkApp::initPython(argc, argv);
        Rv::initCommands();
        Rv::initUICommands();

        if (!opts.initializeAfterParsing((noPrefs) ? 0 : &prefOpts))
        {
            rvapp->console()->processLastTextBuffer();
            exit(-1);
        }

        TwkApp::initWithFile(TwkApp::muContext(), TwkApp::muProcess(),
                             TwkApp::muModuleList(), muInitFile.c_str());

        TwkApp::pyInitWithFile(pyInitFile.c_str(), Rv::pyRvAppCommands(),
                               Rv::pyUICommands());
    }
    catch (const exception& e)
    {
        cerr << "ERROR: during initialization: " << e.what() << endl;
        rvapp->console()->processLastTextBuffer();
        exit(-1);
    }

    rvapp->console()->processTextBuffer();

    bool dorun = true;

    rvapp->createNewSessionFromFiles(opts.inputFiles);

    rvapp->processNetworkOpts();

    int rval = 0;

    //
    //  Overwrite python's signal handler
    //

    if (signal(SIGINT, control_c_handler) == SIG_ERR)
    {
        cout << "ERROR: failed to install SIGINT signal handler" << endl;
    }

    TwkUtil::setThreadName("RV Main");

    if (dorun)
    {
        try
        {
#ifdef PLATFORM_DARWIN
            //
            //  As of Qt 4.7, they have moved their event handler registration
            //  deep into the event loop so it's not possible to override it.
            //  Turns out that if we set this attribute at exactly this moment,
            //  it prevents the handler registration overriding without any
            //  other bad effects.  We hope.  XXX
            //
            QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, false);
#endif
            rval = app->exec();
        }
        catch (TwkExc::Exception& exc)
        {
            cerr << exc;
            exit(-1);
        }
        catch (const exception& e)
        {
            cerr << "ERROR: Unhandled exception during execution: " << e.what()
                 << endl;

            exit(-1);
        }
        catch (...)
        {
            cerr << "ERROR: Unhandled unknown exception" << endl;
            exit(-1);
        }
    }

    TwkMovie::GenericIO::shutdown(); // Shutdown TwkMovie::GenericIO plugins
    TwkFB::GenericIO::shutdown();    // Shutdown TwkFB::GenericIO plugins
    TwkFB::ThreadPool::shutdown();   // Shutdown TwkFB ThreadPool

    TwkGLF::UninitPBOPools();

    // Ensure to delete the QApplication before calling finalizePython
    delete rvapp;
    delete app;
    TwkApp::finalizePython();

    return rval;
}
