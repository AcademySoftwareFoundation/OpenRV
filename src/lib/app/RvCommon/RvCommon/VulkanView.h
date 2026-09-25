//
//  Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#pragma once

#include <QtWidgets/QWidget>
#include <QtCore/QSize>

QT_BEGIN_NAMESPACE
class QWindow;
QT_END_NAMESPACE

namespace Rv
{
    class RvDocument;
    class QTVulkanVideoDevice;
    class VulkanWindow;

    //
    //  VulkanView
    //
    //  Host QWidget that embeds the native Vulkan viewport (VulkanWindow) via
    //  QWidget::createWindowContainer(), and owns the QTVulkanVideoDevice that
    //  drives it. Mirrors GLView/GLWindow; the native window keeps the main
    //  window off Qt's render-to-texture composite path.
    //
    class VulkanView : public QWidget
    {
        Q_OBJECT

    public:
        VulkanView(RvDocument* doc, QWidget* parent = nullptr, bool noResize = true);
        ~VulkanView() override;

        VulkanWindow* vulkanWindow() const { return m_vulkanWindow; }

        QTVulkanVideoDevice* videoDevice() const { return m_videoDevice; }

        void stopProcessingEvents();

        bool firstPaintCompleted() const;

        bool isInitialized() const;

        void absolutePosition(int& x, int& y) const;

        float devicePixelRatio() const;

        void setContentSize(int w, int h) { m_csize = QSize(w, h); }

        void setMinimumContentSize(int w, int h) { m_msize = QSize(w, h); }

        QSize sizeHint() const override { return m_csize; }

        QSize minimumSizeHint() const override { return m_msize; }

        //  See VulkanWindow::supports10BitPresentation().
        static bool supports10BitPresentation();

    private:
        //
        //  Keep the viewport window alive across top-level window churn. Qt
        //  recreates the top-level QWidgetWindow when a widget is reparented
        //  into it (e.g. a plugin adding a QWebEngineView), deleting the child
        //  viewport. destroyed() fires before children are deleted, so the
        //  viewport is detached there and re-attached later (as in GLView).
        //
        void watchParentWindow();
        void parentWindowDestroyed();
        void reattachVulkanWindow();

    protected:
        void showEvent(QShowEvent*) override;

    private:
        RvDocument* m_doc;
        VulkanWindow* m_vulkanWindow;
        QWidget* m_container;
        QTVulkanVideoDevice* m_videoDevice;
        QSize m_csize;
        QSize m_msize;

        //  See watchParentWindow().
        QWindow* m_watchedParentWindow;
        QMetaObject::Connection m_watchedParentConnection;
        bool m_reattachPending;
    };

} // namespace Rv
