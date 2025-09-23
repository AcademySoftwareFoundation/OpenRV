//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __RvCommon__RvBottomViewToolBar__h__
#define __RvCommon__RvBottomViewToolBar__h__
#include <QtCore/QtCore>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QSlider>
#include <TwkApp/EventNode.h>
#include <QAction>
#include <unordered_map>

namespace IPCore
{
    class Session;
}

namespace Rv
{

    class RvBottomViewToolBar
        : public QToolBar
        , public TwkApp::EventNode
    {
        Q_OBJECT

    public:
        RvBottomViewToolBar(QWidget*);
        virtual ~RvBottomViewToolBar();

        void setSession(IPCore::Session*);
        virtual Result receiveEvent(const TwkApp::Event&);

        void build();
        void makeActive(bool);
        void makeActiveFromSettings();

    private slots:
        void smActionTriggered(bool);
        void paintActionTriggered(bool);
        void infoActionTriggered(bool);
        void timelineActionTriggered(bool);
        void timelineMagActionTriggered(bool);
        void networkActionTriggered(bool);

        void ghostTriggered(bool);
        void holdTriggered(bool);

        void backStepTriggered();
        void forwardStepTriggered();
        void backPlayTriggered();
        void forwardPlayTriggered();
        void backMarkTriggered();
        void forwardMarkTriggered();

        void audioSliderChanged(int);
        void audioSliderReleased();
        void audioMuteTriggered(bool);
        void audioMenuTriggered();

        void playModeMenuTriggered(QAction*);
        void playModeMenuUpdate();

    private:
        template <class T> void setVolumeLevel(T& inst, int level);

        void setVolumeIcon();
        void setLiveReviewFilteredActions(bool isDisabled);

        IPCore::Session* m_session;
        QAction* m_smAction;
        QAction* m_paintAction;
        QAction* m_infoAction;
        QAction* m_networkAction;
        QAction* m_timelineMagAction;
        QAction* m_timelineAction;
        QAction* m_holdAction;
        QAction* m_ghostAction;
        QAction* m_backStepAction;
        QAction* m_forwardStepAction;
        QAction* m_backPlayAction;
        QAction* m_forwardPlayAction;
        QAction* m_backMarkAction;
        QAction* m_forwardMarkAction;
        QAction* m_playModeAction;
        QMenu* m_playModeMenu;
        QAction* m_audioAction;
        QSlider* m_audioSlider;
        QMenu* m_audioMenu;
        QAction* m_muteAction;

        QIcon m_playModeOnceIcon;
        QIcon m_playModeLoopIcon;
        QIcon m_playModePingPongIcon;

        QIcon m_volumeZeroIcon;
        QIcon m_volumeLowIcon;
        QIcon m_volumeMediumIcon;
        QIcon m_volumeHighIcon;
        QIcon m_volumeHighMutedIcon;

        std::unordered_map<QAction*, QString> m_liveReviewFilteredActions;
    };

    template <class T>
    void RvBottomViewToolBar::setVolumeLevel(T& inst, int level)
    {
        if (level > 67)
        {
            inst.setIcon(m_volumeHighIcon);
        }
        else if (level > 33)
        {
            inst.setIcon(m_volumeMediumIcon);
        }
        else if (level > 0)
        {
            inst.setIcon(m_volumeLowIcon);
        }
        else
        {
            inst.setIcon(m_volumeZeroIcon);
        }
    }

} // namespace Rv

#endif // __RvCommon__RvBottomViewToolBar__h__
