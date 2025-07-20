//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <RvCommon/RvTopViewToolBar.h>
#include <RvCommon/QTUtils.h>
#include <RvCommon/RvApplication.h>
#include <RvCommon/DesktopVideoDevice.h>
#include <QtCore/QVariant>
#include <RvPackage/PackageManager.h>
#include <QtWidgets/QMenu>
#include <QtWidgets/QWidgetAction>
#include <QtWidgets/QHBoxLayout>
#include <TwkQtCoreUtil/QtConvert.h>
#include <IPCore/Session.h>
#include <IPCore/ImageRenderer.h>
#include <IPCore/PropertyEditor.h>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <TwkApp/VideoDevice.h>
#include <TwkApp/VideoModule.h>
#include <TwkMath/Chromaticities.h>

namespace Rv
{
    using namespace std;
    using namespace TwkApp;
    using namespace boost;
    using namespace TwkMath;
    using namespace IPCore;
    using namespace TwkContainer;
    using namespace TwkQtCoreUtil;

    RvTopViewToolBar::RvTopViewToolBar(QWidget* parent)
        : QToolBar("top", parent)
        , EventNode("topToolBar")
        , m_loading(false)
        , m_session(0)
        , m_device(0)
        , m_outputDevice(0)
        , m_viewBackAction(0)
    {
        // if we don't build up front the style sheet doesn't quite work
        build();
        setObjectName("topToolBar");
    }

    void RvTopViewToolBar::makeActive(bool b)
    {
        if (b && !m_viewBackAction)
            build();
        setVisible(b);
    }

    void RvTopViewToolBar::makeActiveFromSettings()
    {
        RV_QSETTINGS;
        settings.beginGroup("ViewToolBars");

        //   Policy is visible by default.
        bool b = settings.value("top", true).toBool();

        settings.endGroup();
        makeActive(b);
    }

    void RvTopViewToolBar::build()
    {
        if (m_viewBackAction)
            return;

        QMenu* m;
        QToolButton* b;
        QFrame* qf;
        QAction* a;

        m_iconMap["RVSourceGroup"] =
            colorAdjustedIcon(":/images/videofile_48x48.png");
        m_iconMap["RVImageSource"] =
            colorAdjustedIcon(":/images/videofile_48x48.png");
        m_iconMap["RVSwitchGroup"] =
            colorAdjustedIcon(":/images/shuffle_48x48.png");
        m_iconMap["RVRetimeGroup"] =
            colorAdjustedIcon(":/images/tempo_48x48.png");
        m_iconMap["RVLayoutGroup"] =
            colorAdjustedIcon(":/images/lgicn_48x48.png");
        m_iconMap["RVStackGroup"] =
            colorAdjustedIcon(":/images/photoalbum_48x48.png");
        m_iconMap["RVSequenceGroup"] =
            colorAdjustedIcon(":/images/playlist_48x48.png");
        m_iconMap["RVFolderGroup"] =
            colorAdjustedIcon(":/images/foldr_48x48.png");
        m_iconMap["RVFileSource"] =
            colorAdjustedIcon(":/images/videofile_48x48.png");
        m_iconMap[""] = colorAdjustedIcon(":/images/new_48x48.png");

        setProperty("tbstyle", QVariant(QString("viewer")));

        m_viewBackAction = addAction("");
        b = dynamic_cast<QToolButton*>(widgetForAction(m_viewBackAction));
        b->setIcon(QIcon(":/images/view_back.png"));
        b->setProperty("tbstyle", QVariant(QString("view_menu")));
        b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        b->setToolTip("Switch to previous View");

        m_viewForwardAction = addAction("");
        b = dynamic_cast<QToolButton*>(widgetForAction(m_viewForwardAction));
        b->setIcon(QIcon(":/images/view_forwd.png"));
        b->setProperty("tbstyle", QVariant(QString("view_menu")));
        b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        b->setToolTip("Switch to next View");

        connect(m_viewBackAction, SIGNAL(triggered(bool)), this,
                SLOT(previousViewNode()));
        connect(m_viewForwardAction, SIGNAL(triggered(bool)), this,
                SLOT(nextViewNode()));

        QToolButton* viewButton = new QToolButton(this);
        m_viewMenuAction = new QAction(this);
        m_viewMenuAction->setToolTip("Select a View");
        viewButton->setDefaultAction(m_viewMenuAction);
        viewButton->setProperty("tbstyle", QVariant(QString("view_menu")));
        viewButton->setPopupMode(QToolButton::InstantPopup);
        viewButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

        m_viewMenu = new QMenu(viewButton);
        m_viewMenu->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        m_viewMenu->setProperty("tbstyle", QVariant(QString("view_menu")));
        m_viewMenu->setStyleSheet(
            "QMenu::item:checked { background-color: rgb(20,20,20); }");

        viewButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        viewButton->setMenu(m_viewMenu);
        addWidget(viewButton);

        connect(m_viewMenu, SIGNAL(triggered(QAction*)), this,
                SLOT(viewMenuChanged(QAction*)));
        connect(m_viewMenu, SIGNAL(aboutToShow()), this,
                SLOT(viewMenuUpdate()));

        m_fullScreenAction = addAction("");
        b = dynamic_cast<QToolButton*>(widgetForAction(m_fullScreenAction));
        b->setIcon(QIcon(":/images/fullscreen.png"));
        b->setProperty("tbstyle", QVariant(QString("view_menu")));
        b->setToolButtonStyle(Qt::ToolButtonIconOnly);
        b->setPopupMode(QToolButton::InstantPopup);
        b->setToolTip("Toggle full-screen mode");

        m_frameAction = addAction("");
        b = dynamic_cast<QToolButton*>(widgetForAction(m_frameAction));
        b->setIcon(QIcon(":/images/frame.png"));
        b->setProperty("tbstyle", QVariant(QString("view_menu")));
        b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        b->setPopupMode(QToolButton::DelayedPopup);
        b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        b->setToolTip("Frame image in view");

        m = new QMenu(b);
        a = m->addAction("Frame");
        a->setData(QString("extra_commands.frameImage()"));
        a = m->addAction("Frame Width");
        a->setData(QString("rvui.frameWidth(nil)"));

        b->setMenu(m);

        connect(m, SIGNAL(triggered(QAction*)), this,
                SLOT(frameMenuChanged(QAction*)));
        connect(m_frameAction, SIGNAL(triggered()), this, SLOT(frame()));
        connect(m_fullScreenAction, SIGNAL(triggered()), this,
                SLOT(fullscreen()));

        m_bgMenuAction = addAction("");
        b = dynamic_cast<QToolButton*>(widgetForAction(m_bgMenuAction));
        b->setIcon(QIcon(":/images/checker.png"));
        b->setProperty("tbstyle", QVariant(QString("view_menu")));
        b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        b->setPopupMode(QToolButton::InstantPopup);
        b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        m = new QMenu(b);
        m->addAction("Background")->setDisabled(true);
        m_blackBGAction = m->addAction("  Black");
        m_18GreyBGAction = m->addAction("  18% Grey");
        m_50GreyBGAction = m->addAction("  50% Grey");
        m_whiteBGAction = m->addAction("  White");
        m_checkerBGAction = m->addAction("  Checker");
        m_crossHatchBGAction = m->addAction("  Cross Hatch");
        m->setStyleSheet(
            "QMenu::item:checked { background-color: rgb(20,20,20); }");
        m_blackBGAction->setCheckable(true);
        m_18GreyBGAction->setCheckable(true);
        m_50GreyBGAction->setCheckable(true);
        m_whiteBGAction->setCheckable(true);
        m_checkerBGAction->setCheckable(true);
        m_crossHatchBGAction->setCheckable(true);
        m_blackBGAction->setIcon(QIcon(":/images/black.png"));
        m_18GreyBGAction->setIcon(QIcon(":/images/grey18.png"));
        m_50GreyBGAction->setIcon(QIcon(":/images/grey50.png"));
        m_whiteBGAction->setIcon(QIcon(":/images/white.png"));
        m_checkerBGAction->setIcon(QIcon(":/images/checker.png"));
        m_crossHatchBGAction->setIcon(QIcon(":/images/cross_hatch.png"));
        b->setMenu(m);
        m_bgMenuAction->setToolTip("Select background style");

        connect(m, SIGNAL(aboutToShow()), this, SLOT(bgMenuUpdate()));
        connect(m_blackBGAction, SIGNAL(triggered()), this, SLOT(bgBlack()));
        connect(m_18GreyBGAction, SIGNAL(triggered()), this, SLOT(bg18()));
        connect(m_50GreyBGAction, SIGNAL(triggered()), this, SLOT(bg50()));
        connect(m_whiteBGAction, SIGNAL(triggered()), this, SLOT(bgWhite()));
        connect(m_checkerBGAction, SIGNAL(triggered()), this,
                SLOT(bgChecker()));
        connect(m_crossHatchBGAction, SIGNAL(triggered()), this,
                SLOT(bgHatch()));

        m_stereoMenuAction = addAction("Mono");
        b = dynamic_cast<QToolButton*>(widgetForAction(m_stereoMenuAction));
        b->setProperty("tbstyle", QVariant(QString("view_menu")));
        b->setIcon(QIcon(":/images/stereo.png"));
        b->setPopupMode(QToolButton::InstantPopup);
        b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        m = new QMenu(b);
        m->addAction("Stereo")->setDisabled(true);
        m_monoStereoAction = m->addAction("  Mono");
        m_anaglyphStereoAction = m->addAction("  Anaglyph");
        m_lumanaglyphStereoAction = m->addAction("  Luminance Anaglyph");
        m_sideBySideStereoAction = m->addAction("  Side by Side");
        m_mirroredSideBySideStereoAction = m->addAction("  Mirrored");
        m_checkerStereoAction = m->addAction("  DLP Checker");
        m_scanlineStereoAction = m->addAction("  Scanline");
        m_leftStereoAction = m->addAction("  Left Only");
        m_rightStereoAction = m->addAction("  Right Only");
        // m_shutterStereoAction = m->addAction("  Shutter");
        m_monoStereoAction->setCheckable(true);
        m_anaglyphStereoAction->setCheckable(true);
        m_lumanaglyphStereoAction->setCheckable(true);
        m_sideBySideStereoAction->setCheckable(true);
        m_mirroredSideBySideStereoAction->setCheckable(true);
        m_checkerStereoAction->setCheckable(true);
        m_scanlineStereoAction->setCheckable(true);
        m_leftStereoAction->setCheckable(true);
        m_rightStereoAction->setCheckable(true);
        // m_shutterStereoAction->setCheckable(true);
        m_monoStereoAction->setData(QString("off"));
        m_anaglyphStereoAction->setData(QString("anaglyph"));
        m_lumanaglyphStereoAction->setData(QString("lumanaglyph"));
        m_sideBySideStereoAction->setData(QString("pair"));
        m_mirroredSideBySideStereoAction->setData(QString("mirror"));
        m_checkerStereoAction->setData(QString("checker"));
        m_scanlineStereoAction->setData(QString("scanline"));
        m_leftStereoAction->setData(QString("left"));
        m_rightStereoAction->setData(QString("right"));
        //        m_shutterStereoAction->setData(QString("hardware"));
        m->addSeparator();
        m_swapStereoAction = m->addAction("Swap Eyes");
        m_swapStereoAction->setCheckable(true);
        b->setMenu(m);
        stereoMenuUpdate();
        m_stereoMenuAction->setToolTip("Select stereoscopic output style");

        m_stereoActions.push_back(m_monoStereoAction);
        m_stereoActions.push_back(m_anaglyphStereoAction);
        m_stereoActions.push_back(m_lumanaglyphStereoAction);
        m_stereoActions.push_back(m_sideBySideStereoAction);
        m_stereoActions.push_back(m_mirroredSideBySideStereoAction);
        m_stereoActions.push_back(m_checkerStereoAction);
        m_stereoActions.push_back(m_scanlineStereoAction);
        m_stereoActions.push_back(m_leftStereoAction);
        m_stereoActions.push_back(m_rightStereoAction);
        // m_stereoActions.push_back(m_shutterStereoAction);

        connect(m, SIGNAL(aboutToShow()), this, SLOT(stereoMenuUpdate()));
        connect(m_monoStereoAction, SIGNAL(triggered()), this,
                SLOT(monoStereo()));
        connect(m_anaglyphStereoAction, SIGNAL(triggered()), this,
                SLOT(anaglyphStereo()));
        connect(m_lumanaglyphStereoAction, SIGNAL(triggered()), this,
                SLOT(lumanaglyphStereo()));
        connect(m_sideBySideStereoAction, SIGNAL(triggered()), this,
                SLOT(sideBySideStereo()));
        connect(m_mirroredSideBySideStereoAction, SIGNAL(triggered()), this,
                SLOT(mirroredStereo()));
        connect(m_checkerStereoAction, SIGNAL(triggered()), this,
                SLOT(checkerStereo()));
        connect(m_scanlineStereoAction, SIGNAL(triggered()), this,
                SLOT(scanlineStereo()));
        connect(m_leftStereoAction, SIGNAL(triggered()), this,
                SLOT(leftStereo()));
        connect(m_rightStereoAction, SIGNAL(triggered()), this,
                SLOT(rightStereo()));
        // connect(m_shutterStereoAction, SIGNAL(triggered()), this,
        //         SLOT(shutterStereo()));
        connect(m_swapStereoAction, SIGNAL(triggered()), this,
                SLOT(swapEyes()));

        m_channelMenuAction = addAction("Color (RGB)");
        b = dynamic_cast<QToolButton*>(widgetForAction(m_channelMenuAction));
        b->setProperty("tbstyle", QVariant(QString("view_menu")));
        b->setIcon(QIcon(":/images/view_channel.png"));
        b->setPopupMode(QToolButton::InstantPopup);
        b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        m = new QMenu(b);
        m->addAction("Channels")->setDisabled(true);
        m_RGBChannelAction = m->addAction("  Color (RGB)");
        m_RChannelAction = m->addAction("  Red");
        m_GChannelAction = m->addAction("  Green");
        m_BChannelAction = m->addAction("  Blue");
        m_AChannelAction = m->addAction("  Alpha");
        m_LChannelAction = m->addAction("  Luminance");
        b->setMenu(m);
        m_channelMenuAction->setToolTip("Color channel view control");

        m_RGBChannelAction->setCheckable(true);
        m_RChannelAction->setCheckable(true);
        m_GChannelAction->setCheckable(true);
        m_BChannelAction->setCheckable(true);
        m_AChannelAction->setCheckable(true);
        m_LChannelAction->setCheckable(true);

        connect(m, SIGNAL(aboutToShow()), this, SLOT(channelMenuUpdate()));
        connect(m_RGBChannelAction, SIGNAL(triggered()), this,
                SLOT(channelsRGB()));
        connect(m_RChannelAction, SIGNAL(triggered()), this, SLOT(channelsR()));
        connect(m_GChannelAction, SIGNAL(triggered()), this, SLOT(channelsG()));
        connect(m_BChannelAction, SIGNAL(triggered()), this, SLOT(channelsB()));
        connect(m_AChannelAction, SIGNAL(triggered()), this, SLOT(channelsA()));
        connect(m_LChannelAction, SIGNAL(triggered()), this, SLOT(channelsL()));

        m_monitorMenuAction = addAction("NO MONITOR");
        b = dynamic_cast<QToolButton*>(widgetForAction(m_monitorMenuAction));
        b->setProperty("tbstyle", QVariant(QString("view_menu")));
        b->setIcon(QIcon(":/images/view_display.png"));
        b->setPopupMode(QToolButton::InstantPopup);
        b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        m = new QMenu(b);
        m_transferTitleAction = m->addAction("Transfer Function");
        m_transferTitleAction->setDisabled(true);
        m_noTransferAction = m->addAction("  None");
        m_srgbAction = m->addAction("  sRGB");
        m_rec709Action = m->addAction("  Rec.709");
        m_g22Action = m->addAction("  Gamma 2.2");
        m_g24Action = m->addAction("  Gamma 2.4");
        m_g26Action = m->addAction("  Gamma 2.6");
        m_gOtherAction = m->addAction("  Other Gamma");
        m->addSeparator();
        m_primariesTitleAction = m->addAction("Primaries");
        m_primariesTitleAction->setDisabled(true);
        m_primaries709Action = m->addAction("  Rec.709");
        m_primariesP3Action = m->addAction("  P3");
        m_primariesSMPTECAction = m->addAction("  SMPTE-C");
        m_primariesXYZAction = m->addAction("  CIE XYZ");
        m_primariesAdobeRGBAction = m->addAction("  Adobe RGB");
        m_primariesDreamColorAction = m->addAction("  DreamColor Full Gamut");
        m->addSeparator();
        m_ditherTitleAction = m->addAction("Dither");
        m_ditherTitleAction->setDisabled(true);
        m_ditherOff = m->addAction("  Off");
        m_dither8 = m->addAction("  8 Bit");
        m_dither10 = m->addAction("  10 Bit");
        m->addSeparator();
        QWidgetAction* wa = new QWidgetAction(m);
        QWidget* base = new QWidget(b);
        QHBoxLayout* layout = new QHBoxLayout(base);
        m_monitorInfoLabel = new QLabel(b);
        m_monitorInfoLabel->setEnabled(false);
        m_monitorInfoLabel->setTextFormat(Qt::RichText);
        layout->addWidget(m_monitorInfoLabel);
        wa->setDefaultWidget(base);
        m->addAction(wa);
        b->setMenu(m);
        m_monitorMenu = m;
        m_monitorMenuAction->setToolTip("Configure display device");

        m_noTransferAction->setCheckable(true);
        m_srgbAction->setCheckable(true);
        m_rec709Action->setCheckable(true);
        m_g22Action->setCheckable(true);
        m_g24Action->setCheckable(true);
        m_g26Action->setCheckable(true);
        m_gOtherAction->setCheckable(true);
        m_primaries709Action->setCheckable(true);
        m_primariesP3Action->setCheckable(true);
        m_primariesAdobeRGBAction->setCheckable(true);
        m_primariesXYZAction->setCheckable(true);
        m_primariesSMPTECAction->setCheckable(true);
        m_primariesDreamColorAction->setCheckable(true);
        m_ditherOff->setCheckable(true);
        m_dither8->setCheckable(true);
        m_dither10->setCheckable(true);

        connect(m_monitorMenu, SIGNAL(aboutToShow()), this,
                SLOT(monitorMenuUpdate()));
        connect(m_noTransferAction, SIGNAL(triggered()), this,
                SLOT(transferNone()));
        connect(m_srgbAction, SIGNAL(triggered()), this, SLOT(transferSRGB()));
        connect(m_rec709Action, SIGNAL(triggered()), this,
                SLOT(transferRec709()));
        connect(m_g22Action, SIGNAL(triggered()), this, SLOT(transfer22()));
        connect(m_g24Action, SIGNAL(triggered()), this, SLOT(transfer24()));
        connect(m_g26Action, SIGNAL(triggered()), this, SLOT(transfer26()));
        connect(m_primaries709Action, SIGNAL(triggered()), this,
                SLOT(primaries709()));
        connect(m_primariesP3Action, SIGNAL(triggered()), this,
                SLOT(primariesP3()));
        connect(m_primariesSMPTECAction, SIGNAL(triggered()), this,
                SLOT(primariesSMPTEC()));
        connect(m_primariesXYZAction, SIGNAL(triggered()), this,
                SLOT(primariesXYZ()));
        connect(m_primariesAdobeRGBAction, SIGNAL(triggered()), this,
                SLOT(primariesAdobeRGB()));
        connect(m_primariesDreamColorAction, SIGNAL(triggered()), this,
                SLOT(primariesDreamColor()));
        connect(m_ditherOff, SIGNAL(triggered()), this, SLOT(ditherOff()));
        connect(m_dither8, SIGNAL(triggered()), this, SLOT(dither8()));
        connect(m_dither10, SIGNAL(triggered()), this, SLOT(dither10()));

        if (m_session)
            setSession(m_session);
    }

    RvTopViewToolBar::~RvTopViewToolBar() {}

    void RvTopViewToolBar::setSession(Session* s)
    {
        m_session = s;

        if (m_viewBackAction)
        {
            listenTo(s);
            bgMenuUpdate();
            stereoMenuUpdate();
            channelMenuUpdate();
        }
    }

    void RvTopViewToolBar::updateActionToolButton(QAction* action,
                                                  const string& text,
                                                  const string& icon,
                                                  bool forceRepaint)
    {
        action->setText(text.c_str());
        QToolButton* b = dynamic_cast<QToolButton*>(widgetForAction(action));

        if (icon[0] == ':')
        {
            b->setIcon(QIcon(icon.c_str()));
        }
        else
        {
            QString iconName = ":/images/%1";
            b->setIcon(QIcon(iconName.arg(icon.c_str())));
        }
        int w = b->sizeHint().width();
        if (w > b->minimumWidth())
            b->setMinimumWidth(w);

        if (forceRepaint)
            repaint();
        else
            update();
    }

    EventNode::Result RvTopViewToolBar::receiveEvent(const Event& event)
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

                if (parts[2] == "type" && parts[1] == "stereo"
                    && node->protocol() == "RVDisplayStereo")
                {
                    stereoMenuUpdate();
                }
                else if (node->protocol() == "RVDisplayColor" || node->protocol() == "OCIODisplay" )
                {
                    if (parts[1] == "color")
                    {
                        if (parts[2] == "channelFlood")
                        {
                            channelMenuUpdate();
                        }
                        else if (parts[2] == "sRGB" || parts[2] == "Rec709"
                                 || parts[2] == "gamma")
                        {
                            // monitorMenuUpdate();
                        }
                    }
                }
            }
            else if (name == "after-graph-view-change")
            {
                viewMenuUpdate();
            }
            else if (name == "output-video-device-changed")
            {
                m_outputDevice = 0;

                if (contents != "")
                {
                    string::size_type p = contents.find('/');

                    if (p != string::npos)
                    {
                        const string deviceName = contents.substr(p + 1);
                        const string moduleName = contents.substr(0, p);

                        for (const auto module : RvApp()->videoModules())
                        {
                            if (module->name() == moduleName)
                            {
                                for (const auto device : module->devices())
                                {
                                    if (device->name() == deviceName)
                                    {
                                        m_outputDevice = device;
                                        updateActionToolButton(
                                            m_monitorMenuAction, deviceName,
                                            "view_display.png");
                                        return EventAcceptAndContinue;
                                    }
                                }
                            }
                        }
                    }
                }

                if (m_device || m_outputDevice)
                {
                    string t = m_outputDevice ? m_outputDevice->name()
                                              : m_device->name();
                    updateActionToolButton(m_monitorMenuAction, t,
                                           "view_display.png");
                }
            }
            else if (name == "bg-pattern-change")
            {
                bgMenuUpdate();
            }
        }
        else if (const VideoDeviceContextChangeEvent* vevent =
                     dynamic_cast<const VideoDeviceContextChangeEvent*>(&event))
        {
            m_device = vevent->m_physicalDevice;

            if (!m_outputDevice)
            {
                updateActionToolButton(m_monitorMenuAction, m_device->name(),
                                       "view_display.png");
            }

            monitorMenuUpdate();
            channelMenuUpdate();
        }

        return EventAcceptAndContinue;
    }

    void RvTopViewToolBar::nextViewNode()
    {
        string name = m_session->nextViewNode();

        if (!name.empty())
            m_session->setViewNode(name);
    }

    void RvTopViewToolBar::previousViewNode()
    {
        string name = m_session->previousViewNode();

        if (!name.empty())
            m_session->setViewNode(name);
    }

    bool RvTopViewToolBar::isInPresentationMode() const
    {
        RvApplication* rvApp = Rv::RvApp();
        if (rvApp)
        {
            return rvApp->isInPresentationMode();
        }
        return false;
    }

    void RvTopViewToolBar::frame()
    {
        m_session->userGenericEvent("remote-eval",
                                    "extra_commands.frameImage()");
    }

    void RvTopViewToolBar::fullscreen()
    {
        m_session->userGenericEvent("remote-eval",
                                    "extra_commands.toggleFullScreen()");
    }

    void RvTopViewToolBar::viewMenuChanged(QAction* action)
    {
        if (!m_session)
            return;
        string name = UTF8::qconvert(action->data().toString());
        m_session->setViewNode(name);
    }

    void RvTopViewToolBar::frameMenuChanged(QAction* action)
    {
        if (!m_session)
            return;
        m_session->userGenericEvent("remote-eval",
                                    UTF8::qconvert(action->data().toString()));
    }

    void RvTopViewToolBar::viewMenuUpdate()
    {
        if (!m_session)
            return;

        m_viewMenu->clear();
        IPNode* vnode = m_session->graph().viewNode();

        const IPGraph::NodeMap& nodes = m_session->graph().viewableNodes();

        for (IPGraph::NodeMap::const_iterator i = nodes.begin();
             i != nodes.end(); ++i)
        {
            IPNode* node = (*i).second;
            string pname = node->protocol();
            string uiName = node->uiName();
            string nodeName = node->name();
            QString p = pname.c_str();
            QIcon icon = m_iconMap.contains(p) ? m_iconMap[p] : m_iconMap[""];
            QAction* action =
                m_viewMenu->addAction(icon, UTF8::qconvert(uiName));

            action->setData(UTF8::qconvert(nodeName));
            action->setCheckable(true);
            action->setChecked(node == vnode);

            if (node == vnode)
            {
                m_viewMenuAction->setIcon(icon);
                m_viewMenuAction->setText(UTF8::qconvert(uiName)
                                          + QString(" "));
                update();
            }
        }
    }

    void RvTopViewToolBar::bgMenuUpdate()
    {
        try
        {
            m_blackBGAction->setEnabled(true);
            m_18GreyBGAction->setEnabled(true);
            m_50GreyBGAction->setEnabled(true);
            m_whiteBGAction->setEnabled(true);
            m_checkerBGAction->setEnabled(true);
            m_crossHatchBGAction->setEnabled(true);

            bgMenuUpdate2();
        }
        catch (...)
        {
            m_blackBGAction->setEnabled(false);
            m_18GreyBGAction->setEnabled(false);
            m_50GreyBGAction->setEnabled(false);
            m_whiteBGAction->setEnabled(false);
            m_checkerBGAction->setEnabled(false);
            m_crossHatchBGAction->setEnabled(false);
        }
    }

    void RvTopViewToolBar::bgMenuUpdate2()
    {
        ImageRenderer::BGPattern bg = m_session->renderer()->bgPattern();

        m_blackBGAction->setChecked(bg == ImageRenderer::Solid0);
        m_18GreyBGAction->setChecked(bg == ImageRenderer::Solid18);
        m_50GreyBGAction->setChecked(bg == ImageRenderer::Solid50);
        m_whiteBGAction->setChecked(bg == ImageRenderer::Solid100);
        m_checkerBGAction->setChecked(bg == ImageRenderer::Checker);
        m_crossHatchBGAction->setChecked(bg == ImageRenderer::CrossHatch);

        const char* iconName = 0;

        switch (bg)
        {
        default:
        case ImageRenderer::Solid0:
            iconName = ":/images/black.png";
            break;
        case ImageRenderer::Solid18:
            iconName = ":/images/grey18.png";
            break;
        case ImageRenderer::Solid50:
            iconName = ":/images/grey50.png";
            break;
        case ImageRenderer::Solid100:
            iconName = ":/images/white.png";
            break;
        case ImageRenderer::Checker:
            iconName = ":/images/checker.png";
            break;
        case ImageRenderer::CrossHatch:
            iconName = ":/images/cross_hatch.png";
            break;
        }

        m_bgMenuAction->setIcon(QIcon(iconName));
    }

    void RvTopViewToolBar::bgBlack()
    {
        m_session->renderer()->setBGPattern(ImageRenderer::Solid0);
        m_bgMenuAction->setIcon(QIcon(":/images/black.png"));
    }

    void RvTopViewToolBar::bg18()
    {
        m_session->renderer()->setBGPattern(ImageRenderer::Solid18);
        m_bgMenuAction->setIcon(QIcon(":/images/grey18.png"));
    }

    void RvTopViewToolBar::bg50()
    {
        m_session->renderer()->setBGPattern(ImageRenderer::Solid50);
        m_bgMenuAction->setIcon(QIcon(":/images/grey50.png"));
    }

    void RvTopViewToolBar::bgWhite()
    {
        m_session->renderer()->setBGPattern(ImageRenderer::Solid100);
        m_bgMenuAction->setIcon(QIcon(":/images/white.png"));
    }

    void RvTopViewToolBar::bgChecker()
    {
        m_session->renderer()->setBGPattern(ImageRenderer::Checker);
        m_bgMenuAction->setIcon(QIcon(":/images/checker.png"));
    }

    void RvTopViewToolBar::bgHatch()
    {
        m_session->renderer()->setBGPattern(ImageRenderer::CrossHatch);
        m_bgMenuAction->setIcon(QIcon(":/images/cross_hatch.png"));
    }

    void RvTopViewToolBar::stereoMenuUpdate()
    {
        m_stereoMenuAction->setText(
            "DLP  CHECKER"); // just a measurement string
        QToolButton* b =
            dynamic_cast<QToolButton*>(widgetForAction(m_stereoMenuAction));
        b->setIcon(QIcon(":/images/stereo.png"));
        int w = b->sizeHint().width();
        b->setMinimumWidth(w);

        if (!m_session)
        {
            m_stereoMenuAction->setText("Mono");
            QToolButton* b =
                dynamic_cast<QToolButton*>(widgetForAction(m_stereoMenuAction));
            b->setIcon(QIcon(":/images/stereo.png"));
            return;
        }

        StringPropertyEditor editor(m_session->graph(),
                                    m_session->currentFrame(),
                                    "@RVDisplayStereo.stereo.type");

        string type = editor.value();
        if (type == "mono")
            type = "off";
        QString t = type.c_str();

        for (size_t i = 0; i < m_stereoActions.size(); i++)
        {
            QAction* a = m_stereoActions[i];
            bool checked = a->data().toString() == t;
            a->setChecked(checked);

            if (checked)
            {
                m_stereoMenuAction->setText(a->text());
                QToolButton* b = dynamic_cast<QToolButton*>(
                    widgetForAction(m_stereoMenuAction));
                b->setIcon(QIcon(":/images/stereo.png"));
            }
        }

        if ((type != "off") || isInPresentationMode())
        {
            // Update the menu entry for Global Swap Eyes
            // according to the property value.
            IntPropertyEditor globalSwapEyes(m_session->graph(),
                                             m_session->currentFrame(),
                                             "@RVDisplayStereo.stereo.swap");
            m_swapStereoAction->setEnabled(true);
            m_swapStereoAction->setChecked(globalSwapEyes.value() ? true
                                                                  : false);
        }
        else
        {
            // If not in stereo/presentation mode gray out/disable the Global
            // Swap entry.
            m_swapStereoAction->setEnabled(false);
        }
    }

    void RvTopViewToolBar::setStereo(const string& type)
    {
        Session::CachingMode m = m_session->cachingMode();

        StringPropertyEditor editor(m_session->graph(),
                                    m_session->currentFrame(),
                                    "@RVDisplayStereo.stereo.type");

        string oldType = editor.value();
        bool kickCache = (oldType == "off" && type != "off")
                         || (oldType != "off" && type == "off");

        if (kickCache)
            m_session->setCaching(Session::NeverCache);

        editor.setValue(type);

        if (oldType != "hardware" && type == "hardware")
        {
            m_session->send(Session::stereoHardwareOnMessage());
        }
        else if (oldType == "hardware" && type != "hardware")
        {
            m_session->send(Session::stereoHardwareOffMessage());
        }

        if (kickCache)
            m_session->setCaching(m);
    }

    void RvTopViewToolBar::setChannel(int c)
    {
        bool ocio = hasOCIODisplayPipeline();
        IntPropertyEditor editor(m_session->graph(), m_session->currentFrame(),
                                 ocio ? "#OCIODisplay.color.channelFlood" : "#RVDisplayColor.color.channelFlood");

        editor.setValue(c);
    }

    void RvTopViewToolBar::monoStereo() { setStereo("off"); }

    void RvTopViewToolBar::anaglyphStereo() { setStereo("anaglyph"); }

    void RvTopViewToolBar::lumanaglyphStereo() { setStereo("lumanaglyph"); }

    void RvTopViewToolBar::sideBySideStereo() { setStereo("pair"); }

    void RvTopViewToolBar::mirroredStereo() { setStereo("mirror"); }

    void RvTopViewToolBar::checkerStereo() { setStereo("checker"); }

    void RvTopViewToolBar::scanlineStereo() { setStereo("scanline"); }

    void RvTopViewToolBar::leftStereo() { setStereo("left"); }

    void RvTopViewToolBar::rightStereo() { setStereo("right"); }

    // void RvTopViewToolBar::shutterStereo() { setStereo("hardware"); }

    void RvTopViewToolBar::swapEyes()
    {
        IntPropertyEditor editor(m_session->graph(), m_session->currentFrame(),
                                 "@RVDisplayStereo.stereo.swap");
        editor.setValue(editor.value() == 0);
    }

    void RvTopViewToolBar::channelMenuUpdate()
    {
        m_channelMenuAction->setText("MMMMMMMM"); // just a measurement string
        QToolButton* b =
            dynamic_cast<QToolButton*>(widgetForAction(m_channelMenuAction));
        b->setIcon(QIcon(":/images/view_channel.png"));
        int w = b->sizeHint().width();
        b->setMinimumWidth(w);

        if (!m_session)
            return;

        bool hasDisplay = hasStandardDisplayPipeline();
        bool hasOCIODisplay = hasOCIODisplayPipeline();

        if (hasDisplay || hasOCIODisplay)
        {

            IntPropertyEditor editor(m_session->graph(),
                                     m_session->currentFrame(),
                                     hasOCIODisplay ? "#OCIODisplay.color.channelFlood" : "#RVDisplayColor.color.channelFlood");

            m_RGBChannelAction->setVisible(true);
            m_RChannelAction->setVisible(true);
            m_GChannelAction->setVisible(true);
            m_BChannelAction->setVisible(true);
            m_AChannelAction->setVisible(true);
            m_LChannelAction->setVisible(true);

            m_RGBChannelAction->setEnabled(true);
            m_RChannelAction->setEnabled(true);
            m_GChannelAction->setEnabled(true);
            m_BChannelAction->setEnabled(true);
            m_AChannelAction->setEnabled(true);
            m_LChannelAction->setEnabled(true);

            m_RGBChannelAction->setChecked(false);
            m_RChannelAction->setChecked(false);
            m_GChannelAction->setChecked(false);
            m_BChannelAction->setChecked(false);
            m_AChannelAction->setChecked(false);
            m_LChannelAction->setChecked(false);

            QAction* a = 0;

            switch (editor.value())
            {
            default:
            case 0:
                a = m_RGBChannelAction;
                break;
            case 1:
                a = m_RChannelAction;
                break;
            case 2:
                a = m_GChannelAction;
                break;
            case 3:
                a = m_BChannelAction;
                break;
            case 4:
                a = m_AChannelAction;
                break;
            case 5:
                a = m_LChannelAction;
                break;
            }

            a->setChecked(true);

            m_channelMenuAction->setText(a->text());
            b = dynamic_cast<QToolButton*>(
                widgetForAction(m_channelMenuAction));
            b->setIcon(QIcon(":/images/view_channel.png"));
        }
        else
        {
            m_RGBChannelAction->setEnabled(false);
            m_RChannelAction->setEnabled(false);
            m_GChannelAction->setEnabled(false);
            m_BChannelAction->setEnabled(false);
            m_AChannelAction->setEnabled(false);
            m_LChannelAction->setEnabled(false);

            m_RGBChannelAction->setVisible(false);
            m_RChannelAction->setVisible(false);
            m_GChannelAction->setVisible(false);
            m_BChannelAction->setVisible(false);
            m_AChannelAction->setVisible(false);
            m_LChannelAction->setVisible(false);

            m_channelMenuAction->setText(m_RGBChannelAction->text());
            b = dynamic_cast<QToolButton*>(
                widgetForAction(m_channelMenuAction));
            b->setIcon(QIcon(":/images/view_channel.png"));
        }
    }

    void RvTopViewToolBar::channelsRGB() { setChannel(0); }

    void RvTopViewToolBar::channelsR() { setChannel(1); }

    void RvTopViewToolBar::channelsG() { setChannel(2); }

    void RvTopViewToolBar::channelsB() { setChannel(3); }

    void RvTopViewToolBar::channelsA() { setChannel(4); }

    void RvTopViewToolBar::channelsL() { setChannel(5); }

    bool RvTopViewToolBar::hasStandardDisplayPipeline()
    {
        if (!m_session)
            return false;

        StringPropertyEditor displayPipeline(
            m_session->graph(), m_session->currentFrame(),
            "@RVDisplayPipelineGroup.pipeline.nodes");

        const StringPropertyEditor::ContainerType& pipelineNodes =
            displayPipeline.container();
        bool hasDisplay = false;

        for (size_t i = 0; i < pipelineNodes.size(); i++)
        {
            if (pipelineNodes[i] == "RVDisplayColor")
                return true;
        }

        return false;
    }

    bool RvTopViewToolBar::hasOCIODisplayPipeline()
    {
        if (!m_session)
        return false;

        StringPropertyEditor displayPipeline(
            m_session->graph(), m_session->currentFrame(),
            "@RVDisplayPipelineGroup.pipeline.nodes");

        const StringPropertyEditor::ContainerType& pipelineNodes =
            displayPipeline.container();
        bool hasDisplay = false;

        for (size_t i = 0; i < pipelineNodes.size(); i++)
        {
            if (pipelineNodes[i] == "OCIODisplay")
                return true;
        }

        return false;
    }

    void RvTopViewToolBar::monitorMenuUpdate()
    {
        updateActionToolButton(m_monitorMenuAction, "MMMMMMMMMMMMM",
                               "view_display.png");
        if (!m_device)
            return;

        string name =
            m_outputDevice ? m_outputDevice->name() : m_device->name();
        updateActionToolButton(m_monitorMenuAction, name, "view_display.png");

        bool hasDisplay = hasStandardDisplayPipeline();
        bool hasOCIODisplay = hasOCIODisplayPipeline();

        try
        {

            m_noTransferAction->setEnabled(hasDisplay);
            m_srgbAction->setEnabled(hasDisplay);
            m_rec709Action->setEnabled(hasDisplay);
            m_g22Action->setEnabled(hasDisplay);
            m_g24Action->setEnabled(hasDisplay);
            m_g26Action->setEnabled(hasDisplay);
            m_gOtherAction->setEnabled(hasDisplay);
            m_primaries709Action->setEnabled(hasDisplay);
            m_primariesP3Action->setEnabled(hasDisplay);
            m_primariesSMPTECAction->setEnabled(hasDisplay);
            m_primariesXYZAction->setEnabled(hasDisplay);
            m_primariesAdobeRGBAction->setEnabled(hasDisplay);
            m_primariesDreamColorAction->setEnabled(hasDisplay);
            m_ditherOff->setEnabled(hasDisplay || hasOCIODisplay);
            m_dither8->setEnabled(hasDisplay || hasOCIODisplay);
            m_dither10->setEnabled(hasDisplay || hasOCIODisplay);

            m_transferTitleAction->setVisible(hasDisplay);
            m_primariesTitleAction->setVisible(hasDisplay);
            m_ditherTitleAction->setVisible(hasDisplay || hasOCIODisplay);

            m_noTransferAction->setVisible(hasDisplay);
            m_srgbAction->setVisible(hasDisplay);
            m_rec709Action->setVisible(hasDisplay);
            m_g22Action->setVisible(hasDisplay);
            m_g24Action->setVisible(hasDisplay);
            m_g26Action->setVisible(hasDisplay);
            m_gOtherAction->setVisible(hasDisplay);
            m_primaries709Action->setVisible(hasDisplay);
            m_primariesP3Action->setVisible(hasDisplay);
            m_primariesSMPTECAction->setVisible(hasDisplay);
            m_primariesXYZAction->setVisible(hasDisplay);
            m_primariesAdobeRGBAction->setVisible(hasDisplay);
            m_primariesDreamColorAction->setVisible(hasDisplay);
            m_ditherOff->setVisible(hasDisplay || hasOCIODisplay);
            m_dither8->setVisible(hasDisplay || hasOCIODisplay);
            m_dither10->setVisible(hasDisplay || hasOCIODisplay);

            if (hasDisplay || hasOCIODisplay)
            {
                try
                {
                    monitorMenuRVUpdate();
                }
                catch (...)
                {
                }
            }
            else
            {
            }
        }
        catch (...)
        {
        }

        //
        //  Display Info
        //

        QString html =
            "<style type=\"text/css\"> td { padding: 4px; } </style><table>";

        for (const auto module : RvApp()->videoModules())
        {
            for (const auto device : module->devices())
            {
                VideoDevice::VideoFormat format =
                    device->videoFormatAtIndex(device->currentVideoFormat());
                VideoDevice::DataFormat data =
                    device->dataFormatAtIndex(device->currentDataFormat());

                QString icon;

                if (dynamic_cast<const DesktopVideoDevice*>(device))
                {
                    icon = "<img src=\":/images/view_display_flat.png\" "
                           "width=24 height=24>";
                }
                else
                {
                    icon = " ";
                }

                string mname = module->name(); // windows
                string dname = device->name();
                QString name;

                if (mname != "Desktop")
                {
                    name += mname.c_str();
                    name += " / ";
                }

                name += dname.c_str();

                QString mon;

                if (format.width > 0 && format.height > 0)
                {
                    mon = QString("<tr>"
                                  "<td align=center valign=middle>%6</td>"
                                  "<td><strong>%1</strong><br><small>%2 x %3 @ "
                                  "%4Hz<br>%5</small>")
                              .arg(name)
                              .arg(format.width)
                              .arg(format.height)
                              .arg(format.hz)
                              .arg(data.description.c_str())
                              .arg(icon);
                }
                else
                {
                    mon = QString("<tr>"
                                  "<td align=center valign=middle>%1</td>"
                                  "<td><strong>%2</strong>")
                              .arg(icon)
                              .arg(name);
                }

                mon += "</td> </td>";

                html += mon;
            }
        }

        html += "</table>";
        m_monitorInfoLabel->setText(html);
        m_monitorInfoLabel->setTextFormat(Qt::RichText);

        m_monitorMenu->update();
    }

    void RvTopViewToolBar::monitorMenuOCIOUpdate() {}

    void RvTopViewToolBar::monitorMenuRVUpdate()
    {

        bool ocio = hasOCIODisplayPipeline();
        if (ocio)
        {
            IntPropertyEditor dither(m_session->graph(), m_session->currentFrame(),
            "@OCIODisplay.color.dither");

            m_ditherOff->setChecked(dither.value() == 0);
            m_dither8->setChecked(dither.value() == 8);
            m_dither10->setChecked(dither.value() == 10);

            return;
        }

        IntPropertyEditor srgb(m_session->graph(), m_session->currentFrame(),
                               "@RVDisplayColor.color.sRGB");

        IntPropertyEditor rec709(m_session->graph(), m_session->currentFrame(),
                                 "@RVDisplayColor.color.Rec709");

        FloatPropertyEditor gamma(m_session->graph(), m_session->currentFrame(),
                                  "@RVDisplayColor.color.gamma");

        Vec2fPropertyEditor white(m_session->graph(), m_session->currentFrame(),
                                  "@RVDisplayColor.chromaticities.white");

        Vec2fPropertyEditor red(m_session->graph(), m_session->currentFrame(),
                                "@RVDisplayColor.chromaticities.red");

        Vec2fPropertyEditor green(m_session->graph(), m_session->currentFrame(),
                                  "@RVDisplayColor.chromaticities.green");

        Vec2fPropertyEditor blue(m_session->graph(), m_session->currentFrame(),
                                 "@RVDisplayColor.chromaticities.blue");

        Vec2fPropertyEditor neutral(m_session->graph(),
                                    m_session->currentFrame(),
                                    "@RVDisplayColor.chromaticities.neutral");

        IntPropertyEditor active(m_session->graph(), m_session->currentFrame(),
                                 "@RVDisplayColor.chromaticities.active");

        IntPropertyEditor adoptedNeutral(
            m_session->graph(), m_session->currentFrame(),
            "@RVDisplayColor.chromaticities.adoptedNeutral");

        IntPropertyEditor dither(m_session->graph(), m_session->currentFrame(),
                                 "@RVDisplayColor.color.dither");

        m_noTransferAction->setChecked(false);
        m_srgbAction->setChecked(false);
        m_rec709Action->setChecked(false);
        m_g22Action->setChecked(false);
        m_g24Action->setChecked(false);
        m_g26Action->setChecked(false);
        m_gOtherAction->setChecked(false);
        m_gOtherAction->setVisible(false);
        m_gOtherAction->setText("   Other Gamma");

        bool none = true;
        if (srgb.value() == 1)
        {
            m_srgbAction->setChecked(true);
            none = false;
        };
        if (rec709.value() == 1)
        {
            m_rec709Action->setChecked(true);
            none = false;
        }

        if (gamma.value() != 1.0f)
        {
            float y = gamma.value();

            if (y == 2.2f)
            {
                m_g22Action->setChecked(true);
            }
            else if (y == 2.4f)
            {
                m_g24Action->setChecked(true);
            }
            else if (y == 2.6f)
            {
                m_g26Action->setChecked(true);
            }
            else
            {
                m_gOtherAction->setChecked(true);
                m_gOtherAction->setVisible(true);
                m_gOtherAction->setText(QString("   Other Gamma (%1)").arg(y));
                // do something here
            }

            none = false;
        }

        if (none)
        {
            m_noTransferAction->setChecked(true);
        }

        //
        //  Primaries
        //

        Chromaticities<float> currentChr(red.value(), green.value(),
                                         blue.value(), white.value());

        m_primaries709Action->setChecked(false);
        m_primariesP3Action->setChecked(false);
        m_primariesAdobeRGBAction->setChecked(false);
        m_primariesXYZAction->setChecked(false);
        m_primariesSMPTECAction->setChecked(false);
        m_primariesDreamColorAction->setChecked(false);

        if (currentChr == Chromaticities<float>::Rec709())
        {
            m_primaries709Action->setChecked(true);
        }
        else if (currentChr == Chromaticities<float>::P3())
        {
            m_primariesP3Action->setChecked(true);
        }
        else if (currentChr == Chromaticities<float>::AdobeRGB())
        {
            m_primariesAdobeRGBAction->setChecked(true);
        }
        else if (currentChr == Chromaticities<float>::XYZ())
        {
            m_primariesXYZAction->setChecked(true);
        }
        else if (currentChr == Chromaticities<float>::SMPTE_C())
        {
            m_primariesSMPTECAction->setChecked(true);
        }
        else if (currentChr == Chromaticities<float>::DreamcolorFull())
        {
            m_primariesDreamColorAction->setChecked(true);
        }

        m_ditherOff->setChecked(dither.value() == 0);
        m_dither8->setChecked(dither.value() == 8);
        m_dither10->setChecked(dither.value() == 10);
    }

    void RvTopViewToolBar::setTransferFunction(const string& name, float value)
    {
        IntPropertyEditor srgb(m_session->graph(), m_session->currentFrame(),
                               "@RVDisplayColor.color.sRGB");

        IntPropertyEditor rec709(m_session->graph(), m_session->currentFrame(),
                                 "@RVDisplayColor.color.Rec709");

        FloatPropertyEditor gamma(m_session->graph(), m_session->currentFrame(),
                                  "@RVDisplayColor.color.gamma");

        if (name == "srgb")
        {
            rec709.setValue(0);
            srgb.setValue(1);
            gamma.setValue(1.0);
        }
        else if (name == "rec709")
        {
            rec709.setValue(1);
            srgb.setValue(0);
            gamma.setValue(1.0);
        }
        else if (name == "gamma")
        {
            rec709.setValue(0);
            srgb.setValue(0);
            gamma.setValue(value);
        }
        else if (name == "none")
        {
            rec709.setValue(0);
            srgb.setValue(0);
            gamma.setValue(1.0);
        }
    }

    void RvTopViewToolBar::setPrimaries(const Chromaticities<float>& chr)
    {
        Vec2fPropertyEditor white(m_session->graph(), m_session->currentFrame(),
                                  "@RVDisplayColor.chromaticities.white");

        Vec2fPropertyEditor red(m_session->graph(), m_session->currentFrame(),
                                "@RVDisplayColor.chromaticities.red");

        Vec2fPropertyEditor green(m_session->graph(), m_session->currentFrame(),
                                  "@RVDisplayColor.chromaticities.green");

        Vec2fPropertyEditor blue(m_session->graph(), m_session->currentFrame(),
                                 "@RVDisplayColor.chromaticities.blue");

        Vec2fPropertyEditor neutral(m_session->graph(),
                                    m_session->currentFrame(),
                                    "@RVDisplayColor.chromaticities.neutral");

        IntPropertyEditor active(m_session->graph(), m_session->currentFrame(),
                                 "@RVDisplayColor.chromaticities.active");

        IntPropertyEditor adoptedNeutral(
            m_session->graph(), m_session->currentFrame(),
            "@RVDisplayColor.chromaticities.adoptedNeutral");

        active.setValue(1);
        adoptedNeutral.setValue(1);
        red.setValue(chr.red);
        green.setValue(chr.green);
        blue.setValue(chr.blue);
        white.setValue(chr.white);
        neutral.setValue(chr.white);
    }

    void RvTopViewToolBar::transferNone() { setTransferFunction("none", 0); }

    void RvTopViewToolBar::transferSRGB() { setTransferFunction("srgb", 0); }

    void RvTopViewToolBar::transferRec709()
    {
        setTransferFunction("rec709", 0);
    }

    void RvTopViewToolBar::transfer22() { setTransferFunction("gamma", 2.2); }

    void RvTopViewToolBar::transfer24() { setTransferFunction("gamma", 2.4); }

    void RvTopViewToolBar::transfer26() { setTransferFunction("gamma", 2.6); }

    void RvTopViewToolBar::primaries709()
    {
        setPrimaries(Chromaticities<float>::Rec709());
    }

    void RvTopViewToolBar::primariesP3()
    {
        setPrimaries(Chromaticities<float>::P3());
    }

    void RvTopViewToolBar::primariesSMPTEC()
    {
        setPrimaries(Chromaticities<float>::SMPTE_C());
    }

    void RvTopViewToolBar::primariesXYZ()
    {
        setPrimaries(Chromaticities<float>::XYZ());
    }

    void RvTopViewToolBar::primariesAdobeRGB()
    {
        setPrimaries(Chromaticities<float>::AdobeRGB());
    }

    void RvTopViewToolBar::primariesDreamColor()
    {
        setPrimaries(Chromaticities<float>::DreamcolorFull());
    }

    void RvTopViewToolBar::ditherOff()
    {
        bool ocio = hasOCIODisplayPipeline();
        IntPropertyEditor editor(m_session->graph(), m_session->currentFrame(),
                                 ocio ? "@OCIODisplay.color.dither" : "@RVDisplayColor.color.dither");

        editor.setValue(0);
    }

    void RvTopViewToolBar::dither8()
    {
        bool ocio = hasOCIODisplayPipeline();
        IntPropertyEditor editor(m_session->graph(), m_session->currentFrame(),
                                 ocio ? "@OCIODisplay.color.dither" : "@RVDisplayColor.color.dither");

        editor.setValue(8);
    }

    void RvTopViewToolBar::dither10()
    {
        bool ocio = hasOCIODisplayPipeline();
        IntPropertyEditor editor(m_session->graph(), m_session->currentFrame(),
                                 ocio ?  "@OCIODisplay.color.dither" : "@RVDisplayColor.color.dither");


        editor.setValue(10);
    }

    bool RvTopViewToolBar::event(QEvent* qevent)
    {
        if (qevent->type() == QEvent::Show)
        {
            // stereoMenuUpdate();
        }

        return QToolBar::event(qevent);
    }

    void RvTopViewToolBar::setFullscreen(bool fullscreen)
    {
        QToolButton* b =
            dynamic_cast<QToolButton*>(widgetForAction(m_fullScreenAction));
        b->setIcon(QIcon(fullscreen ? ":/images/unfullscreen"
                                    : ":/images/fullscreen.png"));
    }

} // namespace Rv
