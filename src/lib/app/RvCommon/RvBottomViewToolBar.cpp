//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <QtWidgets/qgridlayout.h>
#include <QtWidgets/qsizepolicy.h>
#include <RvCommon/RvBottomViewToolBar.h>
#include <RvCommon/RvApplication.h>
#include <RvPackage/PackageManager.h>
#include <QtCore/QVariant>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolTip>
#include <QAction>
#include <QtWidgets/QWidgetAction>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QWidget>
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

    static constexpr std::string_view playModeDefaultTooltip = "Select playback style";

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

    void RvBottomViewToolBar::build()
    {
        if (m_audioMenu)
            return;

        m_playModeOnceIcon = QIcon(":/images/playmode_once_48x48.png");
        m_playModeLoopIcon = QIcon(":/images/playmode_loop_48x48.png");
        m_playModePingPongIcon = QIcon(":/images/playmode_pingpong_48x48.png");

        m_volumeZeroIcon = QIcon(":/images/volume_zero_48x48.png");
        m_volumeLowIcon = QIcon(":/images/volume_low_48x48.png");
        m_volumeMediumIcon = QIcon(":/images/volume_medium_48x48.png");
        m_volumeHighIcon = QIcon(":/images/volume_high_48x48.png");
        m_volumeHighMutedIcon = QIcon(":/images/volume_high_muted_48x48.png");

        setProperty("tbstyle", QVariant(QString("play_controls")));

        constexpr int sideBoxPadding = 5;

        buildLeft(sideBoxPadding);
        buildRight(sideBoxPadding);
        buildCenter();

        barContainer = new QWidget(this);
        barContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        grid = new QGridLayout(barContainer);
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setSpacing(0);

        grid->addWidget(m_leftBox, 0, 0, Qt::AlignLeft | Qt::AlignVCenter);
        grid->addWidget(m_rightBox, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
        grid->addWidget(m_centerBox, 0, 0, Qt::AlignCenter);

        addWidget(barContainer);

        m_actionCategoryMappings = {{
            {m_smAction, IPCore::EventCategories::sessionmanagerCategory, m_smAction->toolTip()},
            {m_paintAction, IPCore::EventCategories::annotateCategory, m_paintAction->toolTip()},
            {m_holdAction, IPCore::EventCategories::holdAndGhostCategory, m_holdAction->toolTip()},
            {m_ghostAction, IPCore::EventCategories::holdAndGhostCategory, m_ghostAction->toolTip()},
            {m_backStepAction, IPCore::EventCategories::playcontrolCategory, m_backStepAction->toolTip()},
            {m_forwardStepAction, IPCore::EventCategories::playcontrolCategory, m_forwardStepAction->toolTip()},
            {m_backwardPlayAction, IPCore::EventCategories::backwardplayCategory, m_backwardPlayAction->toolTip()},
            {m_forwardPlayAction, IPCore::EventCategories::playcontrolCategory, m_forwardPlayAction->toolTip()},
            {m_playModeAction, IPCore::EventCategories::playcontrolCategory, m_playModeAction->toolTip()},
            {m_backMarkAction, IPCore::EventCategories::playcontrolCategory, m_backMarkAction->toolTip()},
            {m_forwardMarkAction, IPCore::EventCategories::playcontrolCategory, m_forwardMarkAction->toolTip()},
        }};

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
        if (const GenericStringEvent* gevent = dynamic_cast<const GenericStringEvent*>(&event))
        {
            const string& name = event.name();
            const string& contents = gevent->stringContent();

            if (name == "graph-state-change")
            {
                vector<string> parts;
                algorithm::split(parts, contents, is_any_of(string(".")));
                IPGraph::PropertyVector props;

                IPNode* node = m_session->graph().findNode(parts.front());
                m_session->graph().findProperty(m_session->currentFrame(), props, contents);

                if (!node)
                {
                    cout << "ERROR: can't find node " << parts.front() << endl;
                    return EventAcceptAndContinue;
                }

                if (dynamic_cast<SoundTrackIPNode*>(node))
                {
                    if (parts.back() == "volume" && parts[parts.size() - 2] == "audio")
                    {
                        if (FloatProperty* fp = dynamic_cast<FloatProperty*>(props[0]))
                        {
                            m_audioSlider->setValue(fp->front() * 99.0);
                        }
                    }

                    if (parts.back() == "mute" && parts[parts.size() - 2] == "audio")
                    {
                        setVolumeIcon();
                    }
                }
            }
            else if (name == "play-start")
            {
                if (m_session->inc() == -1)
                {
                    m_backwardPlayAction->setIcon(QIcon(":/images/control_pause.png"));
                }
                else
                {
                    m_forwardPlayAction->setIcon(QIcon(":/images/control_pause.png"));
                }
            }
            else if (name == "play-stop")
            {
                m_backwardPlayAction->setIcon(QIcon(":/images/control_bplay.png"));
                m_forwardPlayAction->setIcon(QIcon(":/images/control_play.png"));
            }
            else if (name == "play-mode-changed")
            {
                switch (m_session->playMode())
                {
                case 0: // Session::PlayLoop
                    m_playModeAction->setIcon(m_playModeLoopIcon);
                    break;
                case 1: // Session::PlayOnce
                    m_playModeAction->setIcon(m_playModeOnceIcon);
                    break;
                case 2: // Session::PlayPingPong
                    m_playModeAction->setIcon(m_playModePingPongIcon);
                    break;
                }
            }
            else if (name == "update-ghost-button")
            {
                bool isChecked = (contents == "1");
                m_ghostAction->setChecked(isChecked);
            }
            else if (name == "update-hold-button")
            {
                bool isChecked = (contents == "1");
                m_holdAction->setChecked(isChecked);
            }
            else if (name == "event-category-state-changed")
            {
                updateActionAvailability();
            }
            else if (name == "toolbar-set-cannot-use-tooltip")
            {
                m_customCannotUseTooltip = QString::fromUtf8(contents.c_str());
                updateActionAvailability();
            }
            else if (name == "toolbar-set-disabled-prefix")
            {
                m_customDisabledPrefix = QString::fromUtf8(contents.c_str());
                updateActionAvailability();
            }
        }

        return EventAcceptAndContinue;
    }

    //
    //  SLOTS
    //

    void RvBottomViewToolBar::smActionTriggered(bool b) { m_session->userGenericEvent("mode-manager-toggle-mode", "session_manager"); }

    void RvBottomViewToolBar::paintActionTriggered(bool) { m_session->userGenericEvent("mode-manager-toggle-mode", "annotate_mode"); }

    void RvBottomViewToolBar::infoActionTriggered(bool) { m_session->userGenericEvent("toggle-hud-info-widget", ""); }

    void RvBottomViewToolBar::timelineActionTriggered(bool) { m_session->userGenericEvent("toggle-hud-timeline-widget", ""); }

    void RvBottomViewToolBar::timelineMagActionTriggered(bool) { m_session->userGenericEvent("toggle-hud-timeline-mag-widget", ""); }

    void RvBottomViewToolBar::networkActionTriggered(bool) { RvApp()->showNetworkDialog(); }

    void RvBottomViewToolBar::ghostTriggered(bool isChecked)
    {
        auto* sessionNode = m_session->graph().sessionNode();
        if (sessionNode != nullptr)
        {
            sessionNode->setProperty<IPNode::IntProperty>("paintEffects.ghost", isChecked ? 1 : 0);
            m_session->userGenericEvent("graph-state-change", sessionNode->name() + ".paintEffects.ghost");
        }
    }

    void RvBottomViewToolBar::holdTriggered(bool isChecked)
    {
        auto* sessionNode = m_session->graph().sessionNode();
        if (sessionNode != nullptr)
        {
            sessionNode->setProperty<IPNode::IntProperty>("paintEffects.hold", isChecked ? 1 : 0);
            m_session->userGenericEvent("graph-state-change", sessionNode->name() + ".paintEffects.hold");
        }
    }

    void RvBottomViewToolBar::backStepTriggered() { m_session->userGenericEvent("remote-eval", "extra_commands.stepBackward(1)"); }

    void RvBottomViewToolBar::forwardStepTriggered() { m_session->userGenericEvent("remote-eval", "extra_commands.stepForward(1)"); }

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
                m_session->userGenericEvent("remote-eval", "commands.scrubAudio(false); "
                                                           "commands.setInc(-1); commands.play();");
            else
                m_session->userGenericEvent("remote-eval", "commands.setInc(-1); commands.play();");
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
                m_session->userGenericEvent("remote-eval", "commands.scrubAudio(false); "
                                                           "commands.setInc(1); commands.play();");
            else
                m_session->userGenericEvent("remote-eval", "commands.setInc(1); commands.play();");
        }
    }

    void RvBottomViewToolBar::backMarkTriggered() { m_session->userGenericEvent("remote-eval", "rvui.previousMarkedFrame()"); }

    void RvBottomViewToolBar::forwardMarkTriggered() { m_session->userGenericEvent("remote-eval", "rvui.nextMarkedFrame()"); }

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
        FloatPropertyEditor editor(m_session->graph(), m_session->currentFrame(), "#RVSoundTrack.audio.volume");

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
        IntPropertyEditor editor(m_session->graph(), m_session->currentFrame(), "#RVSoundTrack.audio.mute");

        if (editor.value())
        {
            m_audioAction->setIcon(m_volumeHighMutedIcon);
        }
        else
        {
            int volumeLevel = m_audioSlider->value(); // 0 - 99;

            setVolumeLevel<QAction>(*m_audioAction, volumeLevel);
        }
    }

    void RvBottomViewToolBar::audioMuteTriggered(bool v)
    {
        IntPropertyEditor editor(m_session->graph(), m_session->currentFrame(), "#RVSoundTrack.audio.mute");
        editor.setValue(v ? 1 : 0);

        setVolumeIcon();
    }

    void RvBottomViewToolBar::audioMenuTriggered()
    {
        FloatPropertyEditor veditor(m_session->graph(), m_session->currentFrame(), "#RVSoundTrack.audio.volume");

        IntPropertyEditor meditor(m_session->graph(), m_session->currentFrame(), "#RVSoundTrack.audio.mute");

        m_audioSlider->setValue(veditor.value() * 99.0);

        m_muteAction->setCheckable(true);
        m_muteAction->setChecked(meditor.value() == 1);

        setVolumeIcon();
    }

    void RvBottomViewToolBar::updatePlayModeButtonState()
    {
        if (!m_session)
            return;

        bool loopEnabled = m_session->isEventCategoryEnabled(IPCore::EventCategories::playmodeLoopCategory);
        bool onceEnabled = m_session->isEventCategoryEnabled(IPCore::EventCategories::playmodeOnceCategory);
        bool pingPongEnabled = m_session->isEventCategoryEnabled(IPCore::EventCategories::playmodePingPongCategory);

        m_playModeLoopAction->setEnabled(loopEnabled);
        if (!loopEnabled && !m_customCannotUseTooltip.isEmpty())
        {
            m_playModeLoopAction->setToolTip(m_customCannotUseTooltip);
        }
        else if (!loopEnabled && !m_customDisabledPrefix.isEmpty())
        {
            m_playModeLoopAction->setToolTip(m_customDisabledPrefix + "Loop");
        }
        else
        {
            m_playModeLoopAction->setToolTip("");
        }

        m_playModeOnceAction->setEnabled(onceEnabled);
        if (!onceEnabled && !m_customCannotUseTooltip.isEmpty())
        {
            m_playModeOnceAction->setToolTip(m_customCannotUseTooltip);
        }
        else if (!onceEnabled && !m_customDisabledPrefix.isEmpty())
        {
            m_playModeOnceAction->setToolTip(m_customDisabledPrefix + "Once");
        }
        else
        {
            m_playModeOnceAction->setToolTip("");
        }

        m_playModePingPongAction->setEnabled(pingPongEnabled);
        if (!pingPongEnabled && !m_customCannotUseTooltip.isEmpty())
        {
            m_playModePingPongAction->setToolTip(m_customCannotUseTooltip);
        }
        else if (!pingPongEnabled && !m_customDisabledPrefix.isEmpty())
        {
            m_playModePingPongAction->setToolTip(m_customDisabledPrefix + "PingPong");
        }
        else
        {
            m_playModePingPongAction->setToolTip("");
        }

        // Disable the whole playmode button if all three options are disabled
        bool anyEnabled = loopEnabled || onceEnabled || pingPongEnabled;
        m_playModeAction->setEnabled(anyEnabled);

        QString tooltip;
        if (anyEnabled)
        {
            tooltip = QString::fromUtf8(playModeDefaultTooltip.data(), playModeDefaultTooltip.size());
        }
        else
        {
            if (!m_customCannotUseTooltip.isEmpty())
            {
                tooltip = m_customCannotUseTooltip;
            }
            else if (!m_customDisabledPrefix.isEmpty())
            {
                tooltip = m_customDisabledPrefix + QString::fromUtf8(playModeDefaultTooltip.data(), playModeDefaultTooltip.size());
            }
            else
            {
                tooltip = QString::fromUtf8(playModeDefaultTooltip.data(), playModeDefaultTooltip.size());
            }
        }
        m_playModeAction->setToolTip(tooltip);
    }

    void RvBottomViewToolBar::playModeMenuUpdate()
    {
        if (!m_session)
            return;

        for (int i = 0; i < m_playModeMenu->actions().size(); ++i)
        {
            QAction* playModeMenuAction = m_playModeMenu->actions()[i];
            playModeMenuAction->setCheckable(true);

            if (playModeMenuAction->isEnabled())
            {
                // For enabled items, check if they match the current playmode
                playModeMenuAction->setChecked(playModeMenuAction->data().toMap()["enum"].toInt() == m_session->playMode());
            }
            else
            {
                // For disabled items, always uncheck them.
                playModeMenuAction->setChecked(false);
            }
        }
    }

    void RvBottomViewToolBar::playModeMenuTriggered(QAction* a)
    {
        if (!m_session)
            return;

        m_session->userGenericEvent("remote-eval", UTF8::qconvert(a->data().toMap()["code"].toString()));

        switch (m_session->playMode())
        {
        case Session::PlayOnce:
            m_playModeAction->setIcon(m_playModeOnceIcon);
            break;

        case Session::PlayLoop:
            m_playModeAction->setIcon(m_playModeLoopIcon);
            break;

        case Session::PlayPingPong:
            m_playModeAction->setIcon(m_playModePingPongIcon);
            break;
        }
    }

    bool RvBottomViewToolBar::eventFilter(QObject* obj, QEvent* event)
    {
        if (obj == m_playModeMenu && event->type() == QEvent::MouseMove)
        {
            auto* mouseEvent = dynamic_cast<QMouseEvent*>(event);
            if (mouseEvent)
            {
                QAction* action = m_playModeMenu->actionAt(mouseEvent->pos());

                if (action && !action->isEnabled() && !action->toolTip().isEmpty())
                {
                    QToolTip::showText(mouseEvent->globalPos(), action->toolTip(), m_playModeMenu);
                }
                else
                {
                    QToolTip::hideText();
                }
            }
        }
        return QToolBar::eventFilter(obj, event);
    }

    void RvBottomViewToolBar::resizeEvent(QResizeEvent* event)
    {
        QToolBar::resizeEvent(event);

        if (!m_overflowUpdatePending)
        {
            m_overflowUpdatePending = true;

            QTimer::singleShot(0, this, [this]() {
                m_overflowUpdatePending = false;
                updateOverflow();
            });
        }
    }

    void RvBottomViewToolBar::updateOverflow()
    {
        if (!m_leftBox || !m_centerBox || !m_rightBox)
            return;

        const int centerWidth = m_centerBox->sizeHint().width();
        const int sideBudget = (width() - centerWidth) / 2;

        auto applyOverflow = [&](QWidget* box, bool reverse) {
            QLayout* layout = box->layout();
            if (!layout)
                return;

            box->setUpdatesEnabled(false);

            const QMargins margins = layout->contentsMargins();
            int remaining = sideBudget - margins.left() - margins.right();

            const int n = layout->count();

            for (int idx = 0; idx < n; ++idx)
            {
                const int i = reverse ? (n - 1 - idx) : idx;

                QWidget* w = layout->itemAt(i)->widget();
                if (!w)
                    continue;

                const int hint = w->sizeHint().width();
                const bool fits = hint <= remaining;

                if (w->isVisible() != fits)
                    w->setVisible(fits);

                if (fits)
                    remaining -= hint;
            }

            box->setUpdatesEnabled(true);
            box->updateGeometry();
        };

        applyOverflow(m_leftBox, false);
        applyOverflow(m_rightBox, true);
    }

    void RvBottomViewToolBar::updateActionAvailability()
    {
        if (!m_session)
            return;

        // Update action enabled/disabled state based on event category filtering
        for (const auto& mapping : m_actionCategoryMappings)
        {
            if (mapping.action != nullptr)
            {
                if (mapping.action == m_playModeAction)
                {
                    updatePlayModeButtonState();
                }
                else if (mapping.action == m_backwardPlayAction)
                {
                    bool categoryEnabled = m_session->isEventCategoryEnabled(mapping.category);

                    mapping.action->setEnabled(categoryEnabled);
                    QString tooltip;
                    if (categoryEnabled)
                    {
                        tooltip = mapping.defaultTooltip;
                    }
                    else
                    {
                        if (!m_customCannotUseTooltip.isEmpty())
                        {
                            tooltip = m_customCannotUseTooltip;
                        }
                        else if (!m_customDisabledPrefix.isEmpty())
                        {
                            tooltip = m_customDisabledPrefix + mapping.defaultTooltip;
                        }
                        else
                        {
                            tooltip = mapping.defaultTooltip;
                        }
                    }
                    mapping.action->setToolTip(tooltip);
                }
                else
                {
                    bool categoryEnabled = m_session->isEventCategoryEnabled(mapping.category);

                    mapping.action->setEnabled(categoryEnabled);
                    if (categoryEnabled)
                    {
                        mapping.action->setToolTip(mapping.defaultTooltip);
                    }
                    else
                    {
                        QString tooltip =
                            m_customDisabledPrefix.isEmpty() ? mapping.defaultTooltip : m_customDisabledPrefix + mapping.defaultTooltip;
                        mapping.action->setToolTip(tooltip);
                    }
                }
            }
        }
    }

} // namespace Rv
