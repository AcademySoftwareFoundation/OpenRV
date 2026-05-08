//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
//  Builders for the three sections of the bottom toolbar (left / center /
//  right). Split out of RvBottomViewToolBar.cpp to keep build code separate
//  from event handling and slot implementations.
//
#include <RvCommon/RvBottomViewToolBar.h>
#include <QtCore/QVariant>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QMenu>
#include <QtWidgets/QWidgetAction>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QSlider>
#include <QAction>
#include <IPCore/SoundTrackIPNode.h>
#include <IPCore/Session.h>
#include <RvApp/Options.h>

namespace Rv
{
    using namespace IPCore;

    namespace
    {
        constexpr std::string_view playModeDefaultTooltip = "Select playback style";

        QToolButton* makeBtn(QWidget* parent, QAction* action, const char* tbstyle, const char* tbsize = nullptr)
        {
            QToolButton* btn = new QToolButton(parent);
            btn->setDefaultAction(action);
            btn->setProperty("tbstyle", QVariant(QString(tbstyle)));
            if (tbsize)
                btn->setProperty("tbsize", QVariant(QString(tbsize)));
            btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
            return btn;
        }

        QVariant enumCodeMap(int enumVal, QString codeVal)
        {
            QVariantMap m;
            m["enum"] = enumVal;
            m["code"] = codeVal;
            return QVariant(m);
        }
    } // namespace

    QFrame* RvBottomViewToolBar::makeExpandingSpacer()
    {
        QFrame* qf = new QFrame(this);
        qf->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        qf->setMinimumWidth(0);
        qf->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
        qf->setStyleSheet("background-color: transparent");
        return qf;
    }

    void RvBottomViewToolBar::buildLeft(int padding, int width)
    {
        m_leftBox = new QWidget(this);
        m_leftBox->setObjectName("leftBox");
        QHBoxLayout* leftLayout = new QHBoxLayout(m_leftBox);
        leftLayout->setContentsMargins(padding, 0, padding, 0);
        leftLayout->setSpacing(0);
        m_leftBox->setFixedWidth(width);

        struct LeftActionDef
        {
            const char* iconPath;
            const char* tooltip;
            const char* tbstyle;
            QAction** target;
        };
        const LeftActionDef leftDefs[] = {
            {":/images/smanager.png",     "Toggle Session Manager",      "left",     &m_smAction},
            {":/images/paint_48x48.png",  "Toggle Annotation tools",     "interior", &m_paintAction},
            {":/images/about_48x48.png",  "Toggle Image Info",           "interior", &m_infoAction},
            {":/images/ntwrk_48x48.png",  "Toggle RV Networking Dialog", "interior", &m_networkAction},
            {":/images/timeline_mag.png", "Toggle Timeline Magnifier",   "interior", &m_timelineMagAction},
            {":/images/timeline.png",     "Toggle Timeline",             "right",    &m_timelineAction},
        };
        for (const auto& def : leftDefs)
        {
            QAction* a = new QAction("", this);
            a->setIcon(QIcon(def.iconPath));
            a->setToolTip(def.tooltip);
            *def.target = a;
            leftLayout->addWidget(makeBtn(m_leftBox, a, def.tbstyle));
        }

        connect(m_smAction, SIGNAL(triggered(bool)), this, SLOT(smActionTriggered(bool)));
        connect(m_paintAction, SIGNAL(triggered(bool)), this, SLOT(paintActionTriggered(bool)));
        connect(m_infoAction, SIGNAL(triggered(bool)), this, SLOT(infoActionTriggered(bool)));
        connect(m_timelineAction, SIGNAL(triggered(bool)), this, SLOT(timelineActionTriggered(bool)));
        connect(m_timelineMagAction, SIGNAL(triggered(bool)), this, SLOT(timelineMagActionTriggered(bool)));
        connect(m_networkAction, SIGNAL(triggered(bool)), this, SLOT(networkActionTriggered(bool)));

        m_ghostAction = new QAction("", this);
        m_ghostAction->setIcon(QIcon(":/images/ghost.png"));
        m_ghostAction->setToolTip("Ghost");
        m_ghostAction->setCheckable(true);
        leftLayout->addWidget(makeBtn(m_leftBox, m_ghostAction, "left"));

        m_holdAction = new QAction("", this);
        m_holdAction->setIcon(QIcon(":/images/hold.png"));
        m_holdAction->setToolTip("Hold");
        m_holdAction->setCheckable(true);
        leftLayout->addWidget(makeBtn(m_leftBox, m_holdAction, "right"));

        connect(m_ghostAction, SIGNAL(triggered(bool)), this, SLOT(ghostTriggered(bool)));
        connect(m_holdAction, SIGNAL(triggered(bool)), this, SLOT(holdTriggered(bool)));

        leftLayout->addStretch();
        addWidget(m_leftBox);
    }

    void RvBottomViewToolBar::buildCenter()
    {
        m_centerBox = new QWidget(this);
        m_centerBox->setObjectName("centerBox");
        QHBoxLayout* centerLayout = new QHBoxLayout(m_centerBox);
        centerLayout->setContentsMargins(0, 0, 0, 0);
        centerLayout->setSpacing(0);

        m_backStepAction = new QAction("", this);
        m_backStepAction->setIcon(QIcon(":/images/control_bstep.png"));
        m_backStepAction->setToolTip("Step back one frame");
        QToolButton* backStepBtn = makeBtn(m_centerBox, m_backStepAction, "left");
        backStepBtn->setObjectName("backStepButton");
        centerLayout->addWidget(backStepBtn);

        m_forwardStepAction = new QAction("", this);
        m_forwardStepAction->setIcon(QIcon(":/images/control_fstep.png"));
        m_forwardStepAction->setToolTip("Step forward one frame");
        QToolButton* fwdStepBtn = makeBtn(m_centerBox, m_forwardStepAction, "right");
        fwdStepBtn->setObjectName("forwardStepButton");
        centerLayout->addWidget(fwdStepBtn);

        m_backwardPlayAction = new QAction("", this);
        m_backwardPlayAction->setIcon(QIcon(":/images/control_bplay.png"));
        m_backwardPlayAction->setToolTip("Play backwards");
        centerLayout->addWidget(makeBtn(m_centerBox, m_backwardPlayAction, "left", "double"));

        m_forwardPlayAction = new QAction("", this);
        m_forwardPlayAction->setIcon(QIcon(":/images/control_play.png"));
        m_forwardPlayAction->setToolTip("Play forwards");
        centerLayout->addWidget(makeBtn(m_centerBox, m_forwardPlayAction, "right", "double"));

        m_backMarkAction = new QAction("", this);
        m_backMarkAction->setIcon(QIcon(":/images/control_bmark.png"));
        m_backMarkAction->setToolTip("Skip to start of sequence");
        QToolButton* backMarkBtn = makeBtn(m_centerBox, m_backMarkAction, "left");
        backMarkBtn->setObjectName("firstFrameButton");
        centerLayout->addWidget(backMarkBtn);

        m_forwardMarkAction = new QAction("", this);
        m_forwardMarkAction->setIcon(QIcon(":/images/control_fmark.png"));
        m_forwardMarkAction->setToolTip("Skip to end of sequence");
        QToolButton* fwdMarkBtn = makeBtn(m_centerBox, m_forwardMarkAction, "right");
        fwdMarkBtn->setObjectName("lastFrameButton");
        centerLayout->addWidget(fwdMarkBtn);

        connect(m_backStepAction, SIGNAL(triggered()), this, SLOT(backStepTriggered()));
        connect(m_forwardStepAction, SIGNAL(triggered()), this, SLOT(forwardStepTriggered()));
        connect(m_backwardPlayAction, SIGNAL(triggered()), this, SLOT(backPlayTriggered()));
        connect(m_forwardPlayAction, SIGNAL(triggered()), this, SLOT(forwardPlayTriggered()));
        connect(m_backMarkAction, SIGNAL(triggered()), this, SLOT(backMarkTriggered()));
        connect(m_forwardMarkAction, SIGNAL(triggered()), this, SLOT(forwardMarkTriggered()));

        addWidget(m_centerBox);
    }

    void RvBottomViewToolBar::buildRight(int padding, int width)
    {
        Options& opts = Options::sharedOptions();

        m_rightBox = new QWidget(this);
        m_rightBox->setObjectName("rightBox");
        QHBoxLayout* rightLayout = new QHBoxLayout(m_rightBox);
        rightLayout->setContentsMargins(padding, 0, padding, 0);
        rightLayout->setSpacing(0);
        m_rightBox->setFixedWidth(width);

        rightLayout->addStretch();

        m_playModeAction = new QAction("", this);
        m_playModeAction->setToolTip(QString::fromUtf8(playModeDefaultTooltip.data(), playModeDefaultTooltip.size()));
        switch (static_cast<Session::PlayMode>(opts.loopMode))
        {
        case Session::PlayOnce:
            m_playModeAction->setIcon(m_playModeOnceIcon);
            break;
        case Session::PlayPingPong:
            m_playModeAction->setIcon(m_playModePingPongIcon);
            break;
        default:
        case Session::PlayLoop:
            m_playModeAction->setIcon(m_playModeLoopIcon);
            break;
        }

        QToolButton* playModeBtn = makeBtn(m_rightBox, m_playModeAction, "left_menu");
        playModeBtn->setObjectName("playModeButton");
        playModeBtn->setPopupMode(QToolButton::InstantPopup);

        m_playModeMenu = new QMenu(playModeBtn);
        m_playModeMenu->addAction("Playback")->setDisabled(true);
        m_playModeOnceAction = m_playModeMenu->addAction("  Once");
        m_playModeOnceAction->setData(enumCodeMap(Session::PlayOnce, "commands.setPlayMode(PlayOnce)"));
        m_playModeLoopAction = m_playModeMenu->addAction("  Loop");
        m_playModeLoopAction->setData(enumCodeMap(Session::PlayLoop, "commands.setPlayMode(PlayLoop)"));
        m_playModePingPongAction = m_playModeMenu->addAction("  PingPong");
        m_playModePingPongAction->setData(enumCodeMap(Session::PlayPingPong, "commands.setPlayMode(PlayPingPong)"));
        playModeBtn->setMenu(m_playModeMenu);
        rightLayout->addWidget(playModeBtn);

        m_playModeMenu->installEventFilter(this);
        connect(m_playModeMenu, SIGNAL(triggered(QAction*)), this, SLOT(playModeMenuTriggered(QAction*)));
        connect(m_playModeMenu, SIGNAL(aboutToShow()), this, SLOT(playModeMenuUpdate()));

        int volumeLevel = (int)(100.0f * IPCore::SoundTrackIPNode::defaultVolume);
        m_audioAction = new QAction("", this);
        m_audioAction->setToolTip("Audio control");
        setVolumeLevel<QAction>(*m_audioAction, volumeLevel);

        QToolButton* audioBtn = makeBtn(m_rightBox, m_audioAction, "right_menu");
        audioBtn->setPopupMode(QToolButton::InstantPopup);

        m_audioMenu = new QMenu(audioBtn);
        m_audioMenu->setProperty("menuStyle", QVariant(QString("slim")));
        QAction* audioLabel = m_audioMenu->addAction("Volume");
        audioLabel->setDisabled(true);
        QWidgetAction* wa = new QWidgetAction(audioBtn);
        setVolumeLevel<QWidgetAction>(*wa, volumeLevel);
        m_audioSlider = new QSlider(audioBtn);
        m_audioSlider->setProperty("sliderStyle", QVariant(QString("menu")));
        m_audioSlider->setTickInterval(10);
        wa->setDefaultWidget(m_audioSlider);
        m_audioMenu->addAction(wa);
        m_audioMenu->addSeparator();
        m_muteAction = m_audioMenu->addAction("Mute");
        m_muteAction->setIcon(QIcon(":/images/mute_32x32.png"));
        m_muteAction->setCheckable(true);
        audioBtn->setMenu(m_audioMenu);
        rightLayout->addWidget(audioBtn);

        connect(m_muteAction, SIGNAL(triggered(bool)), this, SLOT(audioMuteTriggered(bool)));
        connect(m_audioMenu, SIGNAL(aboutToShow()), this, SLOT(audioMenuTriggered()));
        connect(m_audioSlider, SIGNAL(valueChanged(int)), this, SLOT(audioSliderChanged(int)));
        connect(m_audioSlider, SIGNAL(sliderReleased()), this, SLOT(audioSliderReleased()));

        addWidget(m_rightBox);
    }

} // namespace Rv
