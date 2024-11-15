//******************************************************************************
// Copyright (c) 2008 Tweak Inc. 
// All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************

#ifdef PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include <TwkGLF/GL.h>

#include <RvCommon/QTUtils.h>
#include <RvCommon/RvApplication.h>
#include <RvCommon/RvDocument.h>
#include <RvCommon/RvPreferences.h>
#include <IOcin/IOcin.h>
#include <IOdpx/IOdpx.h>
#include <IOexr/IOexr.h>
#include <IOjpeg/IOjpeg.h>
#include <IOtarga/IOtarga.h>
#include <IOtiff/IOtiff.h>
#include <RvApp/FormatIPNode.h>
#include <IPCore/DisplayStereoIPNode.h>
#include <IPCore/Application.h>
#include <IPCore/AudioRenderer.h>
#include <IPCore/ImageRenderer.h>
#include <IPCore/LUTIPNode.h>
#include <IPCore/Profile.h>
#include <IPCore/FBCache.h>
#include <ImfThreading.h>
#include <QtGui/QtGui>
#include <QtWidgets/QStyleFactory>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>
#include <TwkQtCoreUtil/QtConvert.h>
#include <RvApp/RvSession.h>
#include <TwkApp/Bundle.h>
#include <TwkApp/VideoDevice.h>
#include <TwkContainer/PropertyContainer.h>
#include <TwkFB/IO.h>
#include <TwkAudio/AudioFormats.h>
#include <TwkMovie/MovieIO.h>
#include <TwkUtil/SystemInfo.h>
#include <TwkUtil/File.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unzip.h>
#include <yaml.h>

#ifdef PLATFORM_DARWIN
extern const char* rv_mac_aqua;
extern const char* rv_mac_dark;
#else
extern const char* rv_linux_dark;
#endif

namespace Rv {
using namespace std;
using namespace TwkApp;
using namespace IPCore;
using namespace TwkQtCoreUtil;

bool ScrollEventEater::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Wheel) return true;

    return QObject::eventFilter(obj, event);
}

static const char* CompositeRenderer = "Composite";
static const char* DirectRenderer    = "Direct";
static const char* AppleRenderer     = "Apple Composite";
static const char* Area              = "area";
static const char* Cubic             = "cubic";
static const char* Linear            = "linear";
static const char* Nearest           = "nearest";
static const char* StereoAnaglyph    = "anaglyph";
static const char* StereoLumAnaglyph = "lumanaglyph";
static const char* StereoPair        = "pair";
static const char* StereoMirror      = "mirror";
static const char* StereoLeft        = "left";
static const char* StereoRight       = "right";
static const char* StereoChecker     = "checker";
static const char* StereoScanline    = "scanline";
static const char* StereoHardware    = "hardware";
static const char* Solid0            = "black";
static const char* Solid18           = "grey18";
static const char* Solid50           = "grey50";
static const char* Solid100          = "white";
static const char* Checker           = "checker";
static const char* CrossHatch        = "crosshatch";
static const char* RGB8              = "RGB8";
static const char* RGBA8             = "RGBA8";
static const char* RGB8_PLANAR       = "RGB8_PLANAR";
static const char* RGB10_A2          = "RGB10_A2";
static const char* A2_BGR10          = "A2_BGR10";
static const char* RGB16             = "RGB16";
static const char* RGBA16            = "RGBA16";
static const char* RGB16_PLANAR      = "RGB16_PLANAR";

RvPreferences::RvPreferences(QWidget* parent) 
    : QDialog(parent, Qt::Window),
      PackageManager(),
      m_showHiddenPackages(false),
      m_updated(false),
      m_currentVideoModule(-1),
      m_currentVideoDevice(-1),
      m_lockPresentCheck(false),
      m_packageLocationDialog(0),
      m_videoLatencyDialog(0)
{
    //
    //  We don't want wheel events or other scrolling to affect the combo
    //  boxes, so filter them out.
    //
    ScrollEventEater *scrollEventEater = new ScrollEventEater(this);

    m_ui.setupUi(this);
    connect(m_ui.exrAutoThreadsToggle, SIGNAL(stateChanged(int)),
            this, SLOT(exrAutoThreads(int)));
    connect(m_ui.exrNumThreadsEdit, SIGNAL(textChanged(const QString&)),
            this, SLOT(exrThreadNumChanged(const QString&)));
    setWindowTitle(UI_APPLICATION_NAME " Preferences");

#ifndef PLATFORM_DARWIN
    m_ui.appleClientStorageToggle->setEnabled(false);
#endif

    m_packageModel = new QStandardItemModel();
    m_ui.extTreeView->setModel(m_packageModel);
    m_ui.extTreeView->setAlternatingRowColors(true);
    QItemSelectionModel* s = m_ui.extTreeView->selectionModel();

    connect(m_ui.stylusAsMouseToggle, SIGNAL(stateChanged(int)),
            this, SLOT(stylusAsMouseChanged(int)));

    connect(m_ui.startupResizeToggle, SIGNAL(stateChanged(int)),
            this, SLOT(startupResizeChanged(int)));

    connect(s, SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
            this, SLOT(packageSelection(const QItemSelection&, const QItemSelection&)));

    connect(m_ui.extTreeView, SIGNAL(clicked(QModelIndex)),
            this, SLOT(clickedPackage(QModelIndex)));

    connect(m_ui.extAddButton, SIGNAL(clicked(bool)), this, SLOT(addPackage(bool)));
    connect(m_ui.extRemoveButton, SIGNAL(clicked(bool)), this, SLOT(removePackage(bool)));
    connect(m_ui.extShowHiddenToggle, SIGNAL(clicked(bool)), this, SLOT(showHiddenPackages(bool)));

    connect(m_ui.audioModuleCombo, SIGNAL(activated(int)),
            this, SLOT(audioModuleChanged(int)));
    m_ui.audioModuleCombo->installEventFilter(scrollEventEater);

    connect(m_ui.audioDeviceCombo, SIGNAL(activated(int)),
            this, SLOT(audioDeviceChanged(int)));
    m_ui.audioDeviceCombo->installEventFilter(scrollEventEater);

    connect(m_ui.audioDeviceRateCombo, SIGNAL(activated(int)),
            this, SLOT(audioRateChanged(int)));
    m_ui.audioDeviceRateCombo->installEventFilter(scrollEventEater);

    connect(m_ui.audioDeviceFormatCombo, SIGNAL(activated(int)),
            this, SLOT(audioFormatChanged(int)));
    m_ui.audioDeviceFormatCombo->installEventFilter(scrollEventEater);

    connect(m_ui.audioDeviceLayoutCombo, SIGNAL(activated(int)),
            this, SLOT(audioChannelsChanged(int)));
    m_ui.audioDeviceLayoutCombo->installEventFilter(scrollEventEater);

    connect(m_ui.audioHoldOpenToggle, SIGNAL(stateChanged(int)),
            this, SLOT(audioHoldOpenChanged(int)));

    connect(m_ui.audioHardwareLockToggle, SIGNAL(stateChanged(int)),
            this, SLOT(audioVideoSyncChanged(int)));

    connect(m_ui.audioPreRollToggle, SIGNAL(stateChanged(int)),
            this, SLOT(audioPreRollChanged(int)));

    connect(m_ui.audioDevicePacketEdit, SIGNAL(editingFinished()),
            this, SLOT(audioDevicePacketChanged()));

    connect(m_ui.audioCachePacketEdit, SIGNAL(editingFinished()),
            this, SLOT(audioCachePacketChanged()));

    connect(m_ui.audioGlobalOffsetEdit, SIGNAL(editingFinished()),
            this, SLOT(audioOffsetFinished()));

    connect(m_ui.audioDeviceLatencyEdit, SIGNAL(editingFinished()),
            this, SLOT(audioDeviceLatencyFinished()));


    // Only connect the update slot to the architecture appropriate signal
    #ifdef ARCH_IA32
        connect(m_ui.lookAheadCacheSize32Edit, SIGNAL(editingFinished()),
                this, SLOT(lookaheadCacheSizeFinisihed()));
        
        connect(m_ui.regionCacheSize32Edit, SIGNAL(editingFinished()),
                this, SLOT(regionCacheSizeFinisihed()));
    #else
        connect(m_ui.lookAheadCacheSize64Edit, SIGNAL(editingFinished()),
                this, SLOT(lookaheadCacheSizeFinisihed()));

        connect(m_ui.regionCacheSize64Edit, SIGNAL(editingFinished()),
                this, SLOT(regionCacheSizeFinisihed()));
    #endif

    connect(m_ui.bufferWaitEdit, SIGNAL(editingFinished()),
            this, SLOT(bufferWaitFinished()));

    connect(m_ui.lookBehindFracEdit, SIGNAL(editingFinished()),
            this, SLOT(lookBehindFinished()));

    connect(m_ui.cacheOutsideRegionToggle, SIGNAL(stateChanged(int)),
            this, SLOT(cacheOutsideRegionChanged(int)));

    connect(m_ui.rthreadEdit, SIGNAL(editingFinished()),
            this, SLOT(rthreadFinished()));

    connect(m_ui.bitDepthCombo, SIGNAL(activated(int)),
            this, SLOT(bitDepthChanged(int)));
    m_ui.bitDepthCombo->installEventFilter(scrollEventEater);

    connect(m_ui.allowFloatToggle, SIGNAL(stateChanged(int)),
            this, SLOT(allowFloatChanged(int)));

    connect(m_ui.newGLSLlutInterpToggle, SIGNAL(stateChanged(int)),
            this, SLOT(newGLSLlutInterpChanged(int)));

    connect(m_ui.swapScanlinesToggle, SIGNAL(stateChanged(int)),
            this, SLOT(swapScanlinesChanged(int)));

    connect(m_ui.prefetchToggle, SIGNAL(stateChanged(int)),
            this, SLOT(prefetchChanged(int)));

    connect(m_ui.appleClientStorageToggle, SIGNAL(stateChanged(int)),
            this, SLOT(appleClientStorageChanged(int)));

    connect(m_ui.useThreadedUploadToggle, SIGNAL(stateChanged(int)),
            this, SLOT(useThreadedUploadChanged(int)));

    connect(m_ui.displayVideoSyncButton, SIGNAL(stateChanged(int)),
            this, SLOT(videoSyncChanged(int)));

    connect(m_ui.configureVideoLatencyButton, SIGNAL(clicked()),
            this, SLOT(configVideoLatency()));

    connect(m_ui.exrRGBAToggle, SIGNAL(stateChanged(int)),
            this, SLOT(exrRGBAChanged(int)));

    connect(m_ui.exrInheritToggle, SIGNAL(stateChanged(int)),
            this, SLOT(exrInheritChanged(int)));

    connect(m_ui.exrPlanar3ChannelToggle, SIGNAL(stateChanged(int)),
            this, SLOT(exrPlanar3ChannelChanged(int)));

    connect(m_ui.exrNoOneChannelToggle, SIGNAL(stateChanged(int)),
            this, SLOT(exrNoOneChannelChanged(int)));

    connect(m_ui.exrReadWindowIsDisplayWindowToggle, SIGNAL(stateChanged(int)),
            this, SLOT(exrReadWindowIsDisplayWindowChanged(int)));

    connect(m_ui.exrNumThreadsEdit, SIGNAL(editingFinished()),
            this, SLOT(exrNumThreadsFinished()));

    connect(m_ui.exrChunkSizeEdit, SIGNAL(editingFinished()),
            this, SLOT(exrChunkSizeFinished()));

    connect(m_ui.exrMaxInFlightEdit, SIGNAL(editingFinished()),
            this, SLOT(exrMaxInFlightFinished()));

    connect(m_ui.exrIOMethodCombo, SIGNAL(activated(int)),
            this, SLOT(exrIOMethodChanged(int)));
    m_ui.exrIOMethodCombo->installEventFilter(scrollEventEater);

    connect(m_ui.exrReadWindowCombo, SIGNAL(activated(int)),
            this, SLOT(exrReadWindowChanged(int)));
    m_ui.exrReadWindowCombo->installEventFilter(scrollEventEater);

    connect(m_ui.cinChunkSizeEdit, SIGNAL(editingFinished()),
            this, SLOT(cinChunkSizeFinished()));

    connect(m_ui.cinMaxInFlightEdit, SIGNAL(editingFinished()),
            this, SLOT(cinMaxInFlightFinished()));

    connect(m_ui.cinIOMethodCombo, SIGNAL(activated(int)),
            this, SLOT(cinIOMethodChanged(int)));
    m_ui.cinIOMethodCombo->installEventFilter(scrollEventEater);

    connect(m_ui.cinDisplayPixelCombo, SIGNAL(activated(int)),
            this, SLOT(cinPixelsChanged(int)));
    m_ui.cinDisplayPixelCombo->installEventFilter(scrollEventEater);

    connect(m_ui.cinChromaToggle, SIGNAL(stateChanged(int)),
            this, SLOT(cinChromaChanged(int)));

    connect(m_ui.dpxChunkSizeEdit, SIGNAL(editingFinished()),
            this, SLOT(dpxChunkSizeFinished()));

    connect(m_ui.dpxMaxInFlightEdit, SIGNAL(editingFinished()),
            this, SLOT(dpxMaxInFlightFinished()));

    connect(m_ui.dpxIOMethodCombo, SIGNAL(activated(int)),
            this, SLOT(dpxIOMethodChanged(int)));
    m_ui.dpxIOMethodCombo->installEventFilter(scrollEventEater);

    connect(m_ui.dpxDisplayPixelCombo, SIGNAL(activated(int)),
            this, SLOT(dpxPixelsChanged(int)));
    m_ui.dpxDisplayPixelCombo->installEventFilter(scrollEventEater);

    connect(m_ui.dpxChromaToggle, SIGNAL(stateChanged(int)),
            this, SLOT(dpxChromaChanged(int)));

    connect(m_ui.tgaChunkSizeEdit, SIGNAL(editingFinished()),
            this, SLOT(tgaChunkSizeFinished()));

    connect(m_ui.tgaMaxInFlightEdit, SIGNAL(editingFinished()),
            this, SLOT(tgaMaxInFlightFinished()));

    connect(m_ui.tgaIOMethodCombo, SIGNAL(activated(int)),
            this, SLOT(tgaIOMethodChanged(int)));
    m_ui.tgaIOMethodCombo->installEventFilter(scrollEventEater);

    connect(m_ui.tifChunkSizeEdit, SIGNAL(editingFinished()),
            this, SLOT(tiffChunkSizeFinished()));

    connect(m_ui.tifMaxInFlightEdit, SIGNAL(editingFinished()),
            this, SLOT(tiffMaxInFlightFinished()));

    connect(m_ui.tifIOMethodCombo, SIGNAL(activated(int)),
            this, SLOT(tiffIOMethodChanged(int)));
    m_ui.tifIOMethodCombo->installEventFilter(scrollEventEater);

    connect(m_ui.jpegChunkSizeEdit, SIGNAL(editingFinished()),
            this, SLOT(jpegChunkSizeFinished()));

    connect(m_ui.jpegMaxInFlightEdit, SIGNAL(editingFinished()),
            this, SLOT(jpegMaxInFlightFinished()));

    connect(m_ui.jpegIOMethodCombo, SIGNAL(activated(int)),
            this, SLOT(jpegIOMethodChanged(int)));
    m_ui.jpegIOMethodCombo->installEventFilter(scrollEventEater);

    connect(m_ui.jpegRGBAToggle, SIGNAL(stateChanged(int)),
            this, SLOT(jpegRGBAChanged(int)));

    connect(m_ui.displayOutputCombo, SIGNAL(activated(int)),
            this, SLOT(displayOutputChanged(int)));

    connect(m_ui.fontSizeSpinBox, SIGNAL(editingFinished()),
            this, SLOT(fontChanged()));

    connect(m_ui.fontSize2SpinBox, SIGNAL(editingFinished()),
            this, SLOT(fontChanged()));

    m_ui.displayOutputCombo->installEventFilter(scrollEventEater);

    #ifdef PLATFORM_WINDOWS
        m_ui.exrMaxInFlightEdit->setEnabled(false);
        m_ui.cinMaxInFlightEdit->setEnabled(false);
        m_ui.dpxMaxInFlightEdit->setEnabled(false);
        m_ui.tgaMaxInFlightEdit->setEnabled(false);
        m_ui.tifMaxInFlightEdit->setEnabled(false);
        m_ui.jpegMaxInFlightEdit->setEnabled(false);

        m_ui.exrMaxInFlightLabel->setEnabled(false);
        m_ui.cinMaxInFlightLabel->setEnabled(false);
        m_ui.dpxMaxInFlightLabel->setEnabled(false);
        m_ui.tgaMaxInFlightLabel->setEnabled(false);
        m_ui.tifMaxInFlightLabel->setEnabled(false);
        m_ui.jpegMaxInFlightLabel->setEnabled(false);
    #endif

    m_ui.extRemoveButton->setEnabled(false);
    m_ui.extTextBrowser->zoomOut();

    connect(m_ui.videoModuleCombo, SIGNAL(activated(int)), this, SLOT(videoModuleChanged(int)));
    connect(m_ui.videoDeviceCombo, SIGNAL(activated(int)), this, SLOT(videoDeviceChanged(int)));
    connect(m_ui.videoFormatCombo, SIGNAL(activated(int)), this, SLOT(videoFormatChanged(int)));
    connect(m_ui.video4KTransportCombo, SIGNAL(activated(int)), this, SLOT(video4KTransportChanged(int)));
    connect(m_ui.videoAudioFormatCombo, SIGNAL(activated(int)), this, SLOT(videoAudioFormatChanged(int)));
    connect(m_ui.dataFormatCombo, SIGNAL(activated(int)), this, SLOT(videoDataFormatChanged(int)));
    connect(m_ui.syncMethodCombo, SIGNAL(activated(int)), this, SLOT(syncMethodChanged(int)));
    connect(m_ui.syncSourceCombo, SIGNAL(activated(int)), this, SLOT(syncSourceChanged(int)));
    connect(m_ui.videoAudioDeviceCheckBox, SIGNAL(stateChanged(int)), this, SLOT(videoAudioCheckBoxChanged(int)));
    connect(m_ui.videoSwapStereoEyesCheckBox, SIGNAL(stateChanged(int)), this, SLOT(videoSwapStereoEyesChanged(int)));
    connect(m_ui.useVideoLatencyCheckBox, SIGNAL(stateChanged(int)), this, SLOT(videoUseLatencyCheckBoxChanged(int)));
    connect(m_ui.presentationCheckBox, SIGNAL(stateChanged(int)), this, SLOT(presentationCheckBoxChanged(int)));
    connect(m_ui.additionalOptionsEdit, SIGNAL(textChanged()), this, SLOT(videoAdditionalOptionsChanged()));

    connect(m_ui.moduleProfileCombo, SIGNAL(activated(int)), this, SLOT(modProfileChanged(int)));
    connect(m_ui.deviceProfileCombo, SIGNAL(activated(int)), this, SLOT(devProfileChanged(int)));
    connect(m_ui.formatProfileCombo, SIGNAL(activated(int)), this, SLOT(formatProfileChanged(int)));

    m_ui.videoModuleCombo->installEventFilter(scrollEventEater);
    m_ui.videoDeviceCombo->installEventFilter(scrollEventEater);
    m_ui.videoFormatCombo->installEventFilter(scrollEventEater);
    m_ui.video4KTransportCombo->installEventFilter(scrollEventEater);
    m_ui.videoAudioFormatCombo->installEventFilter(scrollEventEater);
    m_ui.dataFormatCombo->installEventFilter(scrollEventEater);
    m_ui.syncMethodCombo->installEventFilter(scrollEventEater);
    m_ui.syncSourceCombo->installEventFilter(scrollEventEater);

    m_ui.playbackModeCombo->installEventFilter(scrollEventEater);
    m_ui.startupScreenCombo->installEventFilter(scrollEventEater);
    m_ui.stereoModeCombo->installEventFilter(scrollEventEater);
    m_ui.cacheModeCombo->installEventFilter(scrollEventEater);
    m_ui.resampleMethodCombo->installEventFilter(scrollEventEater);
    m_ui.imageFilterCombo->installEventFilter(scrollEventEater);
    m_ui.bgPatternCombo->installEventFilter(scrollEventEater);
}

RvPreferences::~RvPreferences()
{
}

void
RvPreferences::update()
{
    if (isVisible()) return;
    Options opts;
    loadSettingsIntoOptions(opts);
    RV_QSETTINGS;
    QString s;
    float f;
    int n = 0;

    loadPackages();

#ifdef PLATFORM_DARWIN
    m_ui.noMenuBarToggle->setDisabled(true);
#endif

    //----------------------------------------------------------------------
    settings.beginGroup("Controls");

    m_ui.clickToPlayEnableToggle->setCheckState(opts.clickToPlayEnable ? Qt::Checked : Qt::Unchecked);
    m_ui.scrubEnableToggle->setCheckState(opts.scrubEnable ? Qt::Checked : Qt::Unchecked);

    settings.endGroup();

    //----------------------------------------------------------------------
    settings.beginGroup("General");

    m_ui.fontSizeSpinBox->setValue(opts.fontSize1);
    m_ui.fontSize2SpinBox->setValue(opts.fontSize2);

    m_ui.playbackModeCombo->setCurrentIndex(opts.playMode);
    m_ui.playOnStartupToggle->setCheckState(opts.play ? Qt::Checked : Qt::Unchecked);
    m_ui.reuseSessionToggle->setCheckState(opts.urlsReuseSession ? Qt::Checked : Qt::Unchecked);
    m_ui.networkToggle->setCheckState(opts.networkOnStartup ? Qt::Checked : Qt::Unchecked);
    m_ui.stylusAsMouseToggle->setCheckState(opts.stylusAsMouse ? Qt::Checked : Qt::Unchecked);
    m_ui.startupResizeToggle->setCheckState(opts.startupResize ? Qt::Checked : Qt::Unchecked);
    m_ui.desktopAwareToggle->setCheckState(opts.qtdesktop ? Qt::Checked : Qt::Unchecked);
    m_ui.noMenuBarToggle->setCheckState(opts.nomb ? Qt::Checked : Qt::Unchecked);
    m_ui.fullscreenOnStartupToggle->setCheckState(opts.fullscreen ? Qt::Checked : Qt::Unchecked);
    s.setNum(opts.defaultfps, 'g', 4); m_ui.fpsEdit->setText(s);
    s.setNum(opts.readerThreads); m_ui.rthreadEdit->setText(s);
    m_ui.networkHostEdit->setText(opts.networkHost ?  opts.networkHost : "");
    m_ui.autoRetimeToggle->setCheckState(opts.autoRetime ? Qt::Checked : Qt::Unchecked);
    m_ui.useCrashReporterToggle->setCheckState(Qt::Unchecked);
    m_ui.useCrashReporterToggle->setDisabled(true);

    m_ui.startupScreenCombo->clear();
    m_ui.startupScreenCombo->addItem("Follow Pointer");
    for (int i = 0; i < QApplication::desktop()->screenCount(); ++i)
    {
        m_ui.startupScreenCombo->addItem(QString("Screen %1").arg((unsigned int)(i)));
    }
    m_ui.startupScreenCombo->setCurrentIndex ((opts.screen < m_ui.startupScreenCombo->count()-1) ? opts.screen+1 : 0);

    n = 0;
    if (opts.stereoMode)
    {
        string s = opts.stereoMode;
        if (s == StereoAnaglyph) n = 1;
        else if (s == StereoPair) n = 2;
        else if (s == StereoMirror) n = 3;
        else if (s == StereoLeft) n = 4;
        else if (s == StereoRight) n = 5;
        else if (s == StereoChecker) n = 6;
        else if (s == StereoScanline) n = 7;
        else if (s == StereoHardware) n = 8;
        else if ( s == StereoLumAnaglyph) n = 9;
    }

    m_ui.stereoModeCombo->setCurrentIndex(n);

    settings.endGroup();

    //----------------------------------------------------------------------
    settings.beginGroup("Caching");

    n = 0;
    if (opts.useCache) n = 1;
    if (opts.useLCache) n = 2;

    m_ui.cacheModeCombo->setCurrentIndex(settings.value("cacheMode", n).toInt());

    // Pull the achitecture specific settings... 
    double max32lram = settings.value("lookAheadCacheSize32New", opts.maxlram).toDouble();
    double max64lram = settings.value("lookAheadCacheSize64New", opts.maxlram).toDouble();
    double max32cram = settings.value("regionCacheSize32New",    opts.maxcram).toDouble();
    double max64cram = settings.value("regionCacheSize64New",    opts.maxcram).toDouble();

    // ...and populate the preference fields
    //s.setNum(max32lram, 'g', 3); m_ui.lookAheadCacheSize32Edit->setText(s);
    s.setNum(max64lram, 'g', 3); m_ui.lookAheadCacheSize64Edit->setText(s);
    //s.setNum(max32cram, 'g', 3); m_ui.regionCacheSize32Edit->setText(s);
    s.setNum(max64cram, 'g', 3); m_ui.regionCacheSize64Edit->setText(s);

    s.setNum(opts.maxbwait, 'g', 3); m_ui.bufferWaitEdit->setText(s);
    s.setNum(opts.lookback, 'g', 3); m_ui.lookBehindFracEdit->setText(s);

    m_ui.cacheOutsideRegionToggle->setCheckState(opts.cacheOutsideRegion ? Qt::Checked : Qt::Unchecked);

    settings.endGroup();

    //----------------------------------------------------------------------
    settings.beginGroup("Rendering");

    n = 0;
    if (opts.maxbits == 16) n = 1;
    if (opts.maxbits == 8) n = 2;

    m_ui.bitDepthCombo->setCurrentIndex(n);
    m_ui.allowFloatToggle->setCheckState(opts.nofloat ? Qt::Unchecked : Qt::Checked);
    m_ui.swapScanlinesToggle->setCheckState(opts.swapScanlines ? Qt::Checked : Qt::Unchecked);
    m_ui.prefetchToggle->setCheckState(opts.prefetch ? Qt::Checked : Qt::Unchecked);
    m_ui.newGLSLlutInterpToggle->setCheckState(opts.newGLSLlutInterp ? Qt::Checked : Qt::Unchecked);

    if (ImageRenderer::queryClientStorage())
    {
        m_ui.appleClientStorageToggle->setCheckState(opts.useAppleClientStorage ? Qt::Checked : Qt::Unchecked);
    }
    else
    {
        m_ui.appleClientStorageToggle->setEnabled(false);
    }

    if (ImageRenderer::queryThreadedUpload())
    {
        m_ui.useThreadedUploadToggle->setCheckState(opts.useThreadedUpload ? Qt::Checked : Qt::Unchecked);
    }
    else
    {
        m_ui.useThreadedUploadToggle->setEnabled(false);
    }

    n = 0;
    if (opts.resampleMethod)
    {
        // "area" is n=0.
        if (!strcmp(opts.resampleMethod, "cubic")) n = 1;
        else if (!strcmp(opts.resampleMethod, "linear")) n = 2;
        else if (!strcmp(opts.resampleMethod, "nearest")) n = 3;
    }
    m_ui.resampleMethodCombo->setCurrentIndex(n);

    if (opts.imageFilter == GL_LINEAR)  m_ui.imageFilterCombo->setCurrentIndex(0);
    if (opts.imageFilter == GL_NEAREST) m_ui.imageFilterCombo->setCurrentIndex(1);

    n = 0;
    if (opts.bgpattern)
    {
        if (!strcmp(opts.bgpattern, "grey18")) n = 1;
        else if (!strcmp(opts.bgpattern, "grey50")) n = 2;
        else if (!strcmp(opts.bgpattern, "white")) n = 3;
        else if (!strcmp(opts.bgpattern, "checker")) n = 4;
        else if (!strcmp(opts.bgpattern, "crosshatch")) n = 5;
    }

    m_ui.bgPatternCombo->setCurrentIndex(n);

    settings.endGroup();

    //----------------------------------------------------------------------
    settings.beginGroup("Audio");

    if (AudioRenderer::audioDisabledAlways())
    {
        m_ui.audioModuleCombo->clear();
        m_ui.audioDeviceCombo->clear();
        m_ui.audioDeviceLayoutCombo->clear();
        m_ui.audioDeviceFormatCombo->clear();
        m_ui.audioDeviceRateCombo->clear();
        m_ui.audioModuleCombo->addItem("Audio Disabled");
        m_ui.audioModuleCombo->setItemIcon(0, colorAdjustedIcon(":images/mute_32x32.png"));
        m_ui.audioDeviceCombo->setEnabled(false);
        m_ui.audioDeviceLayoutCombo->setEnabled(false);
        m_ui.audioDeviceFormatCombo->setEnabled(false);
        m_ui.audioDeviceRateCombo->setEnabled(false);
    }
    else if (!m_ui.audioModuleCombo->count())
    {
        const AudioRenderer::ModuleVector& modules = AudioRenderer::modules();
        size_t current = 0;

        for (size_t i= 0; i < modules.size(); i++)
        {
            if (modules[i].internal) break;
            if (opts.audioModule && modules[i].name == opts.audioModule) current = i;
            m_ui.audioModuleCombo->addItem(QString::fromUtf8(modules[i].name.c_str()));
        }

        m_ui.audioModuleCombo->setCurrentIndex(current);

        //
        //  This will take care of the remaining related UI 
        //

        audioModuleChanged(current);
    }

    m_ui.volumeSlider->setSliderPosition(int(opts.volume * 100.0));
    m_ui.audioHoldOpenToggle->setCheckState(opts.audioNice ? Qt::Unchecked : Qt::Checked);
    m_ui.audioHardwareLockToggle->setCheckState(opts.audioNoLock ? Qt::Unchecked : Qt::Checked);
    m_ui.audioScrubAtLaunchToggle->setCheckState(opts.audioScrub ? Qt::Checked : Qt::Unchecked);
    m_ui.audioPreRollToggle->setCheckState(opts.audioPreRoll ? Qt::Checked : Qt::Unchecked);

    s.setNum(opts.aframesize); m_ui.audioDevicePacketEdit->setText(s);
    s.setNum(opts.acachesize); m_ui.audioCachePacketEdit->setText(s);
    s.setNum(opts.audioMinCache, 'g', 2); m_ui.audioCacheMinEdit->setText(s);
    s.setNum(opts.audioMaxCache, 'g', 2); m_ui.audioCacheMaxEdit->setText(s);
    s.setNum(opts.audioGlobalOffset, 'g', 4); m_ui.audioGlobalOffsetEdit->setText(s);
    s.setNum(opts.audioDeviceLatency, 'g', 4); m_ui.audioDeviceLatencyEdit->setText(s);
    //s.setNum(opts.audioRate, 'g', 10); m_ui.audioDeviceRateEdit->setText(s);

    TwkAudio::Format currentFormat;
    switch (opts.audioPrecision)
    {
        default:
        case 32: currentFormat = TwkAudio::Float32Format; break;
        case 24: currentFormat = TwkAudio::Int24Format; break;
        case 16: currentFormat = TwkAudio::Int16Format; break;
        case 8: currentFormat = TwkAudio::Int8Format;; break;
        case -32: currentFormat = TwkAudio::Int32Format; break;
    }

    for (int index=0; index <  m_ui.audioDeviceFormatCombo->count(); ++index)
    {
        if (m_ui.audioDeviceFormatCombo->itemData(index) == currentFormat)
        {
            m_ui.audioDeviceFormatCombo->setCurrentIndex(index);
            break;
        }
    }

    for (int index=0; index <  m_ui.audioDeviceLayoutCombo->count(); ++index)
    {
        if (m_ui.audioDeviceLayoutCombo->itemData(index) == opts.audioLayout)
        {
            m_ui.audioDeviceLayoutCombo->setCurrentIndex(index);
            break;
        }
    }

    settings.endGroup();

    //----------------------------------------------------------------------
    settings.beginGroup("Display");

    m_ui.displayVideoSyncButton->setCheckState(opts.vsync ? Qt::Checked : Qt::Unchecked);
    
    int dindex = 0;

    if (opts.dispRedBits == 8 && 
        opts.dispGreenBits == 8 &&
        opts.dispBlueBits == 8 &&
        opts.dispAlphaBits == 8) 
    {
        dindex = 1;
    }
    else if (opts.dispRedBits == 10 &&
             opts.dispGreenBits == 10 &&
             opts.dispBlueBits == 10 &&
             opts.dispAlphaBits == 2) 
    {
        dindex = 2;
    }
    else
    {
        dindex = 0;
    }

    m_ui.displayOutputCombo->setCurrentIndex(dindex);

    settings.endGroup();

    //----------------------------------------------------------------------
    settings.beginGroup("OpenEXR");
    m_ui.exrRGBAToggle->setCheckState(opts.exrRGBA ? Qt::Checked : Qt::Unchecked);
    m_ui.exrInheritToggle->setCheckState(opts.exrInherit ? Qt::Checked : Qt::Unchecked);
    m_ui.exrPlanar3ChannelToggle->setCheckState(opts.exrPlanar3Chan ? Qt::Checked : Qt::Unchecked);
    m_ui.exrNoOneChannelToggle->setCheckState(opts.exrNoOneChannel ? Qt::Checked : Qt::Unchecked);
    m_ui.exrReadWindowIsDisplayWindowToggle->setCheckState(opts.exrReadWindowIsDisplayWindow ? Qt::Checked : Qt::Unchecked);
    m_ui.exrAutoThreadsToggle->setCheckState(opts.exrcpus == 0 ? Qt::Checked : Qt::Unchecked);
    s.setNum(opts.exrcpus);
    m_ui.exrNumThreadsEdit->setText(s);
    m_ui.exrIOMethodCombo->setCurrentIndex(opts.exrIOMethod);
    m_ui.exrReadWindowCombo->setCurrentIndex(opts.exrReadWindow);
    m_ui.exrNumThreadsEdit->setEnabled(opts.exrcpus != 0);
    m_ui.exrThreadsLabel->setEnabled(opts.exrcpus != 0);
    s.setNum(opts.exrIOSize);
    m_ui.exrChunkSizeEdit->setText(s);
    s.setNum(opts.exrMaxAsync);
    m_ui.exrMaxInFlightEdit->setText(s);
    settings.endGroup();

    //----------------------------------------------------------------------
    settings.beginGroup("JPEG");
    m_ui.jpegRGBAToggle->setCheckState(opts.jpegRGBA ? Qt::Checked : Qt::Unchecked);
    m_ui.jpegIOMethodCombo->setCurrentIndex(opts.jpegIOMethod);
    s.setNum(opts.jpegIOSize);
    m_ui.jpegChunkSizeEdit->setText(s);
    s.setNum(opts.jpegMaxAsync);
    m_ui.jpegMaxInFlightEdit->setText(s);
    settings.endGroup();

    //----------------------------------------------------------------------
    settings.beginGroup("Cineon");
    m_ui.cinIOMethodCombo->setCurrentIndex(opts.cinIOMethod);
    s.setNum(opts.cinIOSize);
    m_ui.cinChunkSizeEdit->setText(s);
    s.setNum(opts.cinMaxAsync);
    m_ui.cinMaxInFlightEdit->setText(s);
    m_ui.cinChromaToggle->setCheckState(opts.cinchroma ? Qt::Checked : Qt::Unchecked);

    n = 2;
    s = opts.cinPixel;
    if      (s == RGB8)         n = 0;
    else if (s == RGBA8)        n = 1;
    else if (s == RGB8_PLANAR)  n = 2;
    else if (s == RGB10_A2)     n = 3;
    else if (s == A2_BGR10)     n = 4;
    else if (s == RGB16)        n = 5;
    else if (s == RGBA16)       n = 6;
    else if (s == RGB16_PLANAR) n = 7;
    m_ui.cinDisplayPixelCombo->setCurrentIndex(n);

    settings.endGroup();

    //----------------------------------------------------------------------
    settings.beginGroup("DPX");
    m_ui.dpxIOMethodCombo->setCurrentIndex(opts.dpxIOMethod);
    s.setNum(opts.dpxIOSize);
    m_ui.dpxChunkSizeEdit->setText(s);
    s.setNum(opts.dpxMaxAsync);
    m_ui.dpxMaxInFlightEdit->setText(s);
    m_ui.dpxChromaToggle->setCheckState(opts.dpxchroma ? Qt::Checked : Qt::Unchecked);

    n = 2;
    s = opts.dpxPixel;
    if      (s == RGB8)         n = 0;
    else if (s == RGBA8)        n = 1;
    else if (s == RGB8_PLANAR)  n = 2;
    else if (s == RGB10_A2)     n = 3;
    else if (s == A2_BGR10)     n = 4;
    else if (s == RGB16)        n = 5;
    else if (s == RGBA16)       n = 6;
    else if (s == RGB16_PLANAR) n = 7;
    m_ui.dpxDisplayPixelCombo->setCurrentIndex(n);

    settings.endGroup();

    //----------------------------------------------------------------------
    settings.beginGroup("TGA");
    m_ui.tgaIOMethodCombo->setCurrentIndex(opts.tgaIOMethod);
    s.setNum(opts.tgaIOSize);
    m_ui.tgaChunkSizeEdit->setText(s);
    s.setNum(opts.tgaMaxAsync);
    m_ui.tgaMaxInFlightEdit->setText(s);
    settings.endGroup();

    //----------------------------------------------------------------------
    settings.beginGroup("TIFF");
    m_ui.tifIOMethodCombo->setCurrentIndex(opts.tiffIOMethod);
    s.setNum(opts.tiffIOSize);
    m_ui.tifChunkSizeEdit->setText(s);
    s.setNum(opts.tiffMaxAsync);
    m_ui.tifMaxInFlightEdit->setText(s);
    settings.endGroup();

    //----------------------------------------------------------------------
    // VIDEO

    updateVideo();
    m_updated = true;
}

void
RvPreferences::resetPreferencesFile()
{
#ifdef PLATFORM_WINDOWS
    QSettings::setDefaultFormat(QSettings::IniFormat);
#endif
    
    QCoreApplication::setOrganizationName(INTERNAL_ORGANIZATION_NAME);
    QCoreApplication::setOrganizationDomain(INTERNAL_ORGANIZATION_DOMAIN);
    QCoreApplication::setApplicationName(INTERNAL_APPLICATION_NAME);

    QSettings user;
    user.clear();
    user.sync();
}

void
RvPreferences::loadSettingsIntoOptions(Options& opts)
{
    RV_QSETTINGS;

    loadSettingsIntoOptions (settings, opts);
}

void
RvPreferences::loadSettingsIntoOptions(RvSettings& settings, Options& opts)
{
    QString s;
    settings.beginGroup("Controls");

    opts.clickToPlayEnable = int(!settings.value("disableClickToPlay", opts.clickToPlayEnable ? false : true).toBool());
    opts.scrubEnable       = int(!settings.value("disableScrubInView", opts.scrubEnable ? false : true).toBool());

    settings.endGroup();

    settings.beginGroup("General");

    opts.fontSize1 = settings.value("fontSize1", opts.fontSize1).toInt();
    opts.fontSize2 = settings.value("fontSize2", opts.fontSize2).toInt();

    opts.playMode         = settings.value("playMode", opts.playMode).toInt();
    opts.play             = int(settings.value("playOnStartup", opts.play ? true : false).toBool());
    opts.networkOnStartup = int(settings.value("networkOnStartup", opts.networkOnStartup ? true : false).toBool());
    opts.stylusAsMouse    = int(settings.value("stylusAsMouse", opts.stylusAsMouse ? true : false).toBool());
    opts.startupResize    = int(settings.value("startupResize", opts.startupResize ? true : false).toBool());
    opts.qtdesktop        = int(settings.value("desktopAware", opts.qtdesktop ? true : false).toBool());
    opts.urlsReuseSession = int(settings.value("urlsReuseSession", opts.urlsReuseSession ? true : false).toBool());
    opts.fullscreen       = int(settings.value("fullscreenOnStartup", opts.fullscreen ? true : false).toBool());
    opts.screen           = settings.value("startupScreenPolicy", opts.screen).toInt();
    opts.nomb             = int(settings.value("noMenuBar", opts.nomb ? true : false).toBool());
    opts.defaultfps       = settings.value("fps", opts.defaultfps).toDouble();
    opts.networkHostBuf   = settings.value("networkHost", (opts.networkHost) ? opts.networkHost : "").toString().toUtf8().data();
    opts.networkHost      = (char *) ((opts.networkHostBuf.empty()) ? 0 : opts.networkHostBuf.c_str());
    opts.readerThreads    = settings.value("readerThreads", opts.readerThreads).toInt();
    opts.autoRetime       = int(settings.value("autoRetime", opts.autoRetime ? true : false).toBool());
    opts.useCrashReporter = int(settings.value("useCrashReporter", opts.useCrashReporter ? true : false).toBool());

    opts.stereoMode = 0;
    s = settings.value("stereoMode", opts.stereoMode).toString();
    if (s == StereoAnaglyph) opts.stereoMode = (char*)StereoAnaglyph;
    if (s == StereoLumAnaglyph) opts.stereoMode = (char*)StereoLumAnaglyph;
    if (s == StereoPair) opts.stereoMode = (char*)StereoPair;
    if (s == StereoMirror) opts.stereoMode = (char*)StereoMirror;
    if (s == StereoChecker) opts.stereoMode = (char*)StereoChecker;
    if (s == StereoScanline) opts.stereoMode = (char*)StereoScanline;
    if (s == StereoHardware) opts.stereoMode = (char*)StereoHardware;
    if (s == StereoLeft) opts.stereoMode = (char*)StereoLeft;
    if (s == StereoRight) opts.stereoMode = (char*)StereoRight;

    settings.endGroup();

    //----------------------------------------------------------------------
    settings.beginGroup("Caching");
    int cacheMode     = settings.value("cacheMode", 2 /*Look-Ahead Cache*/).toInt();

    opts.useCache = false;
    opts.useLCache = false;

    switch (cacheMode)
    {
      default:
      case 0:
          break;
      case 1:
          opts.useCache = true;
          break;
      case 2:
          opts.useLCache = true;
          break;
    }

    // Look for architecture specific preferences first, then fall back to non-specific  
    double tmplram = -1;
    double tmpcram = -1;
    
    #ifdef ARCH_IA32
        tmplram = settings.value("lookAheadCacheSize32New", tmplram).toDouble();
        tmpcram = settings.value("regionCacheSize32New", tmpcram).toDouble();
    #else
        tmplram = settings.value("lookAheadCacheSize64New", tmplram).toDouble();
        tmpcram = settings.value("regionCacheSize64New", tmpcram).toDouble();
    #endif

    if (tmplram == -1) opts.maxlram = settings.value("lookAheadCacheSizeNew", opts.maxlram).toDouble();
    else               opts.maxlram = tmplram;
    if (tmpcram == -1) opts.maxcram = settings.value("regionCacheSizeNew", opts.maxcram).toDouble();
    else               opts.maxcram = tmpcram;

    opts.maxbwait = settings.value("bufferWait", opts.maxbwait).toDouble();
    opts.lookback = settings.value("lookBehindFraction", opts.lookback).toDouble();
    opts.cacheOutsideRegion = settings.value("cacheOutsideRegion", opts.cacheOutsideRegion).toBool();

    settings.endGroup();

    //----------------------------------------------------------------------
    settings.beginGroup("Rendering");
    opts.nofloat = settings.value("nofloat", opts.nofloat).toBool();
    opts.swapScanlines = settings.value("swapScanlines", opts.swapScanlines).toBool();
    opts.prefetch = settings.value("prefetch2", opts.prefetch).toBool();
    opts.useAppleClientStorage = settings.value("appleClientStorage", opts.useAppleClientStorage).toBool();
    opts.newGLSLlutInterp = settings.value("newGLSLlutInterp", opts.newGLSLlutInterp).toBool();

    //  Can't check ImageRenderer::queryThreadedUpload() here because this
    //  function is called very early (before video modules are created).  If
    //  threaded uploads are not allowed, later code will ensure the checkbox
    //  is disabled, etc.  But opts struct should represent state in settings
    //  file.
    //
    
    opts.useThreadedUpload = settings.value("useThreadedUpload3", opts.useThreadedUpload).toBool();
    
    opts.maxvram = settings.value("vram", opts.maxvram).toDouble();
    opts.maxbits = settings.value("maxBitDepth", opts.maxbits).toInt();

    s = settings.value("resampleMethod", opts.resampleMethod).toString();

    if (s == Area) opts.resampleMethod = (char*)Area;
    else if (s == Linear) opts.resampleMethod = (char*)Linear;
    else if (s == Cubic) opts.resampleMethod = (char*)Cubic;
    else if (s == Nearest) opts.resampleMethod = (char*)Nearest;

    s = settings.value("imageFilter", opts.imageFilter).toString();

    if (s == Linear)       opts.imageFilter = GL_LINEAR;
    else if (s == Nearest) opts.imageFilter = GL_NEAREST;

    s = settings.value("backgroundPattern",
                       (opts.bgpattern ? opts.bgpattern : "black")).toString();

    if      (s == "black") opts.bgpattern = (char*)Solid0;
    else if (s == "white")      opts.bgpattern = (char*)Solid100;
    else if (s == "grey18") opts.bgpattern = (char*)Solid18;
    else if (s == "grey50") opts.bgpattern = (char*)Solid50;
    else if (s == "checker") opts.bgpattern = (char*)Checker;
    else if (s == "crosshatch") opts.bgpattern = (char*)CrossHatch;

    settings.endGroup();

    //----------------------------------------------------------------------
    settings.beginGroup("Audio");
    opts.volume = settings.value("volume", opts.volume).toDouble();

    string audioDevice = (opts.audioDevice) ? opts.audioDevice : "";
    audioDevice = UTF8::qconvert(settings.value("outputDevice", QVariant(UTF8::qconvert(audioDevice.c_str()))).toString());
    opts.audioDevice = strdup(audioDevice.c_str());

    string audioModule = (opts.audioModule) ? opts.audioModule : "";
    audioModule = UTF8::qconvert(settings.value("outputModule", QVariant(UTF8::qconvert(audioModule.c_str()))).toString());
    opts.audioModule = strdup(audioModule.c_str());

    opts.audioNice = int(!settings.value("holdDeviceOpen", !opts.audioNice).toBool());
    opts.aframesize = settings.value("devicePacketSize", opts.aframesize).toInt();
    opts.acachesize = settings.value("cachePacketSizeNew", opts.acachesize).toInt();
    opts.audioMinCache = settings.value("audioMinCache", opts.audioMinCache).toDouble();
    opts.audioMaxCache = settings.value("audioMaxCache", opts.audioMaxCache).toDouble();
    opts.audioNoLock = int(!settings.value("hardwareLock", !opts.audioNoLock).toBool());
    opts.audioPrecision = settings.value("outputPrecision", opts.audioPrecision).toInt();
    opts.audioLayout = settings.value("outputLayout", opts.audioLayout).toInt();
    opts.audioRate = settings.value("outputRate", opts.audioRate).toInt();
    opts.audioGlobalOffset = settings.value("globalOffset", opts.audioGlobalOffset).toDouble();
    opts.audioDeviceLatency = settings.value("audioDeviceLatency", opts.audioDeviceLatency).toDouble();
    opts.audioScrub = int(settings.value("audioScrub", opts.audioScrub).toBool());
    opts.audioPreRoll = int(settings.value("audioPreRoll", opts.audioPreRoll).toBool());
    settings.endGroup();

    //----------------------------------------------------------------------
    settings.beginGroup("Display");
    opts.gamma         = settings.value("gamma", opts.gamma).toDouble();
    opts.brightness    = settings.value("brightness", opts.brightness).toDouble();
    opts.vsync         = settings.value("vsync", opts.vsync).toInt();
    opts.dispRedBits   = settings.value("dispRedBits", opts.dispRedBits).toInt();
    opts.dispGreenBits = settings.value("dispBlueBits", opts.dispGreenBits).toInt();
    opts.dispBlueBits  = settings.value("dispGreenBits", opts.dispBlueBits).toInt();
    opts.dispAlphaBits = settings.value("dispAlphaBits", opts.dispAlphaBits).toInt();
    settings.endGroup();

    //----------------------------------------------------------------------
    settings.beginGroup("OpenEXR");
    opts.exrRGBA = settings.value("useRGBA", bool(opts.exrRGBA)).toBool();
    opts.exrInherit = settings.value("inheritChannels", bool(opts.exrInherit)).toBool();
    opts.exrPlanar3Chan = settings.value("planar3Channel", bool(opts.exrPlanar3Chan)).toBool();
    opts.exrNoOneChannel = settings.value("noOneChannel", bool(opts.exrNoOneChannel)).toBool();
    opts.exrcpus = settings.value("cpus", opts.exrcpus).toInt();
    opts.exrIOMethod = settings.value("IOmethod", opts.exrIOMethod).toInt();
    opts.exrIOSize = settings.value("IOsize", opts.exrIOSize).toInt();
    opts.exrMaxAsync = settings.value("MaxInFlight", opts.exrMaxAsync).toInt();
    opts.exrReadWindowIsDisplayWindow = settings.value("readWindowIsDisplayWindow", bool(opts.exrReadWindowIsDisplayWindow)).toBool();
    opts.exrReadWindow = settings.value("readWindow", opts.exrReadWindow).toInt();
    settings.endGroup();

    //----------------------------------------------------------------------
    settings.beginGroup("JPEG");
    opts.jpegRGBA = int(settings.value("RGBA", bool(opts.jpegRGBA)).toBool());
    opts.jpegIOMethod = settings.value("IOmethod", opts.jpegIOMethod).toInt();
    opts.jpegIOSize = settings.value("IOsize", opts.jpegIOSize).toInt();
    opts.jpegMaxAsync = settings.value("MaxInFlight", opts.jpegMaxAsync).toInt();
    settings.endGroup();

    //----------------------------------------------------------------------
    settings.beginGroup("Cineon");
    opts.cinIOMethod = settings.value("IOmethod", opts.cinIOMethod).toInt();
    opts.cinIOSize = settings.value("IOsize", opts.cinIOSize).toInt();
    opts.cinMaxAsync = settings.value("MaxInFlight", opts.cinMaxAsync).toInt();
    opts.cinchroma = int(settings.value("usePrimaries", bool(opts.cinchroma)).toBool());
    
    QString cinpf = settings.value("pixelFormatNew", opts.cinPixel).toString();

    if      (cinpf == RGB8)         opts.cinPixel = (char*)RGB8;
    else if (cinpf == RGBA8)        opts.cinPixel = (char*)RGBA8;
    else if (cinpf == RGB8_PLANAR)  opts.cinPixel = (char*)RGB8_PLANAR;
    else if (cinpf == RGB10_A2)     opts.cinPixel = (char*)RGB10_A2;
    else if (cinpf == A2_BGR10)     opts.cinPixel = (char*)A2_BGR10;
    else if (cinpf == RGB16)        opts.cinPixel = (char*)RGB16;
    else if (cinpf == RGBA16)       opts.cinPixel = (char*)RGBA16;
    else if (cinpf == RGB16_PLANAR) opts.cinPixel = (char*)RGB16_PLANAR;

    settings.endGroup();

    //----------------------------------------------------------------------
    settings.beginGroup("DPX");
    opts.dpxIOMethod = settings.value("IOmethod", opts.dpxIOMethod).toInt();
    opts.dpxIOSize = settings.value("IOsize", opts.dpxIOSize).toInt();
    opts.dpxMaxAsync = settings.value("MaxInFlight", opts.dpxMaxAsync).toInt();
    opts.dpxchroma = int(settings.value("usePrimaries", bool(opts.dpxchroma)).toBool());

    QString dpxpf = settings.value("pixelFormatNew", opts.dpxPixel).toString();

    if      (dpxpf == RGB8)         opts.dpxPixel = (char*)RGB8;
    else if (dpxpf == RGBA8)        opts.dpxPixel = (char*)RGBA8;
    else if (dpxpf == RGB8_PLANAR)  opts.dpxPixel = (char*)RGB8_PLANAR;
    else if (dpxpf == RGB10_A2)     opts.dpxPixel = (char*)RGB10_A2;
    else if (dpxpf == A2_BGR10)     opts.dpxPixel = (char*)A2_BGR10;
    else if (dpxpf == RGB16)        opts.dpxPixel = (char*)RGB16;
    else if (dpxpf == RGBA16)       opts.dpxPixel = (char*)RGBA16;
    else if (dpxpf == RGB16_PLANAR) opts.dpxPixel = (char*)RGB16_PLANAR;

    settings.endGroup();


    //----------------------------------------------------------------------
    settings.beginGroup("TGA");
    opts.tgaIOMethod = settings.value("IOmethod", opts.tgaIOMethod).toInt();
    opts.tgaIOSize = settings.value("IOsize", opts.tgaIOSize).toInt();
    opts.tgaMaxAsync = settings.value("MaxInFlight", opts.tgaMaxAsync).toInt();
    settings.endGroup();

    //----------------------------------------------------------------------
    settings.beginGroup("TIFF");
    opts.tiffIOMethod = settings.value("IOmethod", opts.tiffIOMethod).toInt();
    opts.tiffIOSize = settings.value("IOsize", opts.tiffIOSize).toInt();
    opts.tiffMaxAsync = settings.value("MaxInFlight", opts.tiffMaxAsync).toInt();
    settings.endGroup();

    opts.exportIOEnvVars();


    //----------------------------------------------------------------------
    settings.beginGroup("Video");
    QString pdev = settings.value("presentationDevice", UTF8::qconvert(opts.presentDevice)).toString();
    settings.endGroup();

    QStringList parts = pdev.split("/");

    if (parts.size() == 2)
    {
        // ALLOWED LEAKING! WINDOWS BUG
        //if (strcmp(opts.presentDevice, "")) free(opts.presentDevice);
        opts.presentDevice = strdup(pdev.toUtf8().constData());
    }
}

void 
RvPreferences::closeEvent(QCloseEvent* event)
{
    QWidget::closeEvent(event);
    //
    //  If we've never updated, the values in the dialog are wrong, 
    //  so don't write, and don't tell mu clients to write.
    //
    if (m_updated) 
    {
        write();

        const TwkApp::Application::Documents& docs = RvApp()->documents();

        for (size_t i = 0; i < docs.size(); i++)
        {
            Rv::Session* s = static_cast<Rv::Session*>(docs[i]);
            s->userGenericEvent("preferences-hide", "");
        }
    }
}

void
RvPreferences::write()
{
    Options& opts = Options::sharedOptions();
    RV_QSETTINGS;

    settings.beginGroup("Controls");
    settings.setValue("disableClickToPlay", m_ui.clickToPlayEnableToggle->checkState() != Qt::Checked);
    settings.setValue("disableScrubInView", m_ui.scrubEnableToggle->checkState() != Qt::Checked);
    settings.endGroup();

    settings.beginGroup("General");
    settings.setValue("playOnStartup", m_ui.playOnStartupToggle->checkState() == Qt::Checked);
    settings.setValue("networkOnStartup", m_ui.networkToggle->checkState() == Qt::Checked);
    settings.setValue("stylusAsMouse", m_ui.stylusAsMouseToggle->checkState() == Qt::Checked);
    settings.setValue("startupResize", m_ui.startupResizeToggle->checkState() == Qt::Checked);
    settings.setValue("desktopAware", m_ui.desktopAwareToggle->checkState() == Qt::Checked);
    settings.setValue("urlsReuseSession", m_ui.reuseSessionToggle->checkState() == Qt::Checked);
    settings.setValue("playMode", m_ui.playbackModeCombo->currentIndex());
    settings.setValue("noMenuBar", m_ui.noMenuBarToggle->checkState() == Qt::Checked);
    settings.setValue("fullscreenOnStartup", m_ui.fullscreenOnStartupToggle->checkState() == Qt::Checked);
    settings.setValue("startupScreenPolicy", m_ui.startupScreenCombo->currentIndex()-1);
    settings.setValue("fps", m_ui.fpsEdit->text().toDouble());
    settings.setValue("networkHost", m_ui.networkHostEdit->text());
    settings.setValue("readerThreads", m_ui.rthreadEdit->text().toInt());
    settings.setValue("autoRetime", m_ui.autoRetimeToggle->checkState() == Qt::Checked);
    settings.setValue("useCrashReporter", m_ui.useCrashReporterToggle->checkState() == Qt::Checked);
    settings.setValue("fontSize1", m_ui.fontSizeSpinBox->value());
    settings.setValue("fontSize2", m_ui.fontSize2SpinBox->value());

    char* stereo = 0;
    switch (m_ui.stereoModeCombo->currentIndex())
    {
      default: break;
      case 1: stereo = (char*)StereoAnaglyph; break;
      case 2: stereo = (char*)StereoPair; break;
      case 3: stereo = (char*)StereoMirror; break;
      case 4: stereo = (char*)StereoLeft; break;
      case 5: stereo = (char*)StereoRight; break;
      case 6: stereo = (char*)StereoChecker; break;
      case 7: stereo = (char*)StereoScanline; break;
      case 8: stereo = (char*)StereoHardware; break;
      case 9: stereo = (char*)StereoLumAnaglyph; break;
    }

    settings.setValue("stereoMode", UTF8::qconvert(stereo));

    settings.endGroup();

    //----------------------------------------------------------------------

    settings.beginGroup("Caching");
    settings.setValue("cacheMode", m_ui.cacheModeCombo->currentIndex());

    // Use the setting from the running architecture as the shared setting for 
    // backwards compatibility.
    #ifdef ARCH_IA32
        settings.setValue("lookAheadCacheSizeNew", m_ui.lookAheadCacheSize32Edit->text().toDouble());
        settings.setValue("regionCacheSizeNew", m_ui.regionCacheSize32Edit->text().toDouble());
    #else
        settings.setValue("lookAheadCacheSizeNew", m_ui.lookAheadCacheSize64Edit->text().toDouble());
        settings.setValue("regionCacheSizeNew", m_ui.regionCacheSize64Edit->text().toDouble());
    #endif

    // Set the architecture specific settings
    //settings.setValue("lookAheadCacheSize32New", m_ui.lookAheadCacheSize32Edit->text().toDouble());
    settings.setValue("lookAheadCacheSize64New", m_ui.lookAheadCacheSize64Edit->text().toDouble());
    //settings.setValue("regionCacheSize32New", m_ui.regionCacheSize32Edit->text().toDouble());
    settings.setValue("regionCacheSize64New", m_ui.regionCacheSize64Edit->text().toDouble());

    settings.setValue("bufferWait", m_ui.bufferWaitEdit->text().toDouble());
    settings.setValue("lookBehindFraction", m_ui.lookBehindFracEdit->text().toDouble());
    settings.setValue("cacheOutsideRegion", m_ui.cacheOutsideRegionToggle->checkState() == Qt::Checked);
    settings.endGroup();

    //----------------------------------------------------------------------

    settings.beginGroup("Rendering");

    settings.setValue("nofloat", m_ui.allowFloatToggle->checkState() != Qt::Checked);
    settings.setValue("swapScanlines", m_ui.swapScanlinesToggle->checkState() == Qt::Checked);
    settings.setValue("prefetch2", m_ui.prefetchToggle->checkState() == Qt::Checked);
    settings.setValue("appleClientStorage", m_ui.appleClientStorageToggle->checkState() == Qt::Checked);
    settings.setValue("newGLSLlutInterp", m_ui.newGLSLlutInterpToggle->checkState() == Qt::Checked);

    //  Don't write the value of the toggle as a setting unless the user was
    //  actually allowed to change it.
    //  
    if (ImageRenderer::queryThreadedUpload())
    {
        settings.setValue("useThreadedUpload3", m_ui.useThreadedUploadToggle->checkState() == Qt::Checked);
    }
    
    int n = 32;
    switch (m_ui.bitDepthCombo->currentIndex())
    {
      default:
      case 0: n = 32; break;
      case 1: n = 16; break;
      case 2: n = 8; break;
    }

    settings.setValue("maxBitDepth", n);

    char* method = 0;

    switch (m_ui.resampleMethodCombo->currentIndex())
    {
      default:
      case 0: method = (char*)Area; break;
      case 1: method = (char*)Cubic; break;
      case 2: method = (char*)Linear; break;
      case 3: method = (char*)Nearest; break;
    }

    settings.setValue("resampleMethod", method);

    switch (m_ui.imageFilterCombo->currentIndex())
    {
      default:
      case 0: method = (char*)Linear; break;
      case 1: method = (char*)Nearest; break;
    }

    settings.setValue("imageFilter", method);

    switch (m_ui.bgPatternCombo->currentIndex())
    {
      default:
      case 0: method = (char*)Solid0; break;
      case 1: method = (char*)Solid18; break;
      case 2: method = (char*)Solid50; break;
      case 3: method = (char*)Solid100; break;
      case 4: method = (char*)Checker; break;
      case 5: method = (char*)CrossHatch; break;
    }

    settings.setValue("backgroundPattern", method);

    settings.endGroup();

    //----------------------------------------------------------------------

    settings.beginGroup("Audio");
    settings.setValue("volume", m_ui.volumeSlider->sliderPosition() / 100.0);
    settings.setValue("holdDeviceOpen", m_ui.audioHoldOpenToggle->checkState() == Qt::Checked);
    settings.setValue("hardwareLock", m_ui.audioHardwareLockToggle->checkState() == Qt::Checked);
    settings.setValue("audioScrub", m_ui.audioScrubAtLaunchToggle->checkState() == Qt::Checked);
    settings.setValue("audioPreRoll", m_ui.audioPreRollToggle->checkState() == Qt::Checked);
    settings.setValue("devicePacketSize", m_ui.audioDevicePacketEdit->text().toInt());
    settings.setValue("cachePacketSizeNew", m_ui.audioCachePacketEdit->text().toInt());
    settings.setValue("audioMinCache", m_ui.audioCacheMinEdit->text().toDouble());
    settings.setValue("audioMaxCache", m_ui.audioCacheMaxEdit->text().toDouble());
    settings.setValue("globalOffset", m_ui.audioGlobalOffsetEdit->text().toDouble());
    settings.setValue("audioDeviceLatency", m_ui.audioDeviceLatencyEdit->text().toDouble());

    if (!AudioRenderer::audioDisabledAlways() && 
            m_ui.audioDeviceLayoutCombo->currentText() != "Unavailable" &&
            m_ui.audioDeviceLayoutCombo->currentText() != "")
    {
        settings.setValue("outputRate", m_ui.audioDeviceRateCombo->currentText().toDouble());
        settings.setValue("outputModule", m_ui.audioModuleCombo->currentText());
        settings.setValue("outputDevice", m_ui.audioDeviceCombo->currentText());

        // Handle outputPrecision settings
        int currentIndex = m_ui.audioDeviceFormatCombo->currentIndex();
        TwkAudio::Format currentFormat;

        if (currentIndex < m_ui.audioDeviceFormatCombo->count())
        {
            currentFormat = (TwkAudio::Format) m_ui.audioDeviceFormatCombo->itemData(currentIndex).toInt();
        }

        int prec;
        switch (currentFormat)
        {
            default:
            case TwkAudio::Float32Format: prec = 32; break;
            case TwkAudio::Int24Format: prec = 24; break;
            case TwkAudio::Int16Format: prec = 16; break;
            case TwkAudio::Int8Format: prec = 8; break;
            case TwkAudio::Int32Format: prec = -32 ; break;
        }

        settings.setValue("outputPrecision", prec);

        // Handle outputLayout settings
        currentIndex = m_ui.audioDeviceLayoutCombo->currentIndex();
        TwkAudio::Layout currentChannelLayout;

        if (currentIndex < m_ui.audioDeviceLayoutCombo->count())
        {
            currentChannelLayout = (TwkAudio::Layout) m_ui.audioDeviceLayoutCombo->itemData(currentIndex).toInt();
        }
        settings.setValue("outputLayout", int(currentChannelLayout));
    }

    settings.endGroup();

    //----------------------------------------------------------------------

    settings.beginGroup("Display");
    settings.setValue("vsync", m_ui.displayVideoSyncButton->checkState() == Qt::Checked ? 1 : 0);

    int rbits = 0, gbits = 0, bbits = 0, abits = 0;

    switch (m_ui.displayOutputCombo->currentIndex())
    {
      default:
      case 0: abits = bbits = gbits = rbits = 0; break;
      case 1: abits = bbits = gbits = rbits = 8; break;
      case 2: bbits = gbits = rbits = 10; abits = 2; break;
          break;
    }
    
    settings.setValue("dispRedBits", rbits);
    settings.setValue("dispGreenBits", gbits);
    settings.setValue("dispBlueBits", bbits);
    settings.setValue("dispAlphaBits", abits);
    settings.endGroup();

    //----------------------------------------------------------------------

    settings.beginGroup("OpenEXR");
    settings.setValue("useRGBA", m_ui.exrRGBAToggle->checkState() == Qt::Checked);
    settings.setValue("inheritChannels", m_ui.exrInheritToggle->checkState() == Qt::Checked);
    settings.setValue("planar3Channel", m_ui.exrPlanar3ChannelToggle->checkState() == Qt::Checked);
    settings.setValue("noOneChannel", m_ui.exrNoOneChannelToggle->checkState() == Qt::Checked);
    settings.setValue("readWindowIsDisplayWindow", m_ui.exrReadWindowIsDisplayWindowToggle->checkState() == Qt::Checked);
    settings.setValue("cpus", m_ui.exrNumThreadsEdit->text().toInt());
    settings.setValue("IOmethod", m_ui.exrIOMethodCombo->currentIndex());
    settings.setValue("readWindow", m_ui.exrReadWindowCombo->currentIndex());
    settings.setValue("IOsize", m_ui.exrChunkSizeEdit->text().toInt());
    settings.setValue("MaxInFlight", m_ui.exrMaxInFlightEdit->text().toInt());
    settings.endGroup();

    //----------------------------------------------------------------------

    settings.beginGroup("JPEG");
    settings.setValue("RGBA", m_ui.jpegRGBAToggle->checkState() == Qt::Checked);
    settings.setValue("IOmethod", m_ui.jpegIOMethodCombo->currentIndex());
    settings.setValue("IOsize", m_ui.jpegChunkSizeEdit->text().toInt());
    settings.setValue("MaxInFlight", m_ui.jpegMaxInFlightEdit->text().toInt());
    settings.endGroup();

    //----------------------------------------------------------------------

    settings.beginGroup("Cineon");
    settings.setValue("IOmethod", m_ui.cinIOMethodCombo->currentIndex());
    settings.setValue("IOsize", m_ui.cinChunkSizeEdit->text().toInt());
    settings.setValue("MaxInFlight", m_ui.cinMaxInFlightEdit->text().toInt());
    settings.setValue("usePrimaries", m_ui.cinChromaToggle->checkState() == Qt::Checked);

    const char* cinfmt = opts.cinPixel;

    switch (m_ui.cinDisplayPixelCombo->currentIndex())
    {
      case 0: cinfmt = RGB8;         break;
      case 1: cinfmt = RGBA8;        break;
      case 2: cinfmt = RGB8_PLANAR;  break;
      case 3: cinfmt = RGB10_A2;     break;
      case 4: cinfmt = A2_BGR10;     break;
      case 5: cinfmt = RGB16;        break;
      case 6: cinfmt = RGBA16;       break;
      case 7: cinfmt = RGB16_PLANAR; break;
    }

    settings.setValue("pixelFormatNew", UTF8::qconvert(cinfmt));
    settings.endGroup();

    //----------------------------------------------------------------------

    settings.beginGroup("DPX");
    settings.setValue("IOmethod", m_ui.dpxIOMethodCombo->currentIndex());
    settings.setValue("IOsize", m_ui.dpxChunkSizeEdit->text().toInt());
    settings.setValue("MaxInFlight", m_ui.dpxMaxInFlightEdit->text().toInt());
    settings.setValue("usePrimaries", m_ui.dpxChromaToggle->checkState() == Qt::Checked);

    const char* dpxfmt = opts.dpxPixel;

    switch (m_ui.dpxDisplayPixelCombo->currentIndex())
    {
      case 0: dpxfmt = RGB8;         break;
      case 1: dpxfmt = RGBA8;        break;
      case 2: dpxfmt = RGB8_PLANAR;  break;
      case 3: dpxfmt = RGB10_A2;     break;
      case 4: dpxfmt = A2_BGR10;     break;
      case 5: dpxfmt = RGB16;        break;
      case 6: dpxfmt = RGBA16;       break;
      case 7: dpxfmt = RGB16_PLANAR; break;
    }

    settings.setValue("pixelFormatNew", UTF8::qconvert(dpxfmt));
    settings.endGroup();

    //----------------------------------------------------------------------
    settings.beginGroup("TGA");
    settings.setValue("IOmethod", m_ui.tgaIOMethodCombo->currentIndex());
    settings.setValue("IOsize", m_ui.tgaChunkSizeEdit->text().toInt());
    settings.setValue("MaxInFlight", m_ui.tgaMaxInFlightEdit->text().toInt());
    settings.endGroup();

    settings.beginGroup("TIFF");
    settings.setValue("IOmethod", m_ui.tifIOMethodCombo->currentIndex());
    settings.setValue("IOsize", m_ui.tifChunkSizeEdit->text().toInt());
    settings.setValue("MaxInFlight", m_ui.tifMaxInFlightEdit->text().toInt());
    settings.endGroup();

    //
    //  Ensure that settings changes are written to disk.
    //
    settings.sync();

    const TwkApp::Application::Documents& docs = RvApp()->documents();

    for (size_t i = 0; i < docs.size(); i++)
    {
        Rv::Session* s = static_cast<Rv::Session*>(docs[i]);
        s->userGenericEvent("after-preferences-write", "");
    }
}

//----------------------------------------------------------------------
//
//  General
//


void 
RvPreferences::updateStyleSheet()
{
    Options& opts = Options::sharedOptions();

    if (!opts.qtcss)
    {
#ifdef PLATFORM_DARWIN
        QString csstext = UTF8::qconvert(rv_mac_dark).arg(opts.fontSize1).arg(opts.fontSize2);
#else
        QString csstext = UTF8::qconvert(rv_linux_dark).arg(opts.fontSize1).arg(opts.fontSize2).arg(opts.fontSize2-1);
#endif
        qApp->setStyleSheet(csstext);
    }
}

void 
RvPreferences::stylusAsMouseChanged(int state)
{
    Options::sharedOptions().stylusAsMouse = (state == 0) ? 0 : 1;
}

void 
RvPreferences::startupResizeChanged(int state)
{
    Options::sharedOptions().startupResize = (state == 0) ? 0 : 1;
}

//----------------------------------------------------------------------
//
//  Caching slots
//

static void
newCacheSize(float gigs, IPGraph::CachingMode m)
{
    const TwkApp::Application::Documents& docs = IPCore::App()->documents();

    //
    //  Don't bother setting it to something impossible on a 32-bit
    //  build, and don't set it to 0.0, since something will surely
    //  break.
    //
    if (4 == sizeof(void *) && gigs > 3.5) gigs = 3.5;
    if (gigs < 0.01) gigs = 0.01;

    size_t bytes = size_t(double(gigs) * 1024.0 * 1024.0 * 1024.0);

    if (m == IPGraph::BufferCache)
    {
        Options::sharedOptions().maxlram = gigs;
        Session::setMaxBufferCacheSize(bytes);
    }
    else
    {
        Options::sharedOptions().maxcram = gigs;
        Session::setMaxGreedyCacheSize(bytes);
    }

    for (size_t i = 0; i < docs.size(); i++)
    {
        Session* s = static_cast<Session*>(docs[i]);
        s->graph().setCacheModeSize(m, bytes); 
        s->updateGraphInOut();
    }
}

void 
RvPreferences::lookaheadCacheSizeFinisihed()
{
    float gigs = (dynamic_cast <QLineEdit *> (QObject::sender()))->text().toFloat();
    newCacheSize(gigs, IPGraph::BufferCache);
}

void 
RvPreferences::regionCacheSizeFinisihed()
{
    float gigs = (dynamic_cast <QLineEdit *> (QObject::sender()))->text().toFloat();
    newCacheSize(gigs, IPGraph::GreedyCache);
}

void 
RvPreferences::bufferWaitFinished()
{
    float seconds = m_ui.bufferWaitEdit->text().toFloat();
    Session::setMaxBufferedWaitTime(seconds);
}

void 
RvPreferences::lookBehindFinished()
{
    float percent = m_ui.lookBehindFracEdit->text().toFloat();
    Options::sharedOptions().lookback = percent;
    Session::setCacheLookBehindFraction(percent);

    const TwkApp::Application::Documents& docs = IPCore::App()->documents();

    for (size_t i = 0; i < docs.size(); i++)
    {
        Session* s = static_cast<Session*>(docs[i]);
        s->graph().setLookBehindFraction (percent); 
    }
}

void 
RvPreferences::cacheOutsideRegionChanged(int state)
{
    IPCore::FBCache::setCacheOutsideRegion(state != 0);

    const TwkApp::Application::Documents& docs = IPCore::App()->documents();

    for (size_t i = 0; i < docs.size(); i++)
    {
        Session* s = static_cast<Session*>(docs[i]);
        s->updateGraphInOut();
    }
}

void 
RvPreferences::rthreadFinished()
{
}

//----------------------------------------------------------------------
//
//  Rendering
//

void 
RvPreferences::bitDepthChanged(int index)
{
    switch (index)
    {
      case 0: // 32
          FormatIPNode::defaultBitDepth = 32;
          break;
      case 1: // 16
          FormatIPNode::defaultBitDepth = 16;
          break;
      case 2: // 8
          FormatIPNode::defaultBitDepth = 8;
          break;
    }
}

void 
RvPreferences::allowFloatChanged(int state)
{
    FormatIPNode::defaultAllowFP = state != 0;
}

void 
RvPreferences::newGLSLlutInterpChanged(int state)
{
    LUTIPNode::newGLSLlutInterp = (state != 0);
}

void 
RvPreferences::swapScanlinesChanged(int state)
{
    DisplayStereoIPNode::setSwapScanlines((state == Qt::Checked));
}

void 
RvPreferences::prefetchChanged(int state)
{
    const TwkApp::Application::Documents& docs = IPCore::App()->documents();

    for (size_t i = 0; i < docs.size(); i++)
    {
        Session* s = static_cast<Session*>(docs[i]);
        s->setUsePreEval(state != 0);
    }
}

void 
RvPreferences::appleClientStorageChanged(int state)
{
    ImageRenderer::setUseAppleClientStorage(state != 0);
    const TwkApp::Application::Documents& docs = IPCore::App()->documents();

    for (size_t i = 0; i < docs.size(); i++)
    {
        Session* s = static_cast<Session*>(docs[i]);
        s->clearVideoDeviceCaches();
    }
}

void
RvPreferences::useThreadedUploadChanged(int state)
{
    const TwkApp::Application::Documents& docs = IPCore::App()->documents();

    for (size_t i = 0; i < docs.size(); i++)
    {
        Session* s = static_cast<Session*>(docs[i]);
        s->renderer()->setUseThreadedUpload(state != 0);
    }
}

void 
RvPreferences::videoSyncChanged(int state)
{
    const TwkApp::Application::Documents& docs = IPCore::App()->documents();

    for (size_t i = 0; i < docs.size(); i++)
    {
        Session* s = static_cast<Session*>(docs[i]);
        if (RvDocument* rvdoc = reinterpret_cast<RvDocument*>(s->opaquePointer()))
        {
            rvdoc->setVSync(state != 0);
        }
    }
}

void
RvPreferences::displayOutputChanged(int state)
{
    const TwkApp::Application::Documents& docs = IPCore::App()->documents();

    for (size_t i = 0; i < docs.size(); i++)
    {
        Session* s = static_cast<Session*>(docs[i]);
        if (RvDocument* rvdoc = reinterpret_cast<RvDocument*>(s->opaquePointer()))
        {
            RvDocument::DisplayOutputType t;

            switch (state)
            {
              default:
              case 0: t = RvDocument::OpenGL8888; break;
              case 1: t = RvDocument::OpenGL8888; break;
              case 2: t = RvDocument::OpenGL1010102; break;
            }

            rvdoc->setDisplayOutput(t);
        }
    }
}


//----------------------------------------------------------------------
//
//  Helper func to find plugin
//

typedef TwkFB::GenericIO::Plugins FBPlugins;
typedef TwkFB::IOexr IOexr;
typedef TwkFB::IOjpeg IOjpeg;
typedef TwkFB::IOcin IOcin;
typedef TwkFB::IOdpx IOdpx;
typedef TwkFB::IOtarga IOtarga;
typedef TwkFB::IOtiff IOtiff;
using namespace TwkFB;

static
StreamingFrameBufferIO* findPlugin (const string& name)
{
    const FBPlugins& plugs = TwkFB::GenericIO::allPlugins();

    for (FBPlugins::const_iterator i = plugs.begin();
         i != plugs.end();
         ++i)
    {
        if (StreamingFrameBufferIO* sio = dynamic_cast<StreamingFrameBufferIO*>(*i))
        {
            if (sio->identifier() == name) return sio;
        }
    }

    return 0;
}

//----------------------------------------------------------------------
//
//  EXR slots
//


void RvPreferences::exrRGBAChanged(int state)
{
    if (StreamingFrameBufferIO* exr = findPlugin("IOexr")) 
        exr->setBoolAttribute("rgbaOnly", state != 0);
}

void RvPreferences::exrInheritChanged(int state)
{
    if (StreamingFrameBufferIO* exr = findPlugin("IOexr")) 
        exr->setBoolAttribute("inheritChannels", state != 0);
}

void RvPreferences::exrPlanar3ChannelChanged(int state)
{
    if (StreamingFrameBufferIO* exr = findPlugin("IOexr")) 
        exr->setBoolAttribute("planar3Channel", state != 0);
}

void RvPreferences::exrNoOneChannelChanged(int state)
{
    if (StreamingFrameBufferIO* exr = findPlugin("IOexr")) 
        exr->setBoolAttribute("noOneChannelPlanes", state != 0);
}

void RvPreferences::exrReadWindowIsDisplayWindowChanged(int state)
{
    if (StreamingFrameBufferIO* exr = findPlugin("IOexr")) 
        exr->setBoolAttribute("readWindowIsDisplayWindow", state != 0);
}

void RvPreferences::exrNumThreadsFinished()
{
    if (m_ui.exrNumThreadsEdit->text() == "0")
    {
        Imf::setGlobalThreadCount(TwkUtil::SystemInfo::numCPUs() > 1 
                                  ? (TwkUtil::SystemInfo::numCPUs()-1) 
                                  : 1);
    }
    else
    {
        Imf::setGlobalThreadCount(m_ui.exrNumThreadsEdit->text().toInt());
    }
}

void
RvPreferences::exrAutoThreads(int state)
{
    if (state == Qt::Checked)
    {
        m_ui.exrNumThreadsEdit->setText("0");
        m_ui.exrNumThreadsEdit->setEnabled(false);
        m_ui.exrThreadsLabel->setEnabled(false);
        Imf::setGlobalThreadCount(TwkUtil::SystemInfo::numCPUs() > 1 
                                  ? (TwkUtil::SystemInfo::numCPUs()-1) 
                                  : 1);
    }
    else
    {
        m_ui.exrNumThreadsEdit->setEnabled(true);
        m_ui.exrThreadsLabel->setEnabled(true);
        QString s;
        s.setNum(Imf::globalThreadCount());
        m_ui.exrNumThreadsEdit->setText(s);
    }
}

void 
RvPreferences::exrThreadNumChanged(const QString& text)
{
    if (text == "0")
    {
        m_ui.exrAutoThreadsToggle->setCheckState(Qt::Checked);
    }
    else
    {
        m_ui.exrAutoThreadsToggle->setCheckState(Qt::Unchecked);
    }
}


void 
RvPreferences::exrChunkSizeFinished()
{
    if (StreamingFrameBufferIO* exr = findPlugin("IOexr")) 
        exr->setIntAttribute("iosize", m_ui.exrChunkSizeEdit->text().toInt());
}

void 
RvPreferences::exrMaxInFlightFinished()
{
    if (StreamingFrameBufferIO* exr = findPlugin("IOexr")) 
        exr->setIntAttribute("iomaxAsync", m_ui.exrMaxInFlightEdit->text().toInt());
}

void 
RvPreferences::exrIOMethodChanged(int state)
{
    if (StreamingFrameBufferIO* exr = findPlugin("IOexr")) 
        exr->setIntAttribute("iotype", (TwkFB::StreamingFrameBufferIO::IOType)
                             m_ui.exrIOMethodCombo->currentIndex());
}

void 
RvPreferences::exrReadWindowChanged(int state)
{
    if (StreamingFrameBufferIO* exr = findPlugin("IOexr")) 
        exr->setIntAttribute("readWindow", (TwkFB::IOexr::ReadWindow)
                             m_ui.exrReadWindowCombo->currentIndex());
}

//----------------------------------------------------------------------
//
//  Cineon
//

void 
RvPreferences::cinChunkSizeFinished()
{
    if (StreamingFrameBufferIO* exr = findPlugin("IOexr")) 
        exr->setIntAttribute("iosize", m_ui.exrChunkSizeEdit->text().toInt());
}

void 
RvPreferences::cinMaxInFlightFinished()
{
    if (StreamingFrameBufferIO* cin = findPlugin("IOcin")) 
        cin->setIntAttribute("iomaxAsync", m_ui.cinMaxInFlightEdit->text().toInt());
}

void 
RvPreferences::cinIOMethodChanged(int state)
{
    if (StreamingFrameBufferIO* cin = findPlugin("IOcin")) 
        cin->setIntAttribute("iotype", (TwkFB::StreamingFrameBufferIO::IOType)
                             m_ui.cinIOMethodCombo->currentIndex());
}

void 
RvPreferences::cinChromaChanged(int state)
{
    if (StreamingFrameBufferIO* cin = findPlugin("IOcin")) 
        cin->setBoolAttribute("useChromaticities", state != 0);
}

void 
RvPreferences::cinPixelsChanged(int index)
{
    if (StreamingFrameBufferIO* cin = findPlugin("IOcin")) 
    {
        switch (index)
        {
            case 0: cin->setIntAttribute("format", IOcin::RGB8);         break;
            case 1: cin->setIntAttribute("format", IOcin::RGBA8);        break;
            case 2: cin->setIntAttribute("format", IOcin::RGB8_PLANAR);  break;
            case 3: cin->setIntAttribute("format", IOcin::RGB10_A2);     break;
            case 4: cin->setIntAttribute("format", IOcin::A2_BGR10);     break;
            case 5: cin->setIntAttribute("format", IOcin::RGB16);        break;
            case 6: cin->setIntAttribute("format", IOcin::RGBA16);       break;
            case 7: cin->setIntAttribute("format", IOcin::RGB16_PLANAR); break;
        }
    }
}

//----------------------------------------------------------------------
//
//  DPX
//

void 
RvPreferences::dpxChunkSizeFinished()
{
    if (StreamingFrameBufferIO* dpx = findPlugin("IOdpx")) 
        dpx->setIntAttribute("iosize", m_ui.dpxChunkSizeEdit->text().toInt());
}

void 
RvPreferences::dpxMaxInFlightFinished()
{
    if (StreamingFrameBufferIO* dpx = findPlugin("IOdpx")) 
        dpx->setIntAttribute("iomaxAsync", m_ui.dpxMaxInFlightEdit->text().toInt());
}

void 
RvPreferences::dpxIOMethodChanged(int state)
{
    if (StreamingFrameBufferIO* dpx = findPlugin("IOdpx")) 
        dpx->setIntAttribute("iotype", (TwkFB::StreamingFrameBufferIO::IOType)
                             m_ui.dpxIOMethodCombo->currentIndex());
}

void 
RvPreferences::dpxChromaChanged(int state)
{
    if (StreamingFrameBufferIO* dpx = findPlugin("IOdpx")) 
        dpx->setBoolAttribute("useChromaticities", state != 0);
}

void 
RvPreferences::dpxPixelsChanged(int index)
{
    if (StreamingFrameBufferIO* dpx = findPlugin("IOdpx")) 
    {
        switch (index)
        {
            case 0: dpx->setIntAttribute("format", IOdpx::RGB8);         break;
            case 1: dpx->setIntAttribute("format", IOdpx::RGBA8);        break;
            case 2: dpx->setIntAttribute("format", IOdpx::RGB8_PLANAR);  break;
            case 3: dpx->setIntAttribute("format", IOdpx::RGB10_A2);     break;
            case 4: dpx->setIntAttribute("format", IOdpx::A2_BGR10);     break;
            case 5: dpx->setIntAttribute("format", IOdpx::RGB16);        break;
            case 6: dpx->setIntAttribute("format", IOdpx::RGBA16);       break;
            case 7: dpx->setIntAttribute("format", IOdpx::RGB16_PLANAR); break;
        }
    }
}

//----------------------------------------------------------------------
//
//  TGA
//

void 
RvPreferences::tgaChunkSizeFinished()
{
    if (StreamingFrameBufferIO* tga = findPlugin("IOtga")) 
        tga->setIntAttribute("iosize", m_ui.tgaChunkSizeEdit->text().toInt());
}

void 
RvPreferences::tgaMaxInFlightFinished()
{
    if (StreamingFrameBufferIO* tga = findPlugin("IOtga")) 
        tga->setIntAttribute("iomaxAsync", m_ui.tgaMaxInFlightEdit->text().toInt());
}

void 
RvPreferences::tgaIOMethodChanged(int state)
{
    if (StreamingFrameBufferIO* tga = findPlugin("IOtga")) 
        tga->setIntAttribute("iotype", (TwkFB::StreamingFrameBufferIO::IOType)
                             m_ui.tgaIOMethodCombo->currentIndex());
}

//----------------------------------------------------------------------
//
//  TIFF
//

void 
RvPreferences::tiffChunkSizeFinished()
{
    if (StreamingFrameBufferIO* tif = findPlugin("IOtif")) 
        tif->setIntAttribute("iosize", m_ui.tifChunkSizeEdit->text().toInt());
}

void 
RvPreferences::tiffMaxInFlightFinished()
{
    if (StreamingFrameBufferIO* tif = findPlugin("IOtif")) 
        tif->setIntAttribute("iomaxAsync", m_ui.tifMaxInFlightEdit->text().toInt());
}

void 
RvPreferences::tiffIOMethodChanged(int state)
{
    if (StreamingFrameBufferIO* tif = findPlugin("IOtif")) 
        tif->setIntAttribute("iotype", (TwkFB::StreamingFrameBufferIO::IOType)
                             m_ui.tifIOMethodCombo->currentIndex());
}

//----------------------------------------------------------------------
//
//  JPEG
//

void 
RvPreferences::jpegChunkSizeFinished()
{
    if (StreamingFrameBufferIO* jpeg = findPlugin("IOjpeg")) 
        jpeg->setIntAttribute("iosize", m_ui.jpegChunkSizeEdit->text().toInt());
}

void 
RvPreferences::jpegMaxInFlightFinished()
{
    if (StreamingFrameBufferIO* jpeg = findPlugin("IOjpeg")) 
        jpeg->setIntAttribute("iomaxAsync", m_ui.jpegMaxInFlightEdit->text().toInt());
}

void 
RvPreferences::jpegIOMethodChanged(int state)
{
    if (StreamingFrameBufferIO* jpeg = findPlugin("IOjpeg")) 
        jpeg->setIntAttribute("iotype", (TwkFB::StreamingFrameBufferIO::IOType)
                             m_ui.jpegIOMethodCombo->currentIndex());
}


void 
RvPreferences::jpegRGBAChanged(int state)
{
    //if (StreamingFrameBufferIO* jpeg = findPlugin("IOjpeg")) 
        //if (IOjpeg* jpeg = findPlugin<IOjpeg>())
            //{
                //jpeg->format(state != 0 ? IOjpeg::RGBA : IOjpeg::YUV);
            //}
}

//----------------------------------------------------------------------
//
//  Display Output 
//

//----------------------------------------------------------------------
//
//  Audio slots
//

bool 
RvPreferences::initAudioDeviceMenu(AudioRenderer::RendererParameters &params,
                                   const AudioRenderer::DeviceVector &devices,
                                   const string &currentDeviceName)
{
    m_ui.audioDeviceCombo->clear();

    if (devices.empty())
    {
        m_ui.audioDeviceCombo->addItem("Unavailable");
        m_ui.audioDeviceCombo->setItemIcon(0, colorAdjustedIcon(":images/mute_32x32.png"));
        m_ui.audioDeviceCombo->setCurrentIndex(0);
        m_ui.audioDeviceCombo->setEnabled(false);
        return false;
    }
    else
    {
        int current = 0;
        for (size_t i = 0; i < devices.size(); i++)
        {
            m_ui.audioDeviceCombo->addItem(QString::fromUtf8(devices[i].name.c_str()));
            if (devices[i].name == currentDeviceName) current = i;
        }

        m_ui.audioDeviceCombo->setCurrentIndex(current);
        m_ui.audioDeviceCombo->setEnabled(true);

        params.device = devices[current].name;
    }

    return true;
}


bool 
RvPreferences::initAudioLayoutMenu(AudioRenderer::RendererParameters &params,
                                   const AudioRenderer::LayoutsVector &layouts,
                                   const AudioRenderer::Layout &currentLayout)
{
    m_ui.audioDeviceLayoutCombo->clear();

    if (layouts.empty()) 
    {
        m_ui.audioDeviceLayoutCombo->addItem("Unavailable");
        m_ui.audioDeviceLayoutCombo->setItemIcon(0, colorAdjustedIcon(":images/mute_32x32.png"));
        m_ui.audioDeviceLayoutCombo->setCurrentIndex(0);
        m_ui.audioDeviceLayoutCombo->setEnabled(false);
        return false;
    }
    else
    {
        int current = 0;
        for (size_t i = 0; i < layouts.size(); i++)
        {
            m_ui.audioDeviceLayoutCombo->addItem(QString::fromUtf8(TwkAudio::layoutString(layouts[i]).c_str()), 
                                                 layouts[i]);
            if (layouts[i] == currentLayout) current = i;
        }

        m_ui.audioDeviceLayoutCombo->setEnabled(true);
        m_ui.audioDeviceLayoutCombo->setCurrentIndex(current);
        params.layout = layouts[current];
    }

    return true;
}


bool 
RvPreferences::initAudioFormatMenu(AudioRenderer::RendererParameters &params,
                                   const AudioRenderer::FormatVector &formats,
                                   const AudioRenderer::Format &currentFormat)
{
    m_ui.audioDeviceFormatCombo->clear();

    if (formats.empty()) 
    {
        m_ui.audioDeviceFormatCombo->addItem("Unavailable");
        m_ui.audioDeviceFormatCombo->setItemIcon(0, colorAdjustedIcon(":images/mute_32x32.png"));
        m_ui.audioDeviceFormatCombo->setCurrentIndex(0);
        m_ui.audioDeviceFormatCombo->setEnabled(false);
        return false;
    }
    else
    {
        int current = 0;
        for (size_t i = 0; i < formats.size(); i++)
        {
            m_ui.audioDeviceFormatCombo->addItem(QString::fromUtf8(TwkAudio::formatString(formats[i]).c_str()), 
                                                 formats[i]);
            if (formats[i] == currentFormat) current = i;
        }

        m_ui.audioDeviceFormatCombo->setEnabled(true);
        m_ui.audioDeviceFormatCombo->setCurrentIndex(current);
        params.format = formats[current];
    }

    return true;
}

bool 
RvPreferences::initAudioRatesMenu(AudioRenderer::RendererParameters &params,
                                   const AudioRenderer::RateVector &rates,
                                   const size_t &currentRate)
{
    m_ui.audioDeviceRateCombo->clear();

    if (rates.empty())
    {
        m_ui.audioDeviceRateCombo->clear();
        m_ui.audioDeviceRateCombo->addItem("Unavailable");
        m_ui.audioDeviceRateCombo->setItemIcon(0, colorAdjustedIcon(":images/mute_32x32.png"));
        m_ui.audioDeviceRateCombo->setCurrentIndex(0);
        m_ui.audioDeviceRateCombo->setEnabled(false);
        return false;
    }
    else 
    {
        int current = 0;
        for (size_t i = 0; i < rates.size(); i++)
        {
            QString s = QString("%1").arg((unsigned int)(rates[i]));
            m_ui.audioDeviceRateCombo->addItem(s, (unsigned int)rates[i]);

            if (rates[i] == currentRate) current = i;
        }

        m_ui.audioDeviceRateCombo->setEnabled(true);
        m_ui.audioDeviceRateCombo->setCurrentIndex(current);
        params.rate = rates[current];
    }

    return true;
}


void
RvPreferences::audioModuleChanged(int index)
{
    if (AudioRenderer::audioDisabledAlways()) return;

    if (index != -1)
    {
        try
        {
            string moduleChoice = m_ui.audioModuleCombo->currentText().toUtf8().constData();
            if (moduleChoice != AudioRenderer::currentModule() || m_ui.audioDeviceCombo->count() == 0)
            {
                // If we really have switched module then we must
                // force the device choice to "Default" since this is
                // the only choice that is common across all the audio module
                // choices.
                AudioRenderer::RendererParameters params = AudioRenderer::defaultParameters();
                params.device = "Default";
                AudioRenderer::setDefaultParameters(params);
                AudioRenderer::setModule(moduleChoice);
                AudioRenderer::setDefaultParameters(params); // Re-set cause the setModule() can change params values.
                                                             // when switching modules.
            }
        }
        catch (...)
        {
            /*
            cerr << "got exception from audiorenderer::setModule() " <<
                    m_ui.audioModuleCombo->currentText().toUtf8().constData() << endl;
            */
        }
    }

    AudioRenderer* renderer = 0;
    try { renderer = AudioRenderer::renderer(); }
    catch(...) { /* cerr << "renderer() threw" << endl; */ }

    if (!renderer) return;

    AudioRenderer::RendererParameters params = AudioRenderer::defaultParameters();

    //  If we've initialized a device, try to reuse it.
    //
    string deviceName = params.device;

    //
    //  If we haven't initialized a device, use the one from the
    //  settings file
    //
    Options& opts = Options::sharedOptions();
    if ((deviceName == "" || deviceName == "Default") && opts.audioDevice) deviceName = opts.audioDevice;
    int deviceIndex = renderer->findDeviceByName(deviceName);


    // Audio Device List setup
    //
    //  If we can't find a device by that name, use the default
    //  device
    //
    if (deviceIndex == -1) deviceIndex = renderer->findDefaultDevice();
    AudioRenderer::Device d = renderer->outputDevices()[deviceIndex];    

    //
    //  Create Device choice List
    //
    const AudioRenderer::DeviceVector& devices = renderer->outputDevices();
    bool initMenusOK = initAudioDeviceMenu(params, devices, d.name);

    AudioRenderer::Format currentFormat = AudioRenderer::defaultParameters().format;
    AudioRenderer::Layout currentLayout = AudioRenderer::defaultParameters().layout;
    size_t currentRate = size_t(AudioRenderer::defaultParameters().rate);

    AudioRenderer::LayoutsVector layouts;
    AudioRenderer::FormatVector formats;
    AudioRenderer::RateVector rates;

    IPCore::App()->stopAll();
    renderer->shutdown();
    renderer->availableLayouts(d, layouts);

    bool hasStereoLayout = false;
    for (int i=0; i < layouts.size(); )
    {
        if (layouts[i] == currentLayout) break;
        if (layouts[i] == TwkAudio::Stereo_2) hasStereoLayout = true;
        ++i;
        if (i == layouts.size())
        {
            // Implies currentLayout isnt a valid choice
            // for this device so use Stereo_2 if its an option
            // otherwise pick the first choice in the list.
            currentLayout = (hasStereoLayout?TwkAudio::Stereo_2:layouts.front());
        }
    }
    d.layout = currentLayout;

    renderer->availableFormats(d, formats);

    if (!formats.empty())
    {
        for (int i=0; i < formats.size(); )
        {
            if (formats[i] == currentFormat) break;
            ++i;
            if (i == formats.size())
            {
                // Implies currentFormat isnt a valid choice
                // for this device.
                currentFormat = formats.front();
            }
        }
        renderer->availableRates(d, currentFormat, rates);
    }
    IPCore::App()->resumeAll();

    // Create the channel layout choice list
    initMenusOK = initMenusOK && initAudioLayoutMenu(params, layouts, currentLayout);

    // Create format choice list
    initMenusOK = initMenusOK && initAudioFormatMenu(params, formats, currentFormat);

    // Create rates choice list
    initMenusOK = initMenusOK && initAudioRatesMenu(params, rates, currentRate);

    if (initMenusOK)
    {
        AudioRenderer::setDefaultParameters(params);
        AudioRenderer::reset();
    }
}

void
RvPreferences::audioDeviceChanged(int index)
{
    if (AudioRenderer::audioDisabledAlways()) return;

    bool hasChanged = true;

    if (index != -1)
    {
        AudioRenderer::RendererParameters params = AudioRenderer::defaultParameters();
        string deviceChoice = m_ui.audioDeviceCombo->currentText().toUtf8().constData();

        if (deviceChoice != params.device)
        {
            params.device = deviceChoice;
            AudioRenderer::setDefaultParameters(params);
        }
        else
        {
            hasChanged = false;
        }
    }

    // cerr << "audioDeviceChanged index=" << index << " hasChanged=" << (int) hasChanged << endl;

    AudioRenderer* renderer = 0;
    try { renderer = AudioRenderer::renderer(); }
    catch(...) { /* cerr << "renderer() threw" << endl; */ }

    if (!renderer || !hasChanged) return;

    AudioRenderer::RendererParameters params = AudioRenderer::defaultParameters();

    string deviceName = params.device;
    int deviceIndex = renderer->findDeviceByName(deviceName);
    if (deviceIndex == -1) 
    {
        cerr << "cant find device " << deviceName << endl;
        deviceIndex = renderer->findDefaultDevice();
    }
    AudioRenderer::Device d = renderer->outputDevices()[deviceIndex];    
    AudioRenderer::Format currentFormat = AudioRenderer::defaultParameters().format;
    AudioRenderer::Layout currentLayout = AudioRenderer::defaultParameters().layout;
    size_t currentRate = size_t(AudioRenderer::defaultParameters().rate);

    AudioRenderer::LayoutsVector layouts;
    AudioRenderer::FormatVector formats;
    AudioRenderer::RateVector rates;

    IPCore::App()->stopAll();
    renderer->shutdown();
    renderer->availableLayouts(d, layouts);

    bool hasStereoLayout = false;
    for (int i=0; i < layouts.size(); )
    {
        if (layouts[i] == currentLayout) break;
        if (layouts[i] == TwkAudio::Stereo_2) hasStereoLayout = true;
        ++i;
        if (i == layouts.size())
        {
            // Implies currentLayout isnt a valid choice
            // for this device so use Stereo_2 if its an option
            // otherwise pick the first choice in the list.
            currentLayout = (hasStereoLayout?TwkAudio::Stereo_2:layouts.front());
        }
    }
    d.layout = currentLayout;

    renderer->availableFormats(d, formats);
    if (!formats.empty())
    {
        for (int i=0; i < formats.size(); )
        {
            if (formats[i] == currentFormat) break;
            ++i;
            if (i == formats.size())
            {
                // Implies currentFormat isnt a valid choice
                // for this device.
                currentFormat = formats.front();
            }
        }
        renderer->availableRates(d, currentFormat, rates);
    }
    IPCore::App()->resumeAll();

    // Create the channel layout choice list
    bool initMenusOK = initAudioLayoutMenu(params, layouts, currentLayout);

    // Create format choice list
    initMenusOK = initMenusOK && initAudioFormatMenu(params, formats, currentFormat);

    // Create rates choice list
    initMenusOK = initMenusOK && initAudioRatesMenu(params, rates, currentRate);

    if (initMenusOK)
    {
        AudioRenderer::setDefaultParameters(params);
        AudioRenderer::reset();
    }
}

void
RvPreferences::audioChannelsChanged(int index)
{
    if (AudioRenderer::audioDisabledAlways()) return;

    bool hasChanged = true;

    if ( (index != -1) && (index < m_ui.audioDeviceLayoutCombo->count()) )
    {
        AudioRenderer::RendererParameters params = AudioRenderer::defaultParameters();

        TwkAudio::Layout layoutChoice = (TwkAudio::Layout) m_ui.audioDeviceLayoutCombo->itemData(index).toInt();

        if (layoutChoice != params.layout)
        {
            params.layout = layoutChoice;
            AudioRenderer::setDefaultParameters(params);
        }
        else
        {
            hasChanged = false;
        }
    }

    AudioRenderer* renderer = 0;
    try { renderer = AudioRenderer::renderer(); }
    catch(...) { /* cerr << "renderer() threw" << endl; */ }

    if (!renderer || !hasChanged) return;

    AudioRenderer::RendererParameters params = AudioRenderer::defaultParameters();

    string deviceName = params.device;
    int deviceIndex = renderer->findDeviceByName(deviceName);
    if (deviceIndex == -1) deviceIndex = renderer->findDefaultDevice();
    const AudioRenderer::Device& d = renderer->outputDevices()[deviceIndex];    

    AudioRenderer::Format currentFormat = AudioRenderer::defaultParameters().format;
    size_t currentRate = size_t(AudioRenderer::defaultParameters().rate);

    AudioRenderer::FormatVector formats;
    AudioRenderer::RateVector rates;

    IPCore::App()->stopAll();
    renderer->shutdown();
    renderer->availableFormats(d, formats);
    if (!formats.empty())
    {
        for (int i=0; i < formats.size(); )
        {
            if (formats[i] == currentFormat) break;
            ++i;
            if (i == formats.size())
            {
                // Implies currentFormat isnt a valid choice
                // for this device.
                currentFormat = formats.front();
            }
        }
        renderer->availableRates(d, currentFormat, rates);
    }
    IPCore::App()->resumeAll();

    // Create format choice list
    bool initMenusOK = initAudioFormatMenu(params, formats, currentFormat);

    // Create rates choice list
    initMenusOK = initMenusOK && initAudioRatesMenu(params, rates, currentRate);

    if (initMenusOK)
    {
        AudioRenderer::setDefaultParameters(params);
        AudioRenderer::reset();
    }
}


void
RvPreferences::audioFormatChanged(int index)
{
    if (AudioRenderer::audioDisabledAlways()) return;

    bool hasChanged = true;

    if ( (index != -1) && (index < m_ui.audioDeviceFormatCombo->count()) )
    {
        AudioRenderer::RendererParameters params = AudioRenderer::defaultParameters();

        TwkAudio::Format formatChoice = (TwkAudio::Format) m_ui.audioDeviceFormatCombo->itemData(index).toInt();

        if (formatChoice != params.format)
        {
            params.format = formatChoice;
            AudioRenderer::setDefaultParameters(params);

        }
        else
        {
            hasChanged = false;
        }
    }

    AudioRenderer* renderer = 0;
    try { renderer = AudioRenderer::renderer(); }
    catch(...) { /* cerr << "renderer() threw" << endl; */ }

    if (!renderer || !hasChanged) return;

    AudioRenderer::RendererParameters params = AudioRenderer::defaultParameters();

    string deviceName = params.device;
    int deviceIndex = renderer->findDeviceByName(deviceName);
    if (deviceIndex == -1) deviceIndex = renderer->findDefaultDevice();
    const AudioRenderer::Device& d = renderer->outputDevices()[deviceIndex];    

    AudioRenderer::Format currentFormat = AudioRenderer::defaultParameters().format;
    size_t currentRate = size_t(AudioRenderer::defaultParameters().rate);

    AudioRenderer::RateVector rates;

    IPCore::App()->stopAll();
    renderer->shutdown();
    renderer->availableRates(d, currentFormat, rates);
    IPCore::App()->resumeAll();

    // Create rates choice list
    bool initMenusOK = initAudioRatesMenu(params, rates, currentRate);

    if (initMenusOK)
    {
        AudioRenderer::setDefaultParameters(params);
        AudioRenderer::reset();
    }
}


void
RvPreferences::audioRateChanged(int index)
{
    if (AudioRenderer::audioDisabledAlways()) return;
    size_t rate = m_ui.audioDeviceRateCombo->currentText().toInt();

    AudioRenderer::RendererParameters params = AudioRenderer::defaultParameters();
    params.rate = rate;
    AudioRenderer::setDefaultParameters(params);
    AudioRenderer::reset();
}

void
RvPreferences::audioHoldOpenChanged(int state)
{
    AudioRenderer::RendererParameters params = AudioRenderer::defaultParameters();
    params.holdOpen = state == Qt::Checked;
    AudioRenderer::setDefaultParameters(params);
    AudioRenderer::reset();
}

void
RvPreferences::audioVideoSyncChanged(int state)
{
    AudioRenderer::RendererParameters params = AudioRenderer::defaultParameters();
    params.hardwareLock = state == Qt::Checked;
    AudioRenderer::setDefaultParameters(params);
    AudioRenderer::reset();
}

void
RvPreferences::audioPreRollChanged(int state)
{
    AudioRenderer::RendererParameters params = AudioRenderer::defaultParameters();
    params.preRoll = state == Qt::Checked;
    AudioRenderer::setDefaultParameters(params);
    AudioRenderer::reset();
}


void
RvPreferences::audioDevicePacketChanged()
{
    AudioRenderer::RendererParameters params = AudioRenderer::defaultParameters();
    params.framesPerBuffer = m_ui.audioDevicePacketEdit->text().toInt();
    AudioRenderer::setDefaultParameters(params);
    AudioRenderer::reset();
}

void
RvPreferences::audioCachePacketChanged()
{
}

//----------------------------------------------------------------------
//
//  PACKAGE management
//

void 
RvPreferences::clickedPackage(QModelIndex index)
{
    if (index.column() == 0)
    {
        QStandardItem* pitem = m_packageModel->itemFromIndex(index);
        Package& package = m_packages[pitem->data().toInt()];

        if (!pitem->isEnabled()) return;

        if (package.compatible)
        {
            if (package.installed)
            {
                uninstallPackage(package);
            }
            else
            {
                installPackage(package);
            }

            for (int i = 0; i < m_packages.size(); i++)
            {
                const Package& p = m_packages[i];
                if (p.row >= 0)
                {
                    QStandardItem* iitem = m_packageModel->item(p.row, 0);
                    QStandardItem* litem = m_packageModel->item(p.row, 1);

                    if (p.compatible)
                    {
                        iitem->setCheckState(p.installed ? Qt::Checked : Qt::Unchecked);
                        litem->setCheckState(p.installed && p.loadable ? Qt::Checked : Qt::Unchecked);
                    }
                }
            }

            m_ui.extRemoveButton->setEnabled(!package.installed && 
                                             (package.fileWritable && 
                                              package.dirWritable));
        }
    }
    else if (index.column() == 1)
    {
        QStandardItem* pitem = m_packageModel->itemFromIndex(index);
        Package& package = m_packages[pitem->data().toInt()];

        if (package.installed)
        {
            allowLoading(package, 
                         pitem->checkState() != Qt::Checked,
                         0);

            size_t n = m_packageModel->rowCount();

            for (size_t q = 0; q < n; q++)
            {
                QStandardItem* item = m_packageModel->item(q, 1);
                Package& package = m_packages[item->data().toInt()];

                if (package.installed && package.compatible)
                {
                    item->setCheckState(package.loadable ? Qt::Checked : Qt::Unchecked);
                }
            }
        }
    }
}

void 
RvPreferences::addPackageItem(const Package& p, size_t index)
{
    QFileInfo finfo(p.file);

    QStandardItem* installed    = new QStandardItem();
    QStandardItem* loaded       = new QStandardItem();
    QStandardItem* name         = new QStandardItem(p.name);
    QStandardItem* file         = new QStandardItem(finfo.fileName());
    QStandardItem* version      = new QStandardItem(p.version);
    QStandardItem* author       = new QStandardItem(p.author);
    QStandardItem* organization = new QStandardItem(p.organization);

    if (p.compatible)
    {
        loaded->setCheckState(p.loadable && p.installed ? Qt::Checked : Qt::Unchecked);
        installed->setCheckState(p.installed ? Qt::Checked : Qt::Unchecked);
    }
    else
    {
        QBrush b = name->foreground();
        QColor c = b.color();
        c.setAlpha(128);
        QBrush f = QBrush(c);

        name->setForeground(f);
        file->setForeground(f);
        version->setForeground(f);
        author->setForeground(f);
        organization->setForeground(f);
    }

    installed->setData(QVariant(int(index)));
    loaded->setData(QVariant(int(index)));

    installed->setEditable(false);
    loaded->setEditable(false);
    name->setEditable(false);
    file->setEditable(false);
    version->setEditable(false);
    author->setEditable(false);
    organization->setEditable(false);

    if (!p.dirWritable /*|| p.system*/) installed->setEnabled(false);
    //if (p.system) loaded->setEnabled(false);

    QList<QStandardItem*> row;
    row << installed << loaded << name << file << version << author << organization;

    m_packageModel->appendRow(row);
}

void
RvPreferences::loadPackages()
{
    m_packageModel->clear();
    QStringList headers;
    headers << "Installed" << "Load" << "Name" << "File" << "Version" << "Author" << "Organization";
    m_packageModel->setHorizontalHeaderLabels(headers);

    PackageManager::loadPackages();

    for (int i=0; i < headers.size(); i++) m_ui.extTreeView->resizeColumnToContents(i);
}


void
RvPreferences::declarePackage(Package& p, size_t i)
{
    if (!p.hidden || m_showHiddenPackages)
    {
        p.row = m_packageModel->rowCount();
        addPackageItem(p, i);
    }
    else
    {
        p.row = -1;
    }
}

void
RvPreferences::packageSelection(const QItemSelection& selected,
                                const QItemSelection& deselected)
{
    QItemSelectionModel* s = m_ui.extTreeView->selectionModel();
    QModelIndexList indices = s->selection().indexes();
    QTextBrowser* b = m_ui.extTextBrowser;
    
    b->clear();

    if (indices.empty()) return;

    ostringstream str;

    QStandardItem* pitem = m_packageModel->itemFromIndex(indices[0]);
    int row = pitem->data().toInt();
    const Package& p = m_packages[row];

    str << "<html>";
    
    if (getenv("RV_DARK") && *getenv("RV_DARK") == '1')
    {
        str << "<style type=\"text/css\">a:link {color: rgb(100,100,200);}</style>";
    }

    str << "<h2>" << p.name.toUtf8().data() << "</h2>";

    if (!p.compatible)
    {
        str << "<h3>" << "-- NOT COMPATIBLE WITH THIS VERSION OF RV --" << "</h3>";
    }

    if (p.system)
    {
        str << "<h3>-- THIS IS A BASE SYSTEM PACKAGE REQUIRED BY RV --</h3>";
    }

    str << "<center>"
        << "<table border=0>";


    if (p.author != "")
        str << "<tr><td align=right> <b>Author</b> </td><td> " 
            << p.author.toUtf8().data() << " </td></tr>";

    if (p.organization != "")
        str << "<tr><td align=right> <b>Organization</b> </td><td> " 
            << p.organization.toUtf8().data() << " </td></tr>";

    if (p.contact != "")
        str << "<tr><td align=right> <b>Contact</b> </td><td> <a href=\"mailto:" 
            << p.contact.toUtf8().constData() << "\">"
            << p.contact.toUtf8().constData() << "</a></td></tr>";

    if (p.url != "")
        str << "<tr><td align=right> <b>URL</b> </td><td><a href=\""
            << p.url.toUtf8().constData() << "\">"
            << p.url.toUtf8().constData() << "</a></td></tr>";

    if (p.version != "")
        str << "<tr><td align=right> <b>Version</b> </td><td> "
            << p.version.toUtf8().data() << " </td></tr>";

    if (p.requires != "")
        str << "<tr><td align=right> <b>Requires</b> </td><td> "
            << p.requires.toUtf8().data() << " </td></tr>";

    if (p.rvversion != "")
        str << "<tr><td align=right> <b>RV Version</b> </td><td> "
            << p.rvversion.toUtf8().data() << " </td></tr>";

    if (p.openrvversion != "")
        str << "<tr><td align=right> <b>OpenRV Version</b> </td><td> "
            << p.openrvversion.toUtf8().data() << " </td></tr>";

    str << "<tr><td align=right> <b>File</b> </td><td> "
        << p.file.toUtf8().data() << " </td></tr>";

    if (p.optional)
        str << "<tr><td align=right> <b>Optional Package</b> </td><td> "
            << "This package is not loaded by default </td></tr>";

    str << "</table>"
        << "</center>"
        << "<h3>Description</h3>"
        << p.description.toUtf8().data();

    str << "<h3>Modes in Package</h3>\n<ul>\n";

    for (size_t q = 0; q < p.modes.size(); q++)
    {
        str << "<li>" << p.modes[q].file.toUtf8().data() << "</li>\n";

        if (p.modes[q].requires.size()) 
        {
            str << "\nRequires<ul>";

            for (int i = 0; i < p.modes[q].requires.size(); i++)
            {
                str << "<li>" << p.modes[q].requires[i].toUtf8().data() << "</li>\n";
            }

            str << "</ul>\n";
        }
    }
        
    str << "</ul>\n";

    str << "<h3>Files in Package</h3>\n<ul>\n";

    for (size_t q = 0; q < p.files.size(); q++)
    {
        str << "<li>" << p.files[q].toUtf8().data() << "</li>\n";
    }
         
    str << "</ul>\n";
    str << "</html>";

    b->insertHtml(QString::fromUtf8(str.str().c_str()));

    m_ui.extRemoveButton->setEnabled(!p.installed && 
                                     (p.fileWritable && 
                                      p.dirWritable));
}

void
RvPreferences::showHiddenPackages(bool b)
{
    m_showHiddenPackages = b;
    loadPackages();
}

void
RvPreferences::addPackage(bool)
{
    //
    //  Figure out what dir to copy them to
    //

    TwkApp::Bundle::PathVector supportDirs =
        TwkApp::Bundle::mainBundle()->supportPath();

    QStringList possiblePlaces;

    for (size_t i = 0; i < supportDirs.size(); i++)
    {
        QString pdirname = QString::fromUtf8(supportDirs[i].c_str());
        QDir pdir(pdirname);

        if (pdir.exists())
        { 
            if (!pdir.exists("Packages")) 
            {
                if (!makeSupportDirTree(pdir)) continue;
            }

            pdir.cd("Packages");

            if (TwkUtil::isWritable(UTF8::qconvert(pdir.absolutePath()).c_str()))
            {
                possiblePlaces.push_back(pdir.absolutePath());
            }
        }
    }

    if (possiblePlaces.empty())
    {
        QMessageBox box(this);
        box.setWindowTitle(tr("Unable to Add Packages"));
        box.setText(tr("No package installation locations were found."));
        box.setWindowModality(Qt::WindowModal);
        QPushButton* b2 = box.addButton(tr("Ok"), QMessageBox::AcceptRole);
        box.setIcon(QMessageBox::Critical);
        box.exec();
        return;
    }

    //
    //
    //

    RV_QSETTINGS;

    settings.beginGroup("Packages");
    QString dirname = settings.value("lastPackageDir").toString();

#if defined(PLATFORM_WINDOWS)
    QFileDialog* fileDialog = new QFileDialog(this, 
                                              "Select rvpkg RV package files",
                                              dirname,
                                              "rvpkg Package Files (*.zip *.rvpkg *.rvpkgs)");

    QStringList files;
    if (fileDialog->exec()) files = fileDialog->selectedFiles();
    delete fileDialog;
    
#else
    QString selectedFilter;
    QFileDialog::Options options;
    // Using non native dialog on linux.
    // Rationale:
    // When using native QFileDialog (default) on linux, the following warning
    // appears in the console:
    // <<GTk-Message: 17:38:26.978: GtkDialog mapped without a transient parent.
    //   This is discouraged.>>
    // The warning does not appear when using non native dialog.
    // Moreover, some users have reported that the dialog fails to show since RV
    // was updated to Qt 5.15.3.
#if defined(PLATFORM_LINUX)
    options |= QFileDialog::DontUseNativeDialog;
#endif
    QStringList files = QFileDialog::getOpenFileNames(this, 
                                                      "Select rvpkg RV package files",
                                                      dirname,
                                                      "rvpkg Package Files (*.zip *.rvpkg *.rvpkgs, *.rvpkgs)",
                                                      &selectedFilter,
                                                      options);
#endif

    if (files.size())
    {
        QFileInfo info(files[0]);
        settings.setValue("lastPackageDir", info.absolutePath());
        settings.endGroup();
    }
    else
    {
        settings.endGroup();
        return;
    }

    //if (possiblePlaces.size() > 1)
    {
        //
        //  Ask user where to put it
        //

        if (!m_packageLocationDialog)
        {
            m_packageLocationDialog = new QDialog(this, Qt::Sheet);
            m_packageLocationUI.setupUi(m_packageLocationDialog);
        }

        QListWidget* list = m_packageLocationUI.packageLocationListWidget;
        list->clear();
        list->setSelectionMode(QAbstractItemView::SingleSelection);

        for (size_t i = 0; i < possiblePlaces.size(); i++)
        {
            list->addItem(possiblePlaces[i]);
        }

        list->item(0)->setSelected(true);

        m_packageLocationDialog->show();
        m_packageLocationDialog->exec();

        if (m_packageLocationDialog->result() == QDialog::Rejected)
        {
            return;
        }

        QList<QListWidgetItem*> selection = list->selectedItems();
        int row = list->row(selection.front());
        QString l = possiblePlaces[row];
        possiblePlaces.clear();
        possiblePlaces.push_back(l);
    }

    QStringList nocopy;
    QDir dir(possiblePlaces.front());

    addPackages(files, dir.absolutePath());
    loadPackages();
}

void
RvPreferences::removePackage(bool)
{
    QItemSelectionModel* s = m_ui.extTreeView->selectionModel();
    QModelIndexList indices = s->selection().indexes();

    QMessageBox box(this);
    box.setWindowTitle(tr("Confirm Removal"));

    QString t = tr("Remove Packages ?\n\nThe following packages will be removed:\n");
    size_t count = 0;

    for (size_t i=0; i < indices.size(); i++)
    {
        int row = m_packageModel->itemFromIndex(indices[i])->data().toInt();

        if (m_packages[row].zipFile && indices[i].column() == 0)
        {
            t += m_packages[row].file;
            t += "\n";
            count++;
        }
    }

    if (!count) return;

    box.setText(t);
    box.setWindowModality(Qt::WindowModal);
    QPushButton* b1 = box.addButton(tr("Abort"), QMessageBox::RejectRole);
    QPushButton* b2 = box.addButton(tr("Remove"), QMessageBox::AcceptRole);
    box.setIcon(QMessageBox::Question);
    box.exec();

    if (box.clickedButton() == b1) return;

    QStringList toremove;
    QStringList files;

    for (size_t i = 0; i < indices.size(); i++)
    {
        if (indices[i].column() == 0)
        {
            int row = m_packageModel->itemFromIndex(indices[i])->data().toInt();

            if (m_packages[row].zipFile)
            {
                files.push_back(m_packages[row].file);
            }
        }
    }

    QTextBrowser* b = m_ui.extTextBrowser;
    b->clear();

    removePackages(files);
}

//----------------------------------------------------------------------

bool 
RvPreferences::fixLoadability(const QString& msg)
{
    QMessageBox box(this);
    box.setWindowTitle(tr("Unloadable Package Dependencies"));
    box.setText(tr("Can't make package loadable because some of its dependencies are not loadable. Try and fix them first?\n\nDetails:\n") + msg);

    box.setWindowModality(Qt::WindowModal);
    QPushButton* b1 = box.addButton(tr("Abort"), QMessageBox::RejectRole);
    QPushButton* b2 = box.addButton(tr("Continue"), QMessageBox::AcceptRole);
    box.setIcon(QMessageBox::Critical);
    box.exec();
    
    return box.clickedButton() != b1;
}

bool 
RvPreferences::fixUnloadability(const QString& msg)
{
    QMessageBox box(this);
    box.setWindowTitle(tr("Loadable Package Dependencies"));
    box.setText(tr("Can't make package unloadable because some loaded packages depend on it. Unload them too?\n\nDetails:\n") + msg);

    box.setWindowModality(Qt::WindowModal);
    QPushButton* b1 = box.addButton(tr("Abort"), QMessageBox::RejectRole);
    QPushButton* b2 = box.addButton(tr("Continue"), QMessageBox::AcceptRole);
    box.setIcon(QMessageBox::Critical);
    box.exec();
    
    return box.clickedButton() != b1;
}

bool 
RvPreferences::installDependantPackages(const QString& msg)
{
    QMessageBox box(this);
    box.setWindowTitle(tr("Some Packages Depend on This One"));
    box.setText(tr("Can't install package because some other packages dependend on this one. Try and install them first?\n\nDetails:\n") + msg);

    box.setWindowModality(Qt::WindowModal);
    QPushButton* b1 = box.addButton(tr("Abort"), QMessageBox::RejectRole);
    QPushButton* b2 = box.addButton(tr("Continue"), QMessageBox::AcceptRole);
    box.setIcon(QMessageBox::Critical);
    box.exec();
    
    return box.clickedButton() != b1;
}

bool 
RvPreferences::overwriteExistingFiles(const QString& msg)
{
    QMessageBox box(this);
    box.setWindowTitle(tr("Existing Package Files"));
    box.setText(tr("Package files conflict with existing files? Overwrite Existing Files?\n\nDetails:\n") + msg);

    box.setWindowModality(Qt::WindowModal);
    QPushButton* b1 = box.addButton(tr("Abort"), QMessageBox::RejectRole);
    QPushButton* b2 = box.addButton(tr("Overwrite"), QMessageBox::AcceptRole);
    box.setIcon(QMessageBox::Critical);
    box.exec();

    return box.clickedButton() != b1;
}

void 
RvPreferences::errorMissingPackageDependancies(const QString& msg)
{
    QMessageBox box(this);
    box.setWindowTitle(tr("Missing Package Dependencies"));
    box.setText(tr("Can't install package because some of its dependencies are missing.\n\nDetails:\n") + msg);

    box.setWindowModality(Qt::WindowModal);
    QPushButton* b1 = box.addButton(tr("Abort Install"), QMessageBox::AcceptRole);
    box.setIcon(QMessageBox::Critical);
    box.exec();
}

bool 
RvPreferences::uninstallDependantPackages(const QString& msg)
{
    QMessageBox box(this);
    box.setWindowTitle(tr("Some Packages Depend on This One"));
    box.setText(tr("Can't uninstall package because some other packages dependend on this one. Try and uninstall them first?\n\nDetails:\n") + msg);

    box.setWindowModality(Qt::WindowModal);
    QPushButton* b1 = box.addButton(tr("Abort"), QMessageBox::RejectRole);
    QPushButton* b2 = box.addButton(tr("Continue"), QMessageBox::AcceptRole);
    box.setIcon(QMessageBox::Critical);
    box.exec();

    return box.clickedButton() != b1;
}

void 
RvPreferences::informCannotRemoveSomeFiles(const QString& msg)
{
    QMessageBox box(this);
    box.setWindowTitle(tr("Some Files Cannot Be Removed"));
    box.setText(tr("Could not remove some of the package files.\n\nDetails:\n") + msg);

    box.setWindowModality(Qt::WindowModal);
    QPushButton* b2 = box.addButton(tr("Ok"), QMessageBox::AcceptRole);
    box.setIcon(QMessageBox::Critical);
    box.exec();
}

void 
RvPreferences::errorModeFileWriteFailed(const QString& file)
{
    QMessageBox box(this);
    box.setWindowTitle(tr("File Write Failed"));
    box.setText(tr("File cannot be written:\n") + file); 
    box.setWindowModality(Qt::WindowModal);
    QPushButton* b2 = box.addButton(tr("Ok"), QMessageBox::AcceptRole);
    box.setIcon(QMessageBox::Critical);
    box.exec();
}

void 
RvPreferences::informPackageFailedToCopy(const QString& msg)
{
    QMessageBox box(this);
    box.setWindowTitle(tr("Package Failed to Copy"));
    box.setText(tr("Could not copy package files.\n\nDetails:\n") + msg);

    box.setWindowModality(Qt::WindowModal);
    QPushButton* b2 = box.addButton(tr("Ok"), QMessageBox::AcceptRole);
    box.setIcon(QMessageBox::Critical);
    box.exec();
}

bool 
RvPreferences::uninstallForRemoval(const QString& msg)
{
    QMessageBox box(this);
    box.setWindowTitle(tr("Package is currently installed"));
    box.setText(tr("In order to remove the package it must be uninstalled.\n\nDetails:\n") + msg);

    box.setWindowModality(Qt::WindowModal);
    QPushButton* b1 = box.addButton(tr("Abort"), QMessageBox::RejectRole);
    QPushButton* b2 = box.addButton(tr("Continue"), QMessageBox::AcceptRole);
    box.setIcon(QMessageBox::Critical);
    box.exec();

    return box.clickedButton() != b1;
}

void
RvPreferences::updateVideo()
{
    const IPCore::Application::VideoModules& vmods = RvApp()->videoModules();

    Options& opts = Options::sharedOptions();
    m_ui.videoModuleCombo->clear();

    if (m_currentVideoModule < 0)
    {
        QString pdev = opts.presentDevice;
        QStringList parts = pdev.split("/");

        if (parts.size() == 2)
        {
            m_currentVideoModule =
                RvApp()->findVideoModuleIndexByName(parts[0].toUtf8().constData());;

            if (m_currentVideoDevice < 0 && m_currentVideoModule >= 0)
            {
                const auto m = vmods[m_currentVideoModule];
                const VideoModule::VideoDevices& devs = m->devices();
                string dname = parts[1].toUtf8().constData();

                for (size_t i = 0; i < devs.size(); i++)
                {
                    if (devs[i]->name() == dname)
                    {
                        m_currentVideoDevice = i;
                        break;
                    }
                }
            }
        }
        else
        {
            m_currentVideoModule = 0;
        }
    }

    if (m_currentVideoModule < 0) m_currentVideoModule = 0;
    if (m_currentVideoDevice < 0) m_currentVideoDevice = 0;

    RvDocument* doc = 
        reinterpret_cast<RvDocument*>(TwkApp::Document::documents().front()->opaquePointer());

    for (size_t i = 0; i < vmods.size(); i++)
    {
        auto mod = vmods[i];
        m_ui.videoModuleCombo->addItem(QString::fromUtf8(mod->name().c_str()));

        if (i == m_currentVideoModule)
        {
            if (!mod->isOpen()) RvApp()->openVideoModule(mod.get());
            m_ui.videoModuleCombo->setCurrentIndex(i);
            m_ui.videoDeviceCombo->clear();
            m_ui.videoDeviceCombo->hide();

            QString sdk = UTF8::qconvert(mod->SDKIdentifier());
            QString info = UTF8::qconvert(mod->SDKInfo());
            if (info != "") sdk = QString("%1 -- %2").arg(sdk).arg(info);
            m_ui.videoModuleSDKLabel->setText(sdk);

            const TwkApp::VideoModule::VideoDevices& devs = mod->devices();

            for (size_t j = 0; j < devs.size(); j++)
            {
                TwkApp::VideoDevice* d = devs[j];
                string hd = d->hardwareIdentification();
                string n  = d->name();
                ostringstream str;

                str << hd;
                if (n != hd) str << " (" << n << ")";

                m_ui.videoDeviceCombo->addItem(QString::fromUtf8(str.str().c_str()));

                ostringstream mname;
                mname << mod->name() << "/" << d->name();

                if (j == m_currentVideoDevice)
                {
                    m_lockPresentCheck = true;
                    m_ui.presentationCheckBox->setCheckState(mname.str() == opts.presentDevice 
                                                             ? Qt::Checked : Qt::Unchecked);
                    m_lockPresentCheck = false;

                    m_ui.videoDeviceCombo->setCurrentIndex(j);
                    string options = RvApp()->setVideoDeviceStateFromSettings(d);
                    m_ui.additionalOptionsEdit->setText(options.c_str());
                    updateVideoFormat(d);
                    updateVideo4KTransport(d);
                    updateVideoDataFormat(d);
                    updateVideoSync(d);
                    updateVideoSyncSource(d);
                    updateVideoAudioFormat(d);
                    updateVideoProfiles(d);
                    m_ui.videoSwapStereoEyesCheckBox->setCheckState(
                        d->swapStereoEyes() ? Qt::Checked : Qt::Unchecked);
                }
            }

            m_ui.videoDeviceCombo->show();
        }
    }
}

void
RvPreferences::updateVideoProfiles(TwkApp::VideoDevice* device)
{
    //
    //  Profiles
    //

    IPCore::ProfileVector profiles;

    const TwkApp::Application::Documents& docs = RvApp()->documents();
    Rv::Session* s = static_cast<Rv::Session*>(docs[0]);
    profilesInPath(profiles, "display", &s->graph());

    QList<QComboBox*> combos;
    combos.push_back(m_ui.moduleProfileCombo);
    combos.push_back(m_ui.deviceProfileCombo);
    combos.push_back(m_ui.formatProfileCombo);

    for (size_t i = 0; i < combos.size(); i++) combos[i]->clear();

    RV_QSETTINGS;

    string modName = device->humanReadableID(VideoDevice::ModuleNameID);
    settings.beginGroup(QString::fromUtf8(modName.c_str()));
    QString modProfile = settings.value("DisplayProfile", "").toString();
    settings.endGroup();

    string devName = device->humanReadableID(VideoDevice::DeviceNameID);
    settings.beginGroup(QString::fromUtf8(devName.c_str()));
    QString devProfile = settings.value("DisplayProfile", "").toString();
    settings.endGroup();

    string formatName = device->humanReadableID(VideoDevice::VideoAndDataFormatID);
    settings.beginGroup(QString::fromUtf8(formatName.c_str()));
    QString formatProfile = settings.value("DisplayProfile", "").toString();
    settings.endGroup();

    int modIndex = -1;
    int devIndex = -1;
    int formatIndex = -1;

    //
    //  Add the profile names
    //

    for (size_t i = 0; i < profiles.size(); i++)
    {
        QString name(profiles[i]->name().c_str());

        for (size_t q = 0; q < combos.size(); q++) 
        {
            combos[q]->addItem(name);
        }

        if (name == modProfile) modIndex = i;
        if (name == devProfile) devIndex = i;
        if (name == formatProfile) formatIndex = i;
    }

    //
    //  Add the create and default items
    //

    for (size_t i = 0; i < combos.size(); i++) 
    {
        combos[i]->insertSeparator(combos[i]->count());
        
        switch (i)
        {
          case 0: combos[i]->addItem("- Use Default Profile -"); break;
          case 1: combos[i]->addItem("- Use Module Profile -"); break;
          case 2: combos[i]->addItem("- Use Device Profile -"); break;
        }
    }

    //
    //  Set current values
    //

    combos[0]->setCurrentIndex(modIndex == -1 ? combos[0]->count() - 1 : modIndex);
    combos[1]->setCurrentIndex(devIndex == -1 ? combos[1]->count() - 1 : devIndex);
    combos[2]->setCurrentIndex(formatIndex == -1 ? combos[2]->count() - 1 : formatIndex);
}

void
RvPreferences::updateVideoFormat(TwkApp::VideoDevice* d)
{
    m_ui.videoFormatCombo->clear();

    if (d->numVideoFormats() == 0)
    {
        m_ui.videoFormatCombo->addItem("N/A");
        m_ui.videoFormatCombo->setEnabled(false);
    }
    else
    {
        m_ui.videoFormatCombo->setEnabled(true);
    
        for (size_t i = 0; i < d->numVideoFormats(); i++)
        {
            m_ui.videoFormatCombo->addItem(QString::fromUtf8(d->videoFormatAtIndex(i).description.c_str()));
            if (i == d->currentVideoFormat())
                m_ui.videoFormatCombo->setCurrentIndex(i);
        }
    }
}

void
RvPreferences::updateVideo4KTransport(TwkApp::VideoDevice* d)
{
    m_ui.video4KTransportCombo->clear();

    if (d->numVideo4KTransports() == 0)
    {
        m_ui.video4KTransportCombo->addItem("N/A");
        m_ui.video4KTransportCombo->setEnabled(false);
    }
    else
    {
        m_ui.video4KTransportCombo->setEnabled(true);
    
        for (size_t i = 0; i < d->numVideo4KTransports(); i++)
        {
            m_ui.video4KTransportCombo->addItem(QString::fromUtf8(d->video4KTransportAtIndex(i).description.c_str()));
            if (i == d->currentVideo4KTransport())
                m_ui.video4KTransportCombo->setCurrentIndex(i);
        }
    }
}

void
RvPreferences::updateVideoAudioFormat(TwkApp::VideoDevice* d)
{
    m_ui.videoAudioFormatCombo->clear();
    m_ui.videoAudioDeviceCheckBox->setEnabled(d->hasAudioOutput());
    m_ui.videoAudioDeviceCheckBox->setCheckState(d->audioOutputEnabled() ? Qt::Checked : Qt::Unchecked);
    m_ui.useVideoLatencyCheckBox->setEnabled(!d->hasAudioOutput() || !d->audioOutputEnabled());
    m_ui.configureVideoLatencyButton->setEnabled(!d->hasAudioOutput() || !d->audioOutputEnabled());
    m_ui.useVideoLatencyCheckBox->setCheckState(d->useLatencyForAudio() ? Qt::Checked : Qt::Unchecked);

    if (d->numAudioFormats() == 0)
    {
        m_ui.videoAudioFormatCombo->addItem("N/A");
        m_ui.videoAudioFormatCombo->setEnabled(false);
    }
    else
    {
        m_ui.videoAudioFormatCombo->setEnabled(true);
    
        for (size_t i = 0; i < d->numAudioFormats(); i++)
        {
            m_ui.videoAudioFormatCombo->addItem(QString::fromUtf8(d->audioFormatAtIndex(i).description.c_str()));
            if (i == d->currentAudioFormat())
                m_ui.videoAudioFormatCombo->setCurrentIndex(i);
        }
    }
}

void
RvPreferences::updateVideoDataFormat(TwkApp::VideoDevice* d)
{
    m_ui.dataFormatCombo->clear();

    if (d->numDataFormats() == 0)
    {
        m_ui.dataFormatCombo->addItem("N/A");
        m_ui.dataFormatCombo->setEnabled(false);
    }
    else
    {
        m_ui.dataFormatCombo->setEnabled(true);

        for (size_t i = 0; i < d->numDataFormats(); i++)
        {
            m_ui.dataFormatCombo->addItem(QString::fromUtf8(d->dataFormatAtIndex(i).description.c_str()));
            if (i == d->currentDataFormat())
                m_ui.dataFormatCombo->setCurrentIndex(i);
        }
    }
}

void
RvPreferences::updateVideoSync(TwkApp::VideoDevice* d)
{
    m_ui.syncMethodCombo->clear();

    if (d->numSyncModes() == 0)
    {
        m_ui.syncMethodCombo->addItem("N/A");
        m_ui.syncMethodCombo->setEnabled(false);
    }
    else
    {
        m_ui.syncMethodCombo->setEnabled(true);

        for (size_t i = 0; i < d->numSyncModes(); i++)
        {
            m_ui.syncMethodCombo->addItem(QString::fromUtf8(d->syncModeAtIndex(i).description.c_str()));
            if (i == d->currentSyncMode())
                m_ui.syncMethodCombo->setCurrentIndex(i);
        }
    }
}

void
RvPreferences::updateVideoSyncSource(TwkApp::VideoDevice* d)
{
    m_ui.syncSourceCombo->clear();

    if (d->numSyncSources() == 0)
    {
        m_ui.syncSourceCombo->addItem("N/A");
        m_ui.syncSourceCombo->setEnabled(false);
    }
    else
    {
        m_ui.syncSourceCombo->setEnabled(true);
    
        for (size_t i = 0; i < d->numSyncSources(); i++)
        {
            m_ui.syncSourceCombo->addItem(QString::fromUtf8(d->syncSourceAtIndex(i).description.c_str()));
            if (i == d->currentSyncSource())
                m_ui.syncSourceCombo->setCurrentIndex(i);
        }
    }
}

void 
RvPreferences::videoModuleChanged(int index)
{
    m_currentVideoModule = index;
    m_currentVideoDevice = -1;
    updateVideo();
}

void 
RvPreferences::videoDeviceChanged(int index)
{
    m_currentVideoDevice = index;
    updateVideo();
}

void 
RvPreferences::videoFormatChanged(int v)
{
    if (VideoDevice* d = currentVideoDevice())
    {
        d->setVideoFormat(v);
        const VideoModule* m = d->module();
        ostringstream str;
        str << m->name() << "/" << d->name();

        RV_QSETTINGS;
        settings.beginGroup(QString::fromUtf8(str.str().c_str()));
        settings.setValue("videoFormat", v);
        settings.endGroup();

        // The user has set a new video format which overrides the video format
        // specified on the command line if any
        Rv::Options::sharedOptions().presentFormat=nullptr;

        updateVideoDataFormat(d);
        updateVideoSync(d);
        updateVideoSyncSource(d);
        updateVideoProfiles(d);
    }
}

void 
RvPreferences::video4KTransportChanged(int v)
{
    if (VideoDevice* d = currentVideoDevice())
    {
        d->setVideo4KTransport(v);
        const VideoModule* m = d->module();
        ostringstream str;
        str << m->name() << "/" << d->name();

        RV_QSETTINGS;
        settings.beginGroup(QString::fromUtf8(str.str().c_str()));
        settings.setValue("video4KTransport", v);
        settings.endGroup();

        updateVideoProfiles(d);
    }
}

void 
RvPreferences::videoAudioFormatChanged(int v)
{
    if (VideoDevice* d = currentVideoDevice())
    {
        d->setAudioFormat(v);
        const VideoModule* m = d->module();
        ostringstream str;
        str << m->name() << "/" << d->name();

        RV_QSETTINGS;
        settings.beginGroup(QString::fromUtf8(str.str().c_str()));
        settings.setValue("audioFormat", v);
        settings.endGroup();
    }
}

void 
RvPreferences::videoDataFormatChanged(int v)
{
    if (VideoDevice* d = currentVideoDevice())
    {
        d->setDataFormat(v);
        const VideoModule* m = d->module();
        ostringstream str;
        str << m->name() << "/" << d->name();

        RV_QSETTINGS;
        settings.beginGroup(QString::fromUtf8(str.str().c_str()));
        settings.setValue("dataFormat", v);
        settings.endGroup();

        // The user has set a new video data format which overrides the video 
        // data format specified on the command line if any
        Rv::Options::sharedOptions().presentData=nullptr;

        updateVideoSync(d);
        updateVideoSyncSource(d);
        updateVideoProfiles(d);
    }
}

void 
RvPreferences::syncMethodChanged(int v)
{
    if (VideoDevice* d = currentVideoDevice())
    {
        d->setSyncMode(v);
        const VideoModule* m = d->module();
        ostringstream str;
        str << m->name() << "/" << d->name();

        RV_QSETTINGS;
        settings.beginGroup(QString::fromUtf8(str.str().c_str()));
        settings.setValue("syncMode", v);
        settings.endGroup();

        updateVideoSyncSource(d);
    }
}

void 
RvPreferences::syncSourceChanged(int v)
{
    if (VideoDevice* d = currentVideoDevice())
    {
        d->setSyncSource(v);
        const VideoModule* m = d->module();
        ostringstream str;
        str << m->name() << "/" << d->name();

        RV_QSETTINGS;
        settings.beginGroup(QString::fromUtf8(str.str().c_str()));
        settings.setValue("syncSource", v);
        settings.endGroup();
    }
}

void 
RvPreferences::videoAudioCheckBoxChanged(int v)
{
    if (VideoDevice* d = currentVideoDevice())
    {
        const VideoModule* m = d->module();
        ostringstream str;
        str << m->name() << "/" << d->name();

        bool checked = v == Qt::Checked;

        m_ui.useVideoLatencyCheckBox->setEnabled(!checked);
        m_ui.configureVideoLatencyButton->setEnabled(!checked);

        RV_QSETTINGS;
        settings.beginGroup(QString::fromUtf8(str.str().c_str()));
        settings.setValue("useAsAudioDevice", checked);
        settings.endGroup();

        // This user action overrides the presentAudio command line option if any
        Rv::Options::sharedOptions().presentAudio=-1;
    }
}

void 
RvPreferences::videoSwapStereoEyesChanged(int v)
{
    if (VideoDevice* d = currentVideoDevice())
    {
        const VideoModule* m = d->module();
        ostringstream str;
        str << m->name() << "/" << d->name();

        bool checked = v == Qt::Checked;

        RV_QSETTINGS;
        settings.beginGroup(QString::fromUtf8(str.str().c_str()));
        settings.setValue("swapStereoEyes", checked);
        settings.endGroup();

        d->setSwapStereoEyes(checked);
    }
}

void 
RvPreferences::videoUseLatencyCheckBoxChanged(int v)
{
    if (VideoDevice* d = currentVideoDevice())
    {
        const VideoModule* m = d->module();
        ostringstream str;
        str << m->name() << "/" << d->name();

        bool checked = v == Qt::Checked;

        RV_QSETTINGS;
        settings.beginGroup(QString::fromUtf8(str.str().c_str()));
        settings.setValue("useLatencyForAudio", checked);
        settings.endGroup();
    }
}

void 
RvPreferences::videoAdditionalOptionsChanged()
{
    if (VideoDevice* d = currentVideoDevice())
    {
        const VideoModule* m = d->module();
        ostringstream str;
        str << m->name() << "/" << d->name();

        QString s = m_ui.additionalOptionsEdit->toPlainText();

        RV_QSETTINGS;
        settings.beginGroup(QString::fromUtf8(str.str().c_str()));
        settings.setValue("additionalOptions", s);
        settings.endGroup();
    }
}

void
RvPreferences::presentationCheckBoxChanged(int state)
{
    if (m_lockPresentCheck) return;

    if (VideoDevice* d = currentVideoDevice())
    {
        const VideoModule* m = d->module();
        ostringstream str;
        str << m->name() << "/" << d->name();
        Options& opts = Options::sharedOptions();

        RV_QSETTINGS;
        settings.beginGroup("Video");

        // ALLOWED LEAKING! WINDOWS BUG!
        //if (strcmp(opts.presentDevice, "")) free(opts.presentDevice);
        opts.presentDevice = (char*)"";

        if (state == Qt::Checked)
        {
            settings.setValue("presentationDevice", QString::fromUtf8(str.str().c_str()));
            opts.presentDevice = strdup(str.str().c_str());
        }
        else
        {
            settings.setValue("presentationDevice", QString());
        }

        settings.endGroup();
    }
}

TwkApp::VideoDevice* 
RvPreferences::currentVideoDevice() const
{
    if (m_currentVideoDevice == -1 ||
        m_currentVideoModule == -1) return 0;

    const IPCore::Application::VideoModules& vmods = RvApp()->videoModules();
    const auto mod = vmods[m_currentVideoModule];
    const VideoModule::VideoDevices& devs = mod->devices();
    return devs[m_currentVideoDevice];
}

void 
RvPreferences::audioOffsetFinished()
{
    const TwkApp::Application::Documents& docs = IPCore::App()->documents();

    Options& opts = Options::sharedOptions();
    opts.audioGlobalOffset = m_ui.audioGlobalOffsetEdit->text().toDouble();

    for (size_t i = 0; i < docs.size(); i++)
    {
        Session* s = static_cast<Session*>(docs[i]);
        s->setGlobalAudioOffset(opts.audioGlobalOffset);
    }
}

void 
RvPreferences::audioDeviceLatencyFinished()
{
    AudioRenderer::RendererParameters params = AudioRenderer::defaultParameters();
    Options& opts = Options::sharedOptions();

    opts.audioDeviceLatency = m_ui.audioDeviceLatencyEdit->text().toDouble();

    // Convert from msecs to secs and reverse the sign.
    params.latency = -opts.audioDeviceLatency / 1000.0;

    AudioRenderer::setDefaultParameters(params);

    AudioRenderer::reset();
}

void
RvPreferences::latencyChanged(const QString& s, bool fixed)
{
    double v = s.toDouble();
    TwkApp::VideoDevice* d = currentVideoDevice();
    
    if (fixed) d->setFixedLatency(v);
    else d->setFrameLatency(v);

    double tl = d->totalLatencyInSeconds();
    m_videoLatencyUI.totalLatencyLabel->setText(QString("%1 ms").arg(tl * 1000.0));
}

void
RvPreferences::frameLatencyChanged(const QString& s)
{
    latencyChanged(s, false);
}

void
RvPreferences::fixedLatencyChanged(const QString& s)
{
    latencyChanged(s, true);
}

void
RvPreferences::configVideoLatency()
{
    if (!m_videoLatencyDialog)
    {
        m_videoLatencyDialog = new QDialog(this, Qt::Sheet);
        m_videoLatencyUI.setupUi(m_videoLatencyDialog);

        connect(m_videoLatencyUI.fixedLatencyEdit, 
                SIGNAL(textEdited(const QString&)),
                this, 
                SLOT(fixedLatencyChanged(const QString&)));

        connect(m_videoLatencyUI.frameLatencyEdit, 
                SIGNAL(textEdited(const QString&)),
                this, 
                SLOT(frameLatencyChanged(const QString&)));
    }

    TwkApp::VideoDevice* d = currentVideoDevice();

    double dl = d->deviceLatency();
    double tl = d->totalLatencyInSeconds();
    double xl = d->fixedLatency();
    double fl = d->frameLatencyInFrames();

    m_videoLatencyUI.computedLatencyLabel->setText(QString("%1 ms").arg(dl * 1000.0));
    m_videoLatencyUI.totalLatencyLabel->setText(QString("%1 ms").arg(tl * 1000.0));
    m_videoLatencyUI.fixedLatencyEdit->setText(QString("%1").arg(xl));
    m_videoLatencyUI.frameLatencyEdit->setText(QString("%1").arg(fl));

    m_videoLatencyDialog->show();
    m_videoLatencyDialog->exec();

    if (m_videoLatencyDialog->result() == QDialog::Rejected)
    {
        return;
    }

    xl = m_videoLatencyUI.fixedLatencyEdit->text().toDouble();
    fl = m_videoLatencyUI.frameLatencyEdit->text().toDouble();

    d->setFixedLatency(xl);
    d->setFrameLatency(fl);

    const VideoModule* m = d->module();
    ostringstream str;
    str << m->name() << "/" << d->name();

    RV_QSETTINGS;
    settings.beginGroup(QString::fromUtf8(str.str().c_str()));
    settings.setValue("fixedLatency", xl);
    settings.setValue("frameLatency", fl);
    settings.endGroup();
}

void
RvPreferences::fontChanged()
{
    Options& opts = Options::sharedOptions();
    bool mustUpdateCSS = false;

    if (QObject::sender() == m_ui.fontSizeSpinBox)
    {
        int f1 = opts.fontSize1;
        opts.fontSize1 = m_ui.fontSizeSpinBox->value();
        if (f1 != opts.fontSize1) mustUpdateCSS = true;
    }
    else
    {
        int f2 = opts.fontSize2;
        opts.fontSize2 = m_ui.fontSize2SpinBox->value();
        if (f2 != opts.fontSize2) mustUpdateCSS = true;
    }

    if (mustUpdateCSS) updateStyleSheet();
}

void 
RvPreferences::modProfileChanged(int index)
{
    TwkApp::VideoDevice* d = currentVideoDevice();
    size_t n = m_ui.moduleProfileCombo->count();

    RV_QSETTINGS;

    string modName = d->humanReadableID(VideoDevice::ModuleNameID);
    settings.beginGroup(QString::fromUtf8(modName.c_str()));

    if (index < n - 2)
    {
        settings.setValue("DisplayProfile", QVariant(m_ui.moduleProfileCombo->currentText()));
    }
    else if (index == n - 1)
    {
        settings.remove("DisplayProfile");
    }

    settings.endGroup();

    updateVideoProfiles(d);
}

void 
RvPreferences::devProfileChanged(int index)
{
    TwkApp::VideoDevice* d = currentVideoDevice();
    size_t n = m_ui.deviceProfileCombo->count();

    RV_QSETTINGS;

    string devName = d->humanReadableID(VideoDevice::DeviceNameID);
    settings.beginGroup(QString::fromUtf8(devName.c_str()));

    if (index < n - 2)
    {
        settings.setValue("DisplayProfile", QVariant(m_ui.deviceProfileCombo->currentText()));
    }
    else if (index == n - 1)
    {
        settings.remove("DisplayProfile");
    }

    settings.endGroup();

    updateVideoProfiles(d);
}

void 
RvPreferences::formatProfileChanged(int index)
{
    TwkApp::VideoDevice* d = currentVideoDevice();
    size_t n = m_ui.formatProfileCombo->count();

    RV_QSETTINGS;

    string formatName = d->humanReadableID(VideoDevice::VideoAndDataFormatID);
    settings.beginGroup(QString::fromUtf8(formatName.c_str()));

    if (index < n - 2)
    {
        settings.setValue("DisplayProfile", QVariant(m_ui.formatProfileCombo->currentText()));
    }
    else if (index == n - 1)
    {
        settings.remove("DisplayProfile");
    }

    settings.endGroup();

    updateVideoProfiles(d);
}


} // Rv