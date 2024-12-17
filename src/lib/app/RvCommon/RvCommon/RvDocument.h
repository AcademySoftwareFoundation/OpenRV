//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __rv_qt__RvDocument__h__
#define __rv_qt__RvDocument__h__
#include <TwkGLF/GL.h>
#include <QtCore/QFileSystemWatcher>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QStackedLayout>
#include <TwkUtil/Notifier.h>
#include <TwkApp/Menu.h>
#include <RvApp/RvSession.h>

class QAction;
class QActionGroup;
class QLabel;
class QMenu;
class QToolBar;

namespace TwkApp
{
    class Menu;
}

namespace Rv
{
    class GLView;
    class DesktopVideoModule;
    class DesktopVideoDevice;
    class RvTopViewToolBar;
    class RvBottomViewToolBar;
    class RvSourceEditor;
    class DisplayLink;

    class RvDocument
        : public QMainWindow
        , public TwkUtil::Notifier
    {
        Q_OBJECT

    public:
        RvDocument();
        virtual ~RvDocument();

        virtual std::string name() const { return "RvDocument"; }

        RvSession* session() const { return m_session; }

        virtual QRect childrenRect();
        void center();
        void resizeToFit(bool placement = false, bool firstTime = false);
        void resizeView(int w, int h);
        void toggleFullscreen(bool firstTime = false);
        void toggleMenuBar();

        bool menuBarShown() const { return m_menuBarShown; };

        bool startupResize() const { return m_startupResize; };

        bool aggressiveSizing() { return m_aggressiveSizing; };

        void setAggressiveSizing(bool v) { m_aggressiveSizing = v; };

        QMenu* mainPopup() const { return m_mainPopup; }

        GLView* view() const;

        const QAction* lastPopupAction() const { return m_lastPopupAction; }

        RvTopViewToolBar* topViewToolBar() const { return m_topViewToolBar; };

        RvBottomViewToolBar* bottomViewToolBar() const
        {
            return m_bottomViewToolBar;
        }

        void setDocumentDisabled(bool window, bool menuBarOnly = false);

        QMenuBar* mb();

        void addWatchFile(const std::string&);
        void removeWatchFile(const std::string&);

        void viewSizeChanged(int w, int h);

        //
        //  The resulting popup is associated with this RvDocument only
        //  and is owned by it. The TwkApp::Menu* will be deleted by the
        //  document at some later time. the Menu is only used once.
        //

        void popupMenu(TwkApp::Menu*, QPoint point, bool shortcuts = true);

        enum DisplayOutputType
        {
            OpenGLDefaultFormat, // whatever you get
            OpenGL8888,          // 8 bits per channel + (maybe) alpha
            OpenGL1010102        // 10 bits per channel + (maybe) 2 bits alpha
        };

        void setStereo(bool);
        void setVSync(bool);
        void setDoubleBuffer(bool);
        void setDisplayOutput(DisplayOutputType);

        void disconnectActions(const QList<QAction*>&);

        void resetGLStateAndPrefs();

        bool vsyncDisabled() const { return m_vsyncDisabled; }

        bool queryDriverVSync() const;
        void checkDriverVSync();
        void warnOnDriverVSync();

        void editSourceNode(const std::string&);

        void physicalVideoDeviceChangedSlot(const TwkApp::VideoDevice*);
        void playStartSlot(const std::string&);
        void playStopSlot(const std::string&);

    protected:
        // Overrides for TwkUtil::Notifier
        virtual bool receive(Notifier*, Notifier*, MessageId, MessageData*);

        void enableActions(bool, QMenu*);

    private slots:
        void menuActivated();
        void aboutToShowMenu();
        void buildMenu();
        void watchedFileChanged(const QString&);
        void frameChanged();
        void resetSizePolicy();
        void lazyDeleteGLView();

    private:
        void purgeMenus();
        void mergeMenu(const TwkApp::Menu*, bool shortcuts = true);
        void convert(QMenu*, const TwkApp::Menu*, bool shortcuts);

        void closeEvent(QCloseEvent*);
        void changeEvent(QEvent*);
        bool event(QEvent*);
        void moveEvent(QMoveEvent*);

        void setBuildMenu();

        void rebuildGLView(bool stereo, bool vsync, bool dbl, int, int, int,
                           int);

    private:
        RvSession* m_session;
        QMenu* m_rvMenu;
        QMenu* m_mainPopup;
        QMenu* m_userPopup;
        TwkApp::Menu* m_userMenu;
        GLView* m_glView;
        GLView* m_oldGLView;
        QWidget* m_viewContainerWidget;
        RvTopViewToolBar* m_topViewToolBar;
        RvBottomViewToolBar* m_bottomViewToolBar;
        QWidget* m_centralWidget;
        QStackedLayout* m_stackedLayout;
        int m_menuBarHeight;
        bool m_menuBarDisable;
        bool m_menuBarShown;
        bool m_startupResize;
        bool m_aggressiveSizing;
        int m_menuExecuting;
        QTimer* m_menuTimer;
        QTimer* m_frameChangedTimer;
        QTimer* m_resetPolicyTimer;
        const QAction* m_lastPopupAction;
        QFileSystemWatcher* m_watcher;
        bool m_currentlyClosing;
        bool m_closeEventReceived;
        bool m_vsyncDisabled;
        RvSourceEditor* m_sourceEditor;
        DisplayLink* m_displayLink;
    };

} // namespace Rv

#endif // __rv-qt__RvDocument__h__
