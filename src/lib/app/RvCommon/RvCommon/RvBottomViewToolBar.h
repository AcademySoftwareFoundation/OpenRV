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
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QWidget>
#include <QtWidgets/QFrame>
#include <QtWidgets/qgridlayout.h>
#include <TwkApp/EventNode.h>
#include <QAction>
#include <array>
#include <string>
#include <string_view>

namespace IPCore
{
    class Session;
}

namespace Rv
{

    static constexpr std::string_view playModeDefaultTooltip = "Select playback style";

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
        void buildLeft(int padding);
        void buildCenter();
        void buildRight(int padding);
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

    protected:
        bool eventFilter(QObject* obj, QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void updateOverflow();

    private:
        struct ActionCategoryMapping
        {
            QAction* action;
            std::string_view category;
            QString defaultTooltip;
        };

        template <class T> void setVolumeLevel(T& inst, int level);

        void setVolumeIcon();
        void updateActionAvailability();
        void updatePlayModeButtonState();

        IPCore::Session* m_session;
        QWidget* barContainer;
        QGridLayout* grid;
        QString m_customCannotUseTooltip;
        QString m_customDisabledPrefix;
        QWidget* m_leftBox;
        QWidget* m_centerBox;
        QWidget* m_rightBox;
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
        QAction* m_backwardPlayAction;
        QAction* m_forwardPlayAction;
        QAction* m_backMarkAction;
        QAction* m_forwardMarkAction;
        QAction* m_playModeAction;
        QMenu* m_playModeMenu;
        QAction* m_playModeLoopAction;
        QAction* m_playModeOnceAction;
        QAction* m_playModePingPongAction;
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

        std::array<ActionCategoryMapping, 11> m_actionCategoryMappings;
        bool m_overflowUpdatePending = false;
    };

    template <class T> void RvBottomViewToolBar::setVolumeLevel(T& inst, int level)
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
