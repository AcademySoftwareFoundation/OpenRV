//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __rv__RvApplication__h__
#define __rv__RvApplication__h__
#include <RvCommon/RvConsoleApplication.h>
#include <TwkMediaLibrary/Library.h>
#include <PyMediaLibrary/PyMediaLibrary.h>
#include <QtCore/QTimer>
#include <IPCore/Application.h>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMenu>
#include <TwkUtil/Timer.h>
#include <pthread.h>
#include <map>
#include <mutex>
#include <atomic>

namespace TwkApp
{
    class VideoModule;
}

namespace Rv
{
    class RvDocument;
    class RvPreferences;
    class RvProfileManager;
    class RvConsoleWindow;
    class RvNetworkDialog;
    class RvWebManager;
    class RvSession;
    class QTGLVideoDevice;
    class DesktopVideoModule;

    //
    //  class RvApplication
    //
    //  Qt application + RvConsoleApplication (Rv::Application)
    //

    class RvApplication
        : public QObject
        , public RvConsoleApplication
    {
        Q_OBJECT

    public:
        //
        //  Types
        //

        typedef std::vector<StringVector*> NewSessionQueue;
        typedef std::set<RvDocument*> RvDocSet;
        typedef std::vector<const QTGLVideoDevice*> VideoDeviceVector;
        typedef TwkUtil::Timer Timer;
        typedef std::map<DispatchID, DispatchCallback> DispatchMap;
        typedef std::mutex DispatchMutex;

        //
        //  Constructors
        //

        RvApplication(int argc, char** argv);
        virtual ~RvApplication();

        void createNewSessionFromFiles(const StringVector&) override;
        DispatchID dispatchToMainThread(DispatchCallback callback) override;
        void undispatchToMainThread(DispatchID dispatchID,
                                    double maxDuration) override;
        bool isDispatchExecuting(DispatchID dispatchID);

        RvDocument* newSessionFromFiles(const StringVector&);
        RvDocument* rebuildSessionFromFiles(RvSession* s, const StringVector&);

        QMenuBar* macMenuBar() const { return m_macMenuBar; }

        QMenu* macRVMenu() const { return m_macRVMenu; }

        QAction* aboutAction() const { return m_aboutAct; }

        QAction* quitAction() const { return m_quitAct; }

        QAction* prefAction() const { return m_prefAct; }

        QAction* networkAction() const { return m_networkAct; }

        virtual bool eventFilter(QObject* o, QEvent* event) override;

        RvConsoleWindow* console();
        RvNetworkDialog* networkWindow();
        RvWebManager* webManager();
        RvPreferences* prefDialog();
        RvProfileManager* profileManager();

        bool networkDialogRunning() const
        {
            return m_networkDialog ? true : false;
        }

        void processNetworkOpts(bool startup = true);

        virtual void stopTimer();
        virtual void startTimer();

        static void parseURL(const char* s, std::vector<char*>& av);
        static void sessionFromUrl(std::string url);
        static void putUrlOnClipboard(std::string url, std::string title,
                                      bool doEncode = true);
        static std::string encodeCommandLineURL(int argc, char* argv[]);
        static std::string bakeCommandLineURL(int argc, char* argv[]);
        static void initializeQSettings(std::string altPath);

        void setExecutableNameCaps(std::string nm)
        {
            m_executableNameCaps = nm;
        }

        static std::string queryDriverAttribute(const std::string& var);
        static void setDriverAttribute(const std::string& var,
                                       const std::string& val);

        void openVideoModule(VideoModule*) const;
        int findVideoModuleIndexByName(const std::string&) const;
        TwkApp::VideoDevice*
        findPresentationDevice(const std::string& dpath) const;
        std::string setVideoDeviceStateFromSettings(TwkApp::VideoDevice*) const;

        void setPresentationMode(bool);
        bool isInPresentationMode();

        DesktopVideoModule* desktopVideoModule() const
        {
            return m_desktopModule;
        }

        static int parseInFiles(int argc, char* argv[]);
        virtual VideoModule* primaryVideoModule() const override;

    public slots:
        void showNetworkDialog();
        void heartbeat();
        void runCreateSession();
        void about();
        void prefs();
        void quitAll();
        void lazyBuild();
        void dispatchTimeout();

    private:
        NewSessionQueue m_newSessions;
        RvDocSet m_deleteDocs;
        QTimer* m_timer;
        QTimer* m_newTimer;
        QTimer* m_lazyBuildTimer;
        Timer m_fireTimer;
        RvPreferences* m_prefDialog;
        RvProfileManager* m_profileDialog;
        QMenuBar* m_macMenuBar;
        QMenu* m_macRVMenu;
        QAction* m_aboutAct;
        QAction* m_quitAct;
        QAction* m_prefAct;
        QAction* m_networkAct;
        RvConsoleWindow* m_console;
        RvNetworkDialog* m_networkDialog;
        RvWebManager* m_webManager;
        TwkApp::VideoDevice* m_presentationDevice;
        bool m_presentationMode;
        mutable pthread_mutex_t m_deleteLock;
        std::string m_executableNameCaps;
        DesktopVideoModule* m_desktopModule;
        QTimer* m_pingTimer;
        QTimer* m_dispatchTimer;
        DispatchMutex m_dispatchMutex;
        DispatchID m_nextDispatchID{1};
        DispatchMap m_dispatchMap;
        DispatchMap m_executingMap;
        std::atomic<int> m_dispatchAtomicInt;
    };

    inline RvApplication* RvApp()
    {
        return static_cast<RvApplication*>(IPCore::App());
    }

} // namespace Rv

#endif // __rv__RvApplication__h__
