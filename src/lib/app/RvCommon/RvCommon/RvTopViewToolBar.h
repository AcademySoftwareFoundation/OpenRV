//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __RvCommon__RvTopViewToolBar__h__
#define __RvCommon__RvTopViewToolBar__h__
#include <iostream>
#include <QtCore/QMap>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <QtGui/QIcon>
#include <TwkApp/EventNode.h>
#include <TwkMath/Chromaticities.h>

namespace TwkApp
{
    class VideoDevice;
}

namespace IPCore
{
    class Session;
}

namespace Rv
{

    class RvTopViewToolBar
        : public QToolBar
        , public TwkApp::EventNode
    {
        Q_OBJECT

    public:
        typedef std::vector<QAction*> QActionVector;
        typedef QMap<QString, QIcon> QIconMap;
        typedef TwkApp::VideoDevice VideoDevice;

        RvTopViewToolBar(QWidget*);
        virtual ~RvTopViewToolBar();

        void build();
        void makeActive(bool);
        void makeActiveFromSettings();

        void setSession(IPCore::Session*);
        virtual Result receiveEvent(const TwkApp::Event&);

        bool isInPresentationMode() const;

        void setStereo(const std::string&);
        void setChannel(int);

        void setDevice(const VideoDevice* d) { m_device = d; }

        void setTransferFunction(const std::string&, float);
        void setPrimaries(const TwkMath::Chromaticities<float>&);

        void setFullscreen(bool);

        virtual bool event(QEvent*);
        void bgMenuUpdate2();

    private slots:
        void nextViewNode();
        void previousViewNode();
        void frame();
        void fullscreen();

        void viewMenuUpdate();
        void viewMenuChanged(QAction*);
        void frameMenuChanged(QAction*);

        void bgMenuUpdate();
        void bgBlack();
        void bg18();
        void bg50();
        void bgWhite();
        void bgChecker();
        void bgHatch();

        void stereoMenuUpdate();
        void monoStereo();
        void anaglyphStereo();
        void lumanaglyphStereo();
        void sideBySideStereo();
        void mirroredStereo();
        void checkerStereo();
        void scanlineStereo();
        void leftStereo();
        void rightStereo();
        // void shutterStereo();
        void swapEyes();

        void channelMenuUpdate();
        void channelsRGB();
        void channelsR();
        void channelsG();
        void channelsB();
        void channelsA();
        void channelsL();

        void monitorMenuUpdate();

        void monitorMenuRVUpdate();
        void monitorMenuOCIOUpdate();
        void transferNone();
        void transferSRGB();
        void transferRec709();
        void transfer22();
        void transfer24();
        void transfer26();
        void primaries709();
        void primariesP3();
        void primariesSMPTEC();
        void primariesXYZ();
        void primariesAdobeRGB();
        void primariesDreamColor();
        void ditherOff();
        void dither8();
        void dither10();

        bool hasStandardDisplayPipeline();
        bool hasOCIODisplayPipeline();

    private:
        void updateActionToolButton(QAction* action, const std::string& text,
                                    const std::string& icon,
                                    bool forceRepaint = false);

    private:
        IPCore::Session* m_session;
        QAction* m_viewBackAction;
        QAction* m_viewForwardAction;
        QComboBox* m_viewCombo;
        QMenu* m_viewMenu;
        QAction* m_fullScreenAction;
        QAction* m_frameAction;
        QAction* m_bgMenuAction;
        QAction* m_blackBGAction;
        QAction* m_18GreyBGAction;
        QAction* m_50GreyBGAction;
        QAction* m_whiteBGAction;
        QAction* m_checkerBGAction;
        QAction* m_crossHatchBGAction;
        QAction* m_stereoMenuAction;
        QAction* m_monoStereoAction;
        QAction* m_anaglyphStereoAction;
        QAction* m_lumanaglyphStereoAction;
        QAction* m_sideBySideStereoAction;
        QAction* m_mirroredSideBySideStereoAction;
        QAction* m_checkerStereoAction;
        QAction* m_scanlineStereoAction;
        QAction* m_leftStereoAction;
        QAction* m_rightStereoAction;
        // Shutter mode gone since Qt6/QOpenGLWidget. No customer
        // demand for it. Maybe fix & re-enable if customers demand it and
        // leaving existing code in case customers want to revive it.
        // QAction* m_shutterStereoAction;
        QAction* m_swapStereoAction;
        QActionVector m_stereoActions;
        QAction* m_channelMenuAction;
        QAction* m_viewMenuAction;
        QAction* m_RGBChannelAction;
        QAction* m_RChannelAction;
        QAction* m_GChannelAction;
        QAction* m_BChannelAction;
        QAction* m_AChannelAction;
        QAction* m_LChannelAction;
        QMenu* m_monitorMenu;
        QAction* m_monitorMenuAction;
        QLabel* m_monitorInfoLabel;
        QAction* m_transferTitleAction;
        QAction* m_primariesTitleAction;
        QAction* m_ditherTitleAction;
        QAction* m_noTransferAction;
        QAction* m_srgbAction;
        QAction* m_rec709Action;
        QAction* m_g22Action;
        QAction* m_g24Action;
        QAction* m_g26Action;
        QAction* m_gOtherAction;
        QAction* m_primaries709Action;
        QAction* m_primariesP3Action;
        QAction* m_primariesXYZAction;
        QAction* m_primariesSMPTECAction;
        QAction* m_primariesAdobeRGBAction;
        QAction* m_primariesDreamColorAction;
        QAction* m_ditherOff;
        QAction* m_dither8;
        QAction* m_dither10;
        bool m_loading;
        QIconMap m_iconMap;
        const VideoDevice* m_device;
        const VideoDevice* m_outputDevice;
    };

} // namespace Rv

#endif // __RvCommon__RvTopViewToolBar__h__
