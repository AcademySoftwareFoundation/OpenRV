//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __RvCommon__RvPreferences__h__
#define __RvCommon__RvPreferences__h__

#include <QtCore/QtCore>
#include <RvCommon/generated/ui_RvPreferences.h>
#include <RvCommon/generated/ui_PrefPackageLocationDialog.h>
#include <RvCommon/generated/ui_PrefVideoLatency.h>
#include <RvPackage/PackageManager.h>
#include <RvApp/Options.h>
#include <QtWidgets/QDialog>
#include <QtGui/QStandardItemModel>

#include <IPCore/AudioRenderer.h>

#include <string>

class QSettings;

namespace TwkApp
{
    class VideoDevice;
    class VideoModule;
} // namespace TwkApp

namespace Rv
{

    class RvPreferences
        : public QDialog
        , public PackageManager
    {
        Q_OBJECT

    public:
        RvPreferences(QWidget* parent = 0);
        virtual ~RvPreferences();

        void update();

        static void loadSettingsIntoOptions(Options&);

        static void resetPreferencesFile();

        QTabWidget* tabWidget() const { return m_ui.tabWidget; }

        //
        //  PackageManager API
        //

        virtual bool fixLoadability(const QString& msg);
        virtual bool fixUnloadability(const QString& msg);
        virtual bool overwriteExistingFiles(const QString& msg);
        virtual bool installDependantPackages(const QString& msg);
        virtual void errorMissingPackageDependancies(const QString& msg);
        virtual bool uninstallDependantPackages(const QString& msg);
        virtual void informCannotRemoveSomeFiles(const QString& msg);
        virtual void errorModeFileWriteFailed(const QString& file);
        virtual void informPackageFailedToCopy(const QString& msg);
        virtual void declarePackage(Package&, size_t);
        virtual bool uninstallForRemoval(const QString& msg);

        virtual void loadPackages();

        void updateVideo();
        void updateVideoFormat(TwkApp::VideoDevice*);
        void updateVideo4KTransport(TwkApp::VideoDevice*);
        void updateVideoDataFormat(TwkApp::VideoDevice*);
        void updateVideoSync(TwkApp::VideoDevice*);
        void updateVideoSyncSource(TwkApp::VideoDevice*);
        void updateVideoAudioFormat(TwkApp::VideoDevice*);

        void updateVideoProfiles(TwkApp::VideoDevice*);
        void updateVideoOptions(TwkApp::VideoDevice*);

        TwkApp::VideoDevice* currentVideoDevice() const;

        void updateStyleSheet();

    public slots:
        void write();
        void exrAutoThreads(int state);
        void exrThreadNumChanged(const QString& text);
        void addPackageItem(const Package&, size_t);
        void packageSelection(const QItemSelection&, const QItemSelection&);
        void clickedPackage(QModelIndex);
        void addPackage(bool);
        void removePackage(bool);
        void showHiddenPackages(bool);

        void stylusAsMouseChanged(int);
        void startupResizeChanged(int);

        void audioModuleChanged(int index);
        void audioDeviceChanged(int index);
        void audioChannelsChanged(int index);
        void audioFormatChanged(int index);
        void audioRateChanged(int index);
        void audioHoldOpenChanged(int);
        void audioVideoSyncChanged(int);
        void audioPreRollChanged(int);
        void audioDevicePacketChanged();
        void audioCachePacketChanged();
        void audioOffsetFinished();
        void audioDeviceLatencyFinished();

        void lookaheadCacheSizeFinisihed();
        void regionCacheSizeFinisihed();
        void bufferWaitFinished();
        void lookBehindFinished();
        void rthreadFinished();
        void cacheOutsideRegionChanged(int);

        void bitDepthChanged(int);
        void allowFloatChanged(int);
        void newGLSLlutInterpChanged(int);
        void swapScanlinesChanged(int);
        void prefetchChanged(int);
        void appleClientStorageChanged(int);
        void useThreadedUploadChanged(int);
        void videoSyncChanged(int);
        void displayOutputChanged(int);

        void exrRGBAChanged(int);
        void exrInheritChanged(int);
        void exrPlanar3ChannelChanged(int);
        void exrNoOneChannelChanged(int);
        void exrReadWindowIsDisplayWindowChanged(int);
        void exrNumThreadsFinished();
        void exrChunkSizeFinished();
        void exrMaxInFlightFinished();
        void exrIOMethodChanged(int);
        void exrReadWindowChanged(int);

        void cinChunkSizeFinished();
        void cinMaxInFlightFinished();
        void cinIOMethodChanged(int);
        void cinChromaChanged(int);
        void cinPixelsChanged(int);

        void dpxChunkSizeFinished();
        void dpxMaxInFlightFinished();
        void dpxIOMethodChanged(int);
        void dpxChromaChanged(int);
        void dpxPixelsChanged(int);

        void tgaChunkSizeFinished();
        void tgaMaxInFlightFinished();
        void tgaIOMethodChanged(int);

        void tiffChunkSizeFinished();
        void tiffMaxInFlightFinished();
        void tiffIOMethodChanged(int);

        void jpegChunkSizeFinished();
        void jpegMaxInFlightFinished();
        void jpegIOMethodChanged(int);
        void jpegRGBAChanged(int);

        void videoModuleChanged(int);
        void videoDeviceChanged(int);
        void videoFormatChanged(int);
        void video4KTransportChanged(int);
        void videoDataFormatChanged(int);
        void syncMethodChanged(int);
        void syncSourceChanged(int);
        void presentationCheckBoxChanged(int);
        void videoAudioCheckBoxChanged(int);
        void videoAudioFormatChanged(int);
        void videoSwapStereoEyesChanged(int);
        void videoUseLatencyCheckBoxChanged(int);
        void videoAdditionalOptionsChanged();

        void modProfileChanged(int);
        void devProfileChanged(int);
        void formatProfileChanged(int);

        void configVideoLatency();

        void fixedLatencyChanged(const QString&);
        void frameLatencyChanged(const QString&);

        void fontChanged();

    private:
        void closeEvent(QCloseEvent*);
        static void loadSettingsIntoOptions(RvSettings&, Options&);
        static void write(QSettings&);
        void latencyChanged(const QString&, bool);

        bool
        initAudioDeviceMenu(IPCore::AudioRenderer::RendererParameters& params,
                            const IPCore::AudioRenderer::DeviceVector& devices,
                            const std::string& currentDeviceName);

        bool
        initAudioLayoutMenu(IPCore::AudioRenderer::RendererParameters& params,
                            const IPCore::AudioRenderer::LayoutsVector& layouts,
                            const IPCore::AudioRenderer::Layout& currentLayout);

        bool
        initAudioFormatMenu(IPCore::AudioRenderer::RendererParameters& params,
                            const IPCore::AudioRenderer::FormatVector& formats,
                            const IPCore::AudioRenderer::Format& currentFormat);

        bool
        initAudioRatesMenu(IPCore::AudioRenderer::RendererParameters& params,
                           const IPCore::AudioRenderer::RateVector& rates,
                           const size_t& currentRate);

    private:
        Ui::RvPreferences m_ui;
        Ui::packageLocationDialog m_packageLocationUI;
        Ui::PrefVideoLatencyDialog m_videoLatencyUI;
        QDialog* m_packageLocationDialog;
        QDialog* m_videoLatencyDialog;
        QStandardItemModel* m_packageModel;
        int m_currentVideoModule;
        int m_currentVideoDevice;
        bool m_showHiddenPackages;
        bool m_updated;
        bool m_lockPresentCheck;
    };

    class ScrollEventEater : public QObject
    {
        Q_OBJECT

    public:
        ScrollEventEater(QObject* obj)
            : QObject(obj)
        {
        }

    protected:
        bool eventFilter(QObject* obj, QEvent* event);
    };

} // namespace Rv

#endif // __RvCommon__RvPreferences__h__
