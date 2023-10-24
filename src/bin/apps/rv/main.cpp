//******************************************************************************
// Copyright (c) 2001-2005 Tweak Inc. All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************
#ifdef _MSC_VER
    //
    //  We are targetting at least XP, (IE no windows95, etc).
    //
    #define WINVER 0x0501
    #define _WIN32_WINNT 0x0501
    #include <winsock2.h>
    #include <windows.h>
    #include <winnt.h>
    #include <wincon.h>
    #include <pthread.h>
    //
    //  NOTE: win_pthreads, which supplies implement.h, seems
    //  targetted at an earlier version of windows (pre-XP).  If you
    //  include implement.h here, it won't compile.  But as far as I
    //  can tell, it's not needed, so just leave it out.
    //
    //  #include <implement.h>
#endif

#ifdef PLATFORM_LINUX
#include <sched.h>
#endif
#ifndef PLATFORM_LINUX
#include <GL/glew.h>
#endif
#include <RvCommon/GLView.h>
#include "../../utf8Main.h"
#include <RvCommon/QTUtils.h>
#include <RvCommon/RvApplication.h>
#include <RvCommon/RvConsoleWindow.h>
#include <RvCommon/MuUICommands.h>
#include <RvCommon/PyUICommands.h>
#include <RvCommon/RvPreferences.h>
#include <IOproxy/IOproxy.h>
#include <MovieProxy/MovieProxy.h>
#include <MovieSideCar/MovieSideCarIO.h>
#include <ImfHeader.h>
#include <ImfThreading.h>
#include <MovieFB/MovieFB.h>
#include <MovieProcedural/MovieProcedural.h>
#include <MuGL/GLModule.h>
#include <MuIO/IOModule.h>
#include <MuTwkApp/MuInterface.h>
#include <PyTwkApp/PyInterface.h>
#include <IPCore/Application.h>
#include <IPCore/NodeDefinition.h>
#include <RvApp/CommandsModule.h>
#include <RvApp/PyCommandsModule.h>
#include <RvApp/Options.h>
#include <RvPackage/PackageManager.h>
#include <TwkApp/Document.h>
#include <TwkDeploy/Deploy.h>
#include <QTBundle/QTBundle.h>
#include <TwkQtCoreUtil/QtConvert.h>
#include <TwkExc/TwkExcException.h>
#include <TwkFB/IO.h>
#include <TwkFB/TwkFBThreadPool.h>
#include <TwkMovie/MovieIO.h>
#include <TwkUtil/FrameUtils.h>
#include <TwkUtil/SystemInfo.h>
#include <TwkUtil/Daemon.h>
#include <TwkUtil/File.h>
#include <TwkUtil/PathConform.h>
#include <TwkUtil/FourCC.h>
#include <TwkUtil/ThreadName.h>
#include <TwkUtil/MemPool.h>
#include <TwkGLF/GLPixelBufferObjectPool.h>
#include <arg.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <thread>
#include <sched.h>
#include <sstream>
#include <stdlib.h>
#include <stl_ext/thread_group.h>
#include <signal.h>
#include <string.h>
#include <vector>

#ifndef PLATFORM_WINDOWS
    #include <sys/resource.h>
    #include <sys/time.h>
    #include <unistd.h>
#else
    #include <TwkGLFFBO/FBOVideoDevice.h>
    #include <IPCore/ImageRenderer.h>
    #include <QtOpenGL/QGLContext>
#endif

#include <QtWidgets/QtWidgets>
#include <QtGui/QtGui>
#include <RvCommon/RvDocument.h>

#include <QtGlobal>

extern const char* rv_linux_dark;

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
//#error ********* NO VERSION INFORMATION ***********
#else
#ifndef _MSC_VER
#warning ********* NO VERSION INFORMATION ***********
#endif
#endif
#endif

#ifdef WIN32
#define FILE_SEP ";"
#else
#define FILE_SEP ":"
#endif

using namespace std;
using namespace TwkFB;
using namespace TwkMovie;
using namespace TwkUtil;

extern const char* TweakDark;

#if !defined(PLATFORM_WINDOWS)
static void control_c_handler(int sig)
{
    cout << "INFO: stopped by user" << endl;
    exit(1);
}
#endif

void setEnvVar(const string& var, const string& val)
{
#ifdef WIN32
    ostringstream str;
    str << var << "=" << val;
    putenv(str.str().c_str());
#else
    setenv(var.c_str(), val.c_str(), 1);
#endif
}

void setEnvVar(const string& var, const QFileInfo& val)
{
    setEnvVar(var, val.absoluteFilePath().toUtf8().data());
}

void addToEnvVar(const string& var, const string& val)
{
    if (getenv(var.c_str()))
    {
        ostringstream str;
        str << getenv(var.c_str()) << FILE_SEP << val;
        setEnvVar(var, str.str());
    }
    else
    {
        setEnvVar(var, val);
    }
}

string scarfFile(const string& fileName)
{
    ifstream in(fileName.c_str(), ios::binary);
    stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

#define EXECUTABLE_SHORT_NAME      "rv"
#define EXECUTABLE_SHORT_NAME_CAPS "RV"

TwkApp::QTBundle bundle(EXECUTABLE_SHORT_NAME, MAJOR_VERSION, MINOR_VERSION, REVISION_NUMBER);

#ifdef PLATFORM_LINUX
extern "C" int XInitThreads();
#endif

int
utf8Main(int argc, char *argv[])
{
#ifdef PLATFORM_LINUX
    XInitThreads();
#endif

    const bool noHighDPISupport = getenv("RV_QT_HDPI_SUPPORT") == nullptr;
    if (noHighDPISupport)
    {
        qunsetenv("QT_SCALE_FACTOR");
        qunsetenv("QT_SCREEN_SCALE_FACTORS");
        qunsetenv("QT_AUTO_SCREEN_SCALE_FACTOR");
        qunsetenv("QT_ENABLE_HIGHDPI_SCALING");
        qunsetenv("QT_SCALE_FACTOR_ROUNDING_POLICY");
        qunsetenv("QT_DEVICE_PIXEL_RATIO");
    }

    const char* pythonPath = getenv("PYTHONPATH");
    const char* pythonHome = getenv("PYTHONHOME");

    if (pythonPath) pythonPath = strdup(pythonPath);
    if (pythonHome) pythonHome = strdup(pythonHome);

    #ifndef PLATFORM_WINDOWS
        //
        //  Check the per-process limit on open file descriptors and
        //  reset the soft limit to the hard limit.
        //
        struct rlimit rlim;
        getrlimit (RLIMIT_NOFILE, &rlim);
        rlim.rlim_cur = rlim.rlim_max;
        setrlimit (RLIMIT_NOFILE, &rlim);
        getrlimit (RLIMIT_NOFILE, &rlim);
        if (rlim.rlim_cur < rlim.rlim_max)
        {
            cerr << "WARNING: unable to increase open file limit above " << rlim.rlim_cur << endl;
        }
    #else
        _setmaxstdio(2048);
    #endif

    #ifdef PLATFORM_WINDOWS
        //
        //  RV is a "GUI" app, so by default we get no output in the
        //  console, so "rv.exe -help" doesn't work, etc.  Here we
        //  reattach to the starting console, so we do get that
        //  output.
        //
        HANDLE hand = GetStdHandle(STD_OUTPUT_HANDLE);

        if (    (hand == INVALID_HANDLE_VALUE ||
                GetFileType(hand) == FILE_TYPE_UNKNOWN) &&
                AttachConsole(ATTACH_PARENT_PROCESS))
        {
            freopen("CON", "w", stdout);
            freopen("CON", "w", stderr);
        }
    #endif

    //
    //  Check for a valid HOME. If we are on Windows and HOME is not set then
    //  see if we can create one from concatenating HOMEDRIVE and HOMEPATH.
    //
    if (!getenv("HOME"))
    {
        #ifdef PLATFORM_WINDOWS
            if (getenv("HOMEDRIVE") && getenv("HOMEPATH"))
            {
                string home = string(getenv("HOMEDRIVE")) + getenv("HOMEPATH");
                setEnvVar("HOME", home.c_str());
                cout << "INFO: Using '" << home << "' for $HOME." << endl;
            }
            else
        #endif
            {
                cerr << "ERROR: $HOME is not set in the environment and is required." << endl;
                exit(-1);
            }
    }

    if(getenv("LANG"))
    {
        std::string originalLocale = getenv("LANG");
        setEnvVar("ORIGINALLOCAL", originalLocale.c_str());
    }
    else
    {
        setEnvVar("ORIGINALLOCAL", "en");
    }
    setEnvVar("LANG", "C");
    setEnvVar("LC_ALL", "C");
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    TwkFB::ThreadPool::initialize();

    // Qt 5.12.1 specific
    // Disable Qt Quick hardware rendering because QwebEngineView conflicts with QGLWidget
    setEnvVar( "QT_QUICK_BACKEND", "software");

#if defined(PLATFORM_LINUX)
    // Work around for Wacom Tablet issue on linux
    // Note: This is a Qt 5.12.4 regression (https://bugreports.qt.io/browse/QTBUG-77826)
    setEnvVar( "QT_XCB_TABLET_LEGACY_COORDINATES", "1");
#endif

    // Prevent crash at startup when multithreaded upload is enabled
    // (RV Preferences/Rendering/Multithread GPU Upload)
    QApplication::setAttribute(Qt::AA_DontCheckOpenGLContextThreadAffinity);

    TwkUtil::MemPool::initialize();

    string altPrefsPath;
    for (size_t i = 0; i < argc; i++)
    {
        if (i < argc-1 && !strcmp(argv[i], "-prefsPath")) altPrefsPath = argv[i+1];
    }
    Rv::RvApplication::initializeQSettings (altPrefsPath);

    bool noPrefs = false;
    bool resetPrefs = false;
    Q_INIT_RESOURCE(rv);

    //
    //  Expand any rvlink urls in argv
    //

    vector<char *> newArgv;

    for (int i = 0; i < argc; ++i)
    {
        if (0 == strncmp ("rvlink://", argv[i], 9))
        {
            Rv::RvApplication::parseURL (argv[i], newArgv);
        }
        else newArgv.push_back (argv[i]);
    }
    argv = &(newArgv[0]);
    argc = newArgv.size();

    for (size_t i = 0; i < argc; i++)
    {
        if      (!strcmp(argv[i], "-noPrefs")) noPrefs = true;
        else if (!strcmp(argv[i], "-resetPrefs")) resetPrefs = true;
    }
    for (size_t i = 0; i < argc; i++)
    {
        if (!strcmp(argv[i], "-bakeURL"))
        {
            string url = Rv::RvApplication::bakeCommandLineURL (argc, argv);
            cerr << "Baked URL: " << url << endl;
            exit(0);
        }
        if (!strcmp(argv[i], "-encodeURL"))
        {
            string url = Rv::RvApplication::encodeCommandLineURL (argc, argv);
            cerr << "Encoded URL: " << url << endl;
            exit(0);
        }
    }

    Rv::Options& opts = Rv::Options::sharedOptions();
    if (resetPrefs) Rv::RvPreferences::resetPreferencesFile();
    if (!noPrefs) Rv::RvPreferences::loadSettingsIntoOptions(opts);
    else          Rv::PackageManager::setIgnorePrefs(true);
    Rv::Options::manglePerSourceArgs(argv, argc);
    Rv::Options prefOpts = opts;

    //
    //  Additional options. These are QT UI look related.
    //

    //
    //  Call the deploy functions
    //

    TWK_DEPLOY_APP_OBJECT dobj(MAJOR_VERSION,
                               MINOR_VERSION,
                               REVISION_NUMBER,
                               argc, argv,
                               RELEASE_DESCRIPTION,
                               "HEAD=" GIT_HEAD);

    Imf::staticInitialize();

    TwkFB::GenericIO::init(); // Initialize TwkFB::GenericIO plugins statics
    TwkMovie::GenericIO::init(); // Initialize TwkMovie::GenericIO plugins statics

    IPCore::Application::cacheEnvVars();

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

    int   strictlicense  = 0;
    char *prefsPath = 0;
    int sleepTime = 0;

    if (arg_parse
        (argc, argv,
         "", "",
         RV_ARG_EXAMPLES,
         "", "",
         RV_ARG_SEQUENCE_HELP,
         "", "",
         RV_ARG_SOURCE_OPTIONS(opts),
         "", "",
         "", ARG_SUBR(Rv::RvApplication::parseInFiles), "Input sequence patterns, images, movies, or directories ",
         RV_ARG_PARSE_OPTIONS(opts),
         "-strictlicense", ARG_FLAG(&strictlicense), "Exit rather than consume an RV license if no rvsolo licenses are available",
         "-prefsPath %S", &prefsPath, "Alternate path to preferences directory",
#if defined(PLATFORM_LINUX)
         "-scheduler %S", &opts.schedulePolicy, "Thread scheduling policy (may require root)",
         "-priorities %d %d", &opts.displayPriority, &opts.audioPriority, "Set display and audio thread priorities (may require root)",
#endif
         "-sleep %d", &sleepTime, "Sleep (in seconds) before starting to allow attaching debugger",
         NULL) < 0)
    {
        exit(-1);
    }

    //
    //  Qt itself (on linux) will consider the -geometry flag to be
    //  a traditional X11 geometry flag, which it's not, so prevent
    //  this by switching it to "geometri" if it's in argv.
    //
    for (int i = 0; i < argc; ++i) if (strcmp(argv[i], "-geometry") == 0) argv[i][8] = 'i';

    if (sleepTime > 0)
    {
        cout << "INFO: sleeping " << sleepTime << " seconds" << endl;
        std::this_thread::sleep_for( std::chrono::seconds( sleepTime ) );
        cout << "INFO: continuing after sleep" << endl;
    }

    if (opts.showVersion)
    {
        cout << MAJOR_VERSION << "." << MINOR_VERSION << "." << REVISION_NUMBER << endl;
        exit(0);
    }

    //
    //  Desktop aware: being "desktop aware" may be causing problems
    //  on some systems
    //

    QApplication::setDesktopSettingsAware(opts.qtdesktop == 1);

    //
    //  Set scheduling parameters for display thread
    //

    if (opts.schedulePolicy)
    {
#if defined(PLATFORM_LINUX)
        sched_param sp;
        memset(&sp, 0, sizeof(sched_param));
        sp.sched_priority = opts.displayPriority;

        unsigned int policy = SCHED_OTHER;

        if (!strcmp(opts.schedulePolicy, "SCHED_RR")) policy = SCHED_RR;
        else if (!strcmp(opts.schedulePolicy, "SCHED_FIFO")) policy = SCHED_FIFO;

        if (policy != SCHED_OTHER)
        {
            if (sched_setscheduler(0, policy, &sp))
            {
                cout << "ERROR: can't set thread priority" << endl;
            }
        }
        else
        {
            if (setpriority(PRIO_PROCESS, 0, opts.displayPriority))
            {
                cout << "ERROR: can't set thread priority" << endl;
            }
        }
#elif defined(PLATFORM_DARWIN)
        // don't bother here ... use RV or RV64 instead
#else
        // Uh... windoz
#endif
    }


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
                                  ? (TwkUtil::SystemInfo::numCPUs()-1)
                                  : 1);
    }

    //
    //  Additional options. These are QT UI look  related.
    //

    // Required by the Live Review RV plugin React component's scroll bars
    std::vector<char*> arguments( argv, argv + argc );
    static char enableOverlayScrollbar[] = "--enable-features=OverlayScrollbar";
    arguments.emplace_back( enableOverlayScrollbar );
    argc = static_cast<int>( arguments.size() );
    argv = &arguments[0];

    //
    //  Application
    //

    QApplication* app = new QApplication(argc, argv);

    QTranslator* translator = new QTranslator();
    QLocale locale = QLocale(getenv("ORIGINALLOCAL"));
    if (translator->load(locale, QLatin1String("i18n"), "_",QLatin1String(":/translations"))) {
        app->installTranslator(translator);
    }

    QDir dir(QCoreApplication::applicationDirPath());
    dir.cdUp();
    dir.cd("plugins");
    dir.cd("Qt");
    QApplication::addLibraryPath(dir.absolutePath());

    Rv::RvApplication* rvapp = new Rv::RvApplication(argc, argv);

    rvapp->setExecutableNameCaps ("RV");

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

    if (bundle.top() == "")
    {
        cerr << "ERROR: can't locate RV home directory (is it installed in root?)"
             << endl;

        exit(-1);
    }

    bundle.setEnvVar("RV_PYTHONPATH_EXTERNAL", (pythonPath) ? pythonPath : "");
    bundle.setEnvVar("RV_PYTHONHOME_EXTERNAL", (pythonHome) ? pythonHome : "");

    if (getenv("RV_APP_RVIO")) bundle.setEnvVar("RV_APP_RVIO_SET_BY_USER", "true");
    else                       bundle.setEnvVar("RV_APP_RVIO", bundle.executableFile("rvio"));

    bundle.setEnvVar("RV_APP_RV_SHORT_NAME", EXECUTABLE_SHORT_NAME);
    bundle.setEnvVar("RV_APP_RV", bundle.executableFile(EXECUTABLE_SHORT_NAME));
    bundle.setEnvVar("RV_APP_MANUAL", bundle.resource("rv_manual", "pdf"));
    bundle.setEnvVar("RV_APP_MANUAL_HTML", bundle.resource("rv_manual", "html"));
    bundle.setEnvVar("RV_APP_SDI_MANUAL", bundle.resource("rvsdi_manual", "pdf"));
    bundle.setEnvVar("RV_APP_SDI_MANUAL_HTML", bundle.resource("rvsdi_manual", "html"));
    bundle.setEnvVar("RV_APP_REFERENCE_MANUAL", bundle.resource("rv_reference", "pdf"));
    bundle.setEnvVar("RV_APP_REFERENCE_MANUAL_HTML", bundle.resource("rv_reference", "html"));
    bundle.setEnvVar("RV_APP_MU_MANUAL", bundle.resource("mu", "pdf"));
    bundle.setEnvVar("RV_APP_GTO_REFERENCE", bundle.resource("gto", "pdf"));
    bundle.setEnvVar("RV_APP_RELEASE_NOTES", bundle.resource("rv_release_notes", "html"));
    bundle.setEnvVar("RV_APP_LICENSES_NOTES", bundle.resource("rv_client_licenses", "html"));
    bundle.addPathToEnvVar("OIIO_LIBRARY_PATH", bundle.appPluginPath("OIIO"));

    //
    //  Find the init file
    //

    string muInitFile = bundle.rcfile("rvrc", "mu", "RV_INIT");
    string pyInitFile = bundle.rcfile("rvrc", "py", "RV_PYINIT");
    bundle.setEnvVar("RV_APP_INIT", muInitFile.c_str());
    bundle.setEnvVar("RV_APP_PYINIT", pyInitFile.c_str());

    if (opts.initscript) muInitFile = opts.initscript;

    // RV third party optional customization
#if defined(RV_THIRD_PARTY_CUSTOMIZATION)
    rvThirdPartyCustomization(bundle, opts.licarg);
#endif

    try
    {
        TwkFB::loadProxyPlugins("TWK_FB_PLUGIN_PATH");
        TwkMovie::loadProxyPlugins("TWK_MOVIE_PLUGIN_PATH");
    }
    catch (...)
    {
        cerr << "WARNING: a problem occured while loading plugins." << endl;
        cerr << "         some plugins may not have been loaded." << endl;
    }


    TwkMovie::GenericIO::addPlugin(new MovieFBIO());
    TwkMovie::GenericIO::addPlugin(new MovieProceduralIO());

    //
    //  Compile the list of sequencable file extensions
    //

    TwkFB::GenericIO::compileExtensionSet(predicateFileExtensions());

    #ifdef PLATFORM_WINDOWS
	DWORD targetClass = HIGH_PRIORITY_CLASS;
	if (getenv("RV_REALTIME_PRIORITY_CLASS")) targetClass = REALTIME_PRIORITY_CLASS;
	DWORD proClass1 = GetPriorityClass (GetCurrentProcess());
	if (0 == SetPriorityClass (GetCurrentProcess(), targetClass))
	{
	    cerr << "ERROR: SetPriorityClass failed, error " << GetLastError() << endl;
	}
	DWORD proClass2 = GetPriorityClass (GetCurrentProcess());
	if (proClass2 != targetClass)
	{
	    cerr << "WARNING: failed to set Priority Class, class " << hex << showbase <<
		    int(proClass1) << " -> " << int(proClass2) << noshowbase << dec << endl;
	}
    #endif

    //
    //  QT starts here
    //

#ifdef PTW32_STATIC_LIB
    pthread_win32_process_attach_np();
    pthread_win32_process_attach_np();
#endif

    //Rv::dumpApplicationPalette();
    Rv::initializeDefaultPalette();

    if (!getenv("RV_DARK")) bundle.setEnvVar("RV_DARK", "", true);

    QString csstext;

    if (opts.qtcss)
    {
        string s = scarfFile(opts.qtcss);
        csstext = s.c_str();
    }
    else
    {
        csstext = QString(rv_linux_dark)
                        .arg(opts.fontSize1)
                        .arg(opts.fontSize2)
                        .arg(opts.fontSize2-1);
    }

    if (!opts.qtstyle || !strcmp(opts.qtstyle, "RV"))
    {
        bundle.setEnvVar("RV_DARK", "1", true);
        app->setStyle(QStyleFactory::create("Cleanlooks"));
    }
    else
    {
        app->setStyle(QStyleFactory::create(opts.qtstyle));
    }

    app->setStyleSheet(csstext);

    try
    {
        TwkApp::initMu(0);
        TwkApp::initPython(argc, argv);
        Rv::initUICommands();
        Rv::initCommands();

        if (!opts.initializeAfterParsing((noPrefs) ? 0 : &prefOpts))
        {
            rvapp->console()->processLastTextBuffer();
            exit(-1);
        }

        TwkApp::initWithFile(TwkApp::muContext(),
                             TwkApp::muProcess(),
                             TwkApp::muModuleList(),
                             muInitFile.c_str());

        TwkApp::pyInitWithFile(pyInitFile.c_str(), Rv::pyRvAppCommands(), Rv::pyUICommands());
    }
    catch (const exception &e)
    {
        cerr << "ERROR: during initialization: " << e.what() << endl;
        rvapp->console()->processLastTextBuffer();
        exit( -1 );
    }

    //
    //  XXX dummyDev is leaking here.  Best would be to pass it to the App so
    //  that it could delete it after startup, since it's no longer needed at
    //  that point.
    //

    #ifdef PLATFORM_WINDOWS
    TwkGLF::FBOVideoDevice* dummyDev = new TwkGLF::FBOVideoDevice(0, 10, 10, false);
    IPCore::ImageRenderer::queryGL();
    const char* glVersion =(const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    IPCore::Shader::Function::useShadingLanguageVersion(glVersion);
    #endif

    //
    //  Organize the input file list
    //

    bool dorun = true;

    rvapp->createNewSessionFromFiles(opts.inputFiles);

    rvapp->processNetworkOpts();

    int rval = 0;

    rvapp->console()->processTextBuffer();

#if !defined(PLATFORM_WINDOWS)
    //
    //  Overwrite python's signal handler
    //

    if (signal(SIGINT, control_c_handler) == SIG_ERR)
    {
        cout << "ERROR: failed to install SIGINT signal handler" << endl;
    }
#endif

    TwkUtil::setThreadName("RV Main");

    if (dorun)
    {
        try
        {
            rval = app->exec();
        }
        catch (TwkExc::Exception& exc)
        {
            cerr << exc;
            exit(-1);
        }
        catch (const exception &e)
        {
            cerr << "ERROR: Unhandled exception during execution: "
                 << e.what()
                 << endl;

            exit(-1);
        }
        catch (...)
        {
            cerr << "ERROR: Unhandled unknown exception" << endl;
            exit(-1);
        }
    }

#ifdef PTW32_STATIC_LIB
    pthread_win32_thread_detach_np();
    pthread_win32_process_detach_np();
#endif

    TwkFB::ThreadPool::shutdown();
    TwkMovie::GenericIO::shutdown(); // Shutdown TwkMovie::GenericIO plugins
    TwkFB::GenericIO::shutdown(); // Shutdown TwkFB::GenericIO plugins

    TwkGLF::UninitPBOPools();

    // Ensure to delete the QApplication before calling finalizePython
    delete rvapp;
    delete app;
    TwkApp::finalizePython();

    return rval;
}
