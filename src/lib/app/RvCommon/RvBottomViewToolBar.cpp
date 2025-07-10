//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <RvCommon/RvBottomViewToolBar.h>
#include <RvCommon/RvApplication.h>
#include <RvPackage/PackageManager.h>
#include <QtCore/QVariant>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QMenu>
#include <QAction>
#include <QtWidgets/QWidgetAction>
#include <QtWidgets/QFrame>
#include <IPCore/Session.h>
#include <IPCore/IPGraph.h>
#include <IPCore/SoundTrackIPNode.h>
#include <IPCore/PropertyEditor.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <TwkQtCoreUtil/QtConvert.h>
#include <IPCore/IPNode.h>
#include <IPCore/SessionIPNode.h>

#include <RvApp/Options.h>

namespace Rv
{
    using namespace IPCore;
    using namespace std;
    using namespace TwkApp;
    using namespace TwkContainer;
    using namespace boost;
    using namespace TwkQtCoreUtil;

    RvBottomViewToolBar::RvBottomViewToolBar(QWidget* parent)
        : QToolBar("bottom", parent)
        , EventNode("bottomToolBar")
        , m_session(0)
        , m_audioMenu(0)
    {
        // if we don't build up front the style sheet doesn't quite work
        build();
        setObjectName("bottomToolBar");
    }

    void RvBottomViewToolBar::makeActiveFromSettings()
    {
        RV_QSETTINGS;
        settings.beginGroup("ViewToolBars");

        //   Policy is visible by default.
        bool b = settings.value("bottom", true).toBool();

        settings.endGroup();
        makeActive(b);
    }

    void RvBottomViewToolBar::makeActive(bool b)
    {
        if (!m_audioMenu && b)
            build();
        setVisible(b);
    }

    static QVariant enumCodeMap(int enumVal, QString codeVal)
    {
        QVariantMap m;

        m["enum"] = enumVal;
        m["code"] = codeVal;

        return QVariant(m);
    }

    void RvBottomViewToolBar::build()
    {
        if (m_audioMenu)
            return;

        Options& opts = Options::sharedOptions();

        //  Initialize QIcons
        m_playModeOnceIcon = QIcon(":/images/playmode_once_48x48.png");
        m_playModeLoopIcon = QIcon(":/images/playmode_loop_48x48.png");
        m_playModePingPongIcon = QIcon(":/images/playmode_pingpong_48x48.png");

        m_volumeZeroIcon = QIcon(":/images/volume_zero_48x48.png");
        m_volumeLowIcon = QIcon(":/images/volume_low_48x48.png");
        m_volumeMediumIcon = QIcon(":/images/volume_medium_48x48.png");
        m_volumeHighIcon = QIcon(":/images/volume_high_48x48.png");
        m_volumeHighMutedIcon = QIcon(":/images/volume_high_muted_48x48.png");

        setProperty("tbstyle", QVariant(QString("play_controls")));

        QAction* a;
        QMenu* m;
        QToolButton* b;
        QFrame* qf;

        for (size_t i = 0; i < 6; i++)
        {
            a = addAction("");
            b = dynamic_cast<QToolButton*>(widgetForAction(a));
            b->setIcon(m_playModeLoopIcon);
            b->setToolButtonStyle(Qt::ToolButtonIconOnly);
            b->setProperty("tbstyle", QVariant(QString("interior")));

            switch (i)
            {
            case 0:
                b->setProperty("tbstyle", QVariant(QString("left")));
                a->setIcon(QIcon(":/images/smanager.png"));
                a->setToolTip("Toggle Session Manager");
                m_smAction = a;
                break;
            case 1:
                a->setIcon(QIcon(":/images/paint_48x48.png"));
                a->setToolTip("Toggle Annotation tools");
                m_paintAction = a;
                break;
            case 2:
                a->setIcon(QIcon(":/images/about_48x48.png"));
                a->setToolTip("Toggle Image Info");
                m_infoAction = a;
                break;
            case 3:
                a->setIcon(QIcon(":/images/ntwrk_48x48.png"));
                a->setToolTip("Toggle RV Networking Dialog");
                m_networkAction = a;
                break;
            case 4:
                a->setIcon(QIcon(":/images/timeline_mag.png"));
                a->setToolTip("Toggle Timeline Magnifier");
                m_timelineMagAction = a;
                break;
            case 5:
                a->setIcon(QIcon(":/images/timeline.png"));
                a->setToolTip("Toggle Timeline");
                b->setProperty("tbstyle", QVariant(QString("right")));
                m_timelineAction = a;
                break;
            }
        }

        connect(m_smAction, SIGNAL(triggered(bool)), this,
                SLOT(smActionTriggered(bool)));
        connect(m_paintAction, SIGNAL(triggered(bool)), this,
                SLOT(paintActionTriggered(bool)));
        connect(m_infoAction, SIGNAL(triggered(bool)), this,
                SLOT(infoActionTriggered(bool)));
        connect(m_timelineAction, SIGNAL(triggered(bool)), this,
                SLOT(timelineActionTriggered(bool)));
        connect(m_timelineMagAction, SIGNAL(triggered(bool)), this,
                SLOT(timelineMagActionTriggered(bool)));
        connect(m_networkAction, SIGNAL(triggered(bool)), this,
                SLOT(networkActionTriggered(bool)));

        a = addAction("");
        a->setIcon(QIcon(":/images/ghost.png"));
        a->setToolTip("Ghost");
        a->setCheckable(true);
        b = dynamic_cast<QToolButton*>(widgetForAction(a));
        b->setProperty("tbstyle", QVariant(QString("left")));
        b->setToolButtonStyle(Qt::ToolButtonIconOnly);
        b->setStyleSheet(R"(
            QToolButton:checked {
                background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                stop: 0.0 rgb(70,70,255),
                stop: .1 rgb(56,56,220),
                stop: .5 rgb(45,45,210),
                stop: .6 rgb(39,39,190),
                stop: .9 rgb(36,36,220),
                stop: 1.0 rgb(62,62,230));
            }
        )");
        m_ghostAction = a;

        a = addAction("");
        a->setIcon(QIcon(":/images/hold.png"));
        a->setToolTip("Hold");
        a->setCheckable(true);
        b = dynamic_cast<QToolButton*>(widgetForAction(a));
        b->setProperty("tbstyle", QVariant(QString("right")));
        b->setToolButtonStyle(Qt::ToolButtonIconOnly);
        b->setStyleSheet(R"(
            QToolButton:checked {
                background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                stop: 0.0 rgb(70,70,255),
                stop: .1 rgb(56,56,220),
                stop: .5 rgb(45,45,210),
                stop: .6 rgb(39,39,190),
                stop: .9 rgb(36,36,220),
                stop: 1.0 rgb(62,62,230));
            }
        )");
        m_holdAction = a;

        connect(m_ghostAction, SIGNAL(triggered(bool)), this,
                SLOT(ghostTriggered(bool)));
        connect(m_holdAction, SIGNAL(triggered(bool)), this,
                SLOT(holdTriggered(bool)));

        //
        //  Add some expanding space before the play buttons
        //
        qf = new QFrame(this);
        qf->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        qf->setMinimumWidth(0);
        qf->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
        qf->setStyleSheet("background-color: transparent");
        addWidget(qf);

        // play buts
        a = addAction("");
        a->setIcon(QIcon(":/images/control_bstep.png"));
        a->setToolTip("Step back one frame");
        b = dynamic_cast<QToolButton*>(widgetForAction(a));
        b->setProperty("tbstyle", QVariant(QString("left")));
        b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        m_backStepAction = a;

        a = addAction("");
        a->setIcon(QIcon(":/images/control_fstep.png"));
        a->setToolTip("Step forward one frame");
        b = dynamic_cast<QToolButton*>(widgetForAction(a));
        b->setProperty("tbstyle", QVariant(QString("right")));
        b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        m_forwardStepAction = a;

        a = addAction("");
        a->setIcon(QIcon(":/images/control_bplay.png"));
        a->setToolTip("Play backwards");
        b = dynamic_cast<QToolButton*>(widgetForAction(a));
        b->setProperty("tbstyle", QVariant(QString("left")));
        b->setProperty("tbsize", QVariant(QString("double")));
        b->setToolButtonStyle(Qt::ToolButtonIconOnly);
        m_backPlayAction = a;

        a = addAction("");
        a->setIcon(QIcon(":/images/control_play.png"));
        a->setToolTip("Play forwards");
        b = dynamic_cast<QToolButton*>(widgetForAction(a));
        b->setProperty("tbstyle", QVariant(QString("right")));
        b->setProperty("tbsize", QVariant(QString("double")));
        b->setToolButtonStyle(Qt::ToolButtonIconOnly);
        m_forwardPlayAction = a;

        a = addAction("");
        a->setIcon(QIcon(":/images/control_bmark.png"));
        a->setToolTip("Skip to start of sequence");
        b = dynamic_cast<QToolButton*>(widgetForAction(a));
        b->setProperty("tbstyle", QVariant(QString("left")));
        b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        m_backMarkAction = a;

        a = addAction("");
        a->setIcon(QIcon(":/images/control_fmark.png"));
        a->setToolTip("Skip to end of sequence");
        b = dynamic_cast<QToolButton*>(widgetForAction(a));
        b->setProperty("tbstyle", QVariant(QString("right")));
        b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        m_forwardMarkAction = a;

        connect(m_backStepAction, SIGNAL(triggered()), this,
                SLOT(backStepTriggered()));
        connect(m_forwardStepAction, SIGNAL(triggered()), this,
                SLOT(forwardStepTriggered()));
        connect(m_backPlayAction, SIGNAL(triggered()), this,
                SLOT(backPlayTriggered()));
        connect(m_forwardPlayAction, SIGNAL(triggered()), this,
                SLOT(forwardPlayTriggered()));
        connect(m_backMarkAction, SIGNAL(triggered()), this,
                SLOT(backMarkTriggered()));
        connect(m_forwardMarkAction, SIGNAL(triggered()), this,
                SLOT(forwardMarkTriggered()));

        //
        //  Add some expanding space after the play buttons
        //
        qf = new QFrame(this);
        qf->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        qf->setMinimumWidth(0);
        qf->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
        qf->setStyleSheet("background-color: transparent");
        addWidget(qf);

        //
        //  This expanding space will not go to more than 90 pixels.  It's to
        //  "counter balance" the buttons on the other side, so the play control
        //  buttons stay in the center of the view (roughly).
        //
        qf = new QFrame(this);
        qf->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        qf->setMinimumWidth(0);
        qf->setMaximumWidth(90);
        qf->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
        qf->setStyleSheet("background-color: transparent");
        addWidget(qf);

        m_playModeAction = addAction("");
        m_playModeAction->setToolTip("Select playback style");
        b = dynamic_cast<QToolButton*>(widgetForAction(m_playModeAction));

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

        b->setProperty("tbstyle", QVariant(QString("left_menu")));
        b->setToolButtonStyle(Qt::ToolButtonIconOnly);
        b->setPopupMode(QToolButton::InstantPopup);

        m_playModeMenu = new QMenu(b);
        m_playModeMenu->addAction("Playback")->setDisabled(true);
        m_playModeMenu->addAction("  Once")->setData(
            enumCodeMap(Session::PlayOnce, "commands.setPlayMode(PlayOnce)"));
        m_playModeMenu->addAction("  Loop")->setData(
            enumCodeMap(Session::PlayLoop, "commands.setPlayMode(PlayLoop)"));
        m_playModeMenu->addAction("  PingPong")
            ->setData(enumCodeMap(Session::PlayPingPong,
                                  "commands.setPlayMode(PlayPingPong)"));
        //  m_playModeMenu->addAction("  Loop with Black 0.5 Second");
        //  m_playModeMenu->addAction("  Loop with Black 1.0 Second");

        b->setMenu(m_playModeMenu);

        connect(m_playModeMenu, SIGNAL(triggered(QAction*)), this,
                SLOT(playModeMenuTriggered(QAction*)));
        connect(m_playModeMenu, SIGNAL(aboutToShow()), this,
                SLOT(playModeMenuUpdate()));

        int volumeLevel =
            (int)(100.0f * IPCore::SoundTrackIPNode::defaultVolume);
        m_audioAction = addAction("");
        m_audioAction->setToolTip("Audio control");
        b = dynamic_cast<QToolButton*>(widgetForAction(m_audioAction));
        setVolumeLevel<QToolButton>(*b, volumeLevel);
        b->setProperty("tbstyle", QVariant(QString("right_menu")));
        b->setToolButtonStyle(Qt::ToolButtonIconOnly);
        b->setPopupMode(QToolButton::InstantPopup);
        m = new QMenu(b);
        m->setProperty("menuStyle", QVariant(QString("slim")));
        QAction* audio = m->addAction("Volume");
        audio->setDisabled(true);
        QWidgetAction* wa = new QWidgetAction(b);
        setVolumeLevel<QWidgetAction>(*wa, volumeLevel);
        m_audioSlider = new QSlider(b);
        m_audioSlider->setProperty("sliderStyle", QVariant(QString("menu")));
        m_audioSlider->setTickInterval(10);
        wa->setDefaultWidget(m_audioSlider);
        m->addAction(wa);
        m->addSeparator();
        m_muteAction = m->addAction("Mute");
        m_muteAction->setIcon(QIcon(":/images/mute_32x32.png"));
        m_muteAction->setCheckable(true);
        b->setMenu(m);
        m_audioMenu = m;

        connect(m_muteAction, SIGNAL(triggered(bool)), this,
                SLOT(audioMuteTriggered(bool)));
        connect(m_audioMenu, SIGNAL(aboutToShow()), this,
                SLOT(audioMenuTriggered()));
        connect(m_audioSlider, SIGNAL(valueChanged(int)), this,
                SLOT(audioSliderChanged(int)));
        connect(m_audioSlider, SIGNAL(sliderReleased()), this,
                SLOT(audioSliderReleased()));

        m_liveReviewFilteredActions = {
            {m_smAction, m_smAction->toolTip()},
            {m_paintAction, m_paintAction->toolTip()},
            {m_holdAction, m_holdAction->toolTip()},
            {m_ghostAction, m_ghostAction->toolTip()},
            {m_backStepAction, m_backStepAction->toolTip()},
            {m_forwardStepAction, m_forwardStepAction->toolTip()},
            {m_backPlayAction, m_backPlayAction->toolTip()},
            {m_forwardPlayAction, m_forwardPlayAction->toolTip()},
            {m_backMarkAction, m_backMarkAction->toolTip()},
            {m_forwardMarkAction, m_forwardMarkAction->toolTip()},
            {m_playModeAction, m_playModeAction->toolTip()},
        };

        if (m_session)
            setSession(m_session);
    }

    RvBottomViewToolBar::~RvBottomViewToolBar() {}

    void RvBottomViewToolBar::setSession(Session* s)
    {
        m_session = s;

        if (m_audioMenu)
        {
            listenTo(s);
        }
    }

    EventNode::Result RvBottomViewToolBar::receiveEvent(const Event& event)
    {
        if (const GenericStringEvent* gevent =
                dynamic_cast<const GenericStringEvent*>(&event))
        {
            const string& name = event.name();
            const string& contents = gevent->stringContent();

            if (name == "graph-state-change")
            {
                vector<string> parts;
                algorithm::split(parts, contents, is_any_of(string(".")));
                IPGraph::PropertyVector props;

                IPNode* node = m_session->graph().findNode(parts.front());
                m_session->graph().findProperty(m_session->currentFrame(),
                                                props, contents);

                if (!node)
                {
                    cout << "ERROR: can't find node " << parts.front() << endl;
                    return EventAcceptAndContinue;
                }

                if (dynamic_cast<SoundTrackIPNode*>(node))
                {
                    if (parts.back() == "volume"
                        && parts[parts.size() - 2] == "audio")
                    {
                        if (FloatProperty* fp =
                                dynamic_cast<FloatProperty*>(props[0]))
                        {
                            m_audioSlider->setValue(fp->front() * 99.0);
                        }
                    }

                    if (parts.back() == "mute"
                        && parts[parts.size() - 2] == "audio")
                    {
                        setVolumeIcon();
                    }
                }
            }
            else if (name == "play-start")
            {
                QToolButton* bb = dynamic_cast<QToolButton*>(
                    widgetForAction(m_backPlayAction));
                QToolButton* bf = dynamic_cast<QToolButton*>(
                    widgetForAction(m_forwardPlayAction));

                if (m_session->inc() == -1)
                {
                    bb->setIcon(QIcon(":/images/control_pause.png"));
                }
                else
                {
                    bf->setIcon(QIcon(":/images/control_pause.png"));
                }
            }
            else if (name == "play-stop")
            {
                QToolButton* bb = dynamic_cast<QToolButton*>(
                    widgetForAction(m_backPlayAction));
                QToolButton* bf = dynamic_cast<QToolButton*>(
                    widgetForAction(m_forwardPlayAction));

                bb->setIcon(QIcon(":/images/control_bplay.png"));
                bf->setIcon(QIcon(":/images/control_play.png"));
            }
            else if (name == "play-mode-changed")
            {
                if (QToolButton* b = dynamic_cast<QToolButton*>(
                        widgetForAction(m_playModeAction)))
                {
                    switch (m_session->playMode())
                    {
                    case 0: // Session::PlayLoop
                        b->setIcon(m_playModeLoopIcon);
                        break;
                    case 1: // Session::PlayOnce
                        b->setIcon(m_playModeOnceIcon);
                        break;
                    case 2: // Session::PlayPingPong
                        b->setIcon(m_playModePingPongIcon);
                        break;
                    }
                }
            }
            else if (name == "update-ghost-button")
            {
                bool isChecked = (contents == "1");
                m_ghostAction->setChecked(isChecked);

                if (m_session->filterLiveReviewEvents())
                {
                    m_ghostAction->setDisabled(true);
                }
            }
            else if (name == "update-hold-button")
            {
                bool isChecked = (contents == "1");
                m_holdAction->setChecked(isChecked);

                if (m_session->filterLiveReviewEvents())
                {
                    m_holdAction->setDisabled(true);
                }
            }
            else if (name == "internal-sync-presenter-changed"
                     || name == "sync-session-ended")
            {
                bool isDisabled = m_session->filterLiveReviewEvents();
                setLiveReviewFilteredActions(isDisabled);
            }
        }

        return EventAcceptAndContinue;
    }

    //
    //  SLOTS
    //

    void RvBottomViewToolBar::smActionTriggered(bool b)
    {
        m_session->userGenericEvent("mode-manager-toggle-mode",
                                    "session_manager");
    }

    void RvBottomViewToolBar::paintActionTriggered(bool)
    {
        m_session->userGenericEvent("mode-manager-toggle-mode",
                                    "annotate_mode");
    }

    void RvBottomViewToolBar::infoActionTriggered(bool)
    {
        m_session->userGenericEvent("toggle-hud-info-widget", "");
    }

    void RvBottomViewToolBar::timelineActionTriggered(bool)
    {
        m_session->userGenericEvent("toggle-hud-timeline-widget", "");
    }

    void RvBottomViewToolBar::timelineMagActionTriggered(bool)
    {
        m_session->userGenericEvent("toggle-hud-timeline-mag-widget", "");
    }

    void RvBottomViewToolBar::networkActionTriggered(bool)
    {
        RvApp()->showNetworkDialog();
    }

    void RvBottomViewToolBar::ghostTriggered(bool isChecked)
    {
        auto* sessionNode = m_session->graph().sessionNode();
        if (sessionNode != nullptr)
        {
            sessionNode->setProperty<IPNode::IntProperty>("paintEffects.ghost",
                                                          isChecked ? 1 : 0);
            m_session->userGenericEvent("graph-state-change",
                                        sessionNode->name()
                                            + ".paintEffects.ghost");
        }
    }

    void RvBottomViewToolBar::holdTriggered(bool isChecked)
    {
        auto* sessionNode = m_session->graph().sessionNode();
        if (sessionNode != nullptr)
        {
            sessionNode->setProperty<IPNode::IntProperty>("paintEffects.hold",
                                                          isChecked ? 1 : 0);
            m_session->userGenericEvent("graph-state-change",
                                        sessionNode->name()
                                            + ".paintEffects.hold");
        }
    }

    void RvBottomViewToolBar::backStepTriggered()
    {
        m_session->userGenericEvent("remote-eval",
                                    "extra_commands.stepBackward(1)");
    }

    void RvBottomViewToolBar::forwardStepTriggered()
    {
        m_session->userGenericEvent("remote-eval",
                                    "extra_commands.stepForward(1)");
    }

    void RvBottomViewToolBar::backPlayTriggered()
    {
        //
        //  Don't configure button state here, since that'll happen when we get
        //  the resulting events, if the session does in fact begin to play, or
        //  stop.
        //

        if (m_session->isPlaying() && m_session->inc() == -1)
        {
            m_session->userGenericEvent("remote-eval", "commands.stop()");
        }
        else
        {
            // If we are going from scrubbing to play; we must turn off
            // scrubbing.
            if (m_session->isScrubbingAudio())
                m_session->userGenericEvent(
                    "remote-eval", "commands.scrubAudio(false); "
                                   "commands.setInc(-1); commands.play();");
            else
                m_session->userGenericEvent(
                    "remote-eval", "commands.setInc(-1); commands.play();");
        }
    }

    void RvBottomViewToolBar::forwardPlayTriggered()
    {
        //
        //  Don't configure button state here, since that'll happen when we get
        //  the resulting events, if the session does in fact begin to play, or
        //  stop.
        //

        if (m_session->isPlaying() && m_session->inc() == 1)
        {
            m_session->userGenericEvent("remote-eval", "commands.stop()");
        }
        else
        {
            // If we are going from scrubbing to play; we must turn off
            // scrubbing.
            if (m_session->isScrubbingAudio())
                m_session->userGenericEvent(
                    "remote-eval", "commands.scrubAudio(false); "
                                   "commands.setInc(1); commands.play();");
            else
                m_session->userGenericEvent(
                    "remote-eval", "commands.setInc(1); commands.play();");
        }
    }

    void RvBottomViewToolBar::backMarkTriggered()
    {
        m_session->userGenericEvent("remote-eval",
                                    "rvui.previousMarkedFrame()");
    }

    void RvBottomViewToolBar::forwardMarkTriggered()
    {
        m_session->userGenericEvent("remote-eval", "rvui.nextMarkedFrame()");
    }

    void RvBottomViewToolBar::audioSliderReleased()
    {
        //
        //  More mac-like -- hude the "menu" immediately after using
        //

        m_audioMenu->hide();
    }

    void RvBottomViewToolBar::audioSliderChanged(int value)
    {
        const float v = float(value) / 99.0;
        FloatPropertyEditor editor(m_session->graph(),
                                   m_session->currentFrame(),
                                   "#RVSoundTrack.audio.volume");

        // Has the volume changed?
        if (v != editor.value())
        {
            // Turn off the mute audio first
            audioMuteTriggered(false);
            m_muteAction->setChecked(false);

            editor.setValue(v);
        }
    }

    void RvBottomViewToolBar::setVolumeIcon()
    {
        if (QToolButton* b =
                dynamic_cast<QToolButton*>(widgetForAction(m_audioAction)))
        {
            IntPropertyEditor editor(m_session->graph(),
                                     m_session->currentFrame(),
                                     "#RVSoundTrack.audio.mute");

            if (editor.value())
            {
                b->setIcon(m_volumeHighMutedIcon);
            }
            else
            {
                int volumeLevel = m_audioSlider->value(); // 0 - 99;

                setVolumeLevel<QToolButton>(*b, volumeLevel);
            }
        }
    }

    void RvBottomViewToolBar::audioMuteTriggered(bool v)
    {
        IntPropertyEditor editor(m_session->graph(), m_session->currentFrame(),
                                 "#RVSoundTrack.audio.mute");
        editor.setValue(v ? 1 : 0);

        setVolumeIcon();
    }

    void RvBottomViewToolBar::audioMenuTriggered()
    {
        FloatPropertyEditor veditor(m_session->graph(),
                                    m_session->currentFrame(),
                                    "#RVSoundTrack.audio.volume");

        IntPropertyEditor meditor(m_session->graph(), m_session->currentFrame(),
                                  "#RVSoundTrack.audio.mute");

        m_audioSlider->setValue(veditor.value() * 99.0);

        m_muteAction->setCheckable(true);
        m_muteAction->setChecked(meditor.value() == 1);

        setVolumeIcon();
    }

    void RvBottomViewToolBar::playModeMenuUpdate()
    {
        if (!m_session)
            return;

        for (int i = 0; i < m_playModeMenu->actions().size(); ++i)
        {
            QAction* a = m_playModeMenu->actions()[i];

            if (!a->isEnabled())
                continue;

            a->setCheckable(true);
            a->setChecked(a->data().toMap()["enum"].toInt()
                          == m_session->playMode());
        }
    }

    void RvBottomViewToolBar::playModeMenuTriggered(QAction* a)
    {
        if (!m_session)
            return;

        m_session->userGenericEvent(
            "remote-eval",
            UTF8::qconvert(a->data().toMap()["code"].toString()));

        if (QToolButton* b =
                dynamic_cast<QToolButton*>(widgetForAction(m_playModeAction)))
        {
            switch (m_session->playMode())
            {
            case Session::PlayOnce:
                b->setIcon(m_playModeOnceIcon);
                break;

            case Session::PlayLoop:
                b->setIcon(m_playModeLoopIcon);
                break;

            case Session::PlayPingPong:
                b->setIcon(m_playModePingPongIcon);
                break;
            }
        }
    }

    void RvBottomViewToolBar::setLiveReviewFilteredActions(bool isDisabled)
    {
        for (const auto& button : m_liveReviewFilteredActions)
        {
            QAction* action = button.first;
            QString tooltipText = button.second;

            if (action != nullptr)
            {
                action->setDisabled(isDisabled);
                action->setToolTip(isDisabled ? "You must be presenter to use"
                                              : tooltipText);
            }
        }
    }

} // namespace Rv
