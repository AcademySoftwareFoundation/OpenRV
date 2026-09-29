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
    //  drives it.
    //
    //  This is deliberately the same shape as GLView/GLWindow. Keeping the
    //  viewport on a native window of its own -- rather than on a widget that
    //  Qt composites into the top-level window -- is what keeps the main window
    //  off a render-to-texture composite path, and it means the two backends
    //  share one set of embedding and lifetime rules instead of two.
    //
    class VulkanView : public QWidget
    {
        Q_OBJECT

    public:
        VulkanView(RvDocument* doc, QWidget* parent = nullptr, bool noResize = true);
        ~VulkanView() override;

        VulkanWindow* vulkanWindow() const { return m_vulkanWindow; }

        QTVulkanVideoDevice* videoDevice() const { return m_videoDevice; }

        //
        //  Delegated to the viewport window.
        //
        void stopProcessingEvents();

        bool firstPaintCompleted() const;

        bool isInitialized() const;

        void absolutePosition(int& x, int& y) const;

        float devicePixelRatio() const;

        void setContentSize(int w, int h) { m_csize = QSize(w, h); }

        void setMinimumContentSize(int w, int h) { m_msize = QSize(w, h); }

        QSize sizeHint() const override { return m_csize; }

        QSize minimumSizeHint() const override { return m_msize; }

        //
        //  Probe for whether this machine's Vulkan can present a 10-bit format.
        //  Forwards to VulkanWindow; see the note there.
        //
        static bool supports10BitPresentation();

    private:
        //
        //  Keeping the viewport window alive across top-level window churn.
        //
        //  createWindowContainer() transfers ownership of the viewport window to
        //  the container, which parents it to the top-level QWidgetWindow. Qt
        //  destroys and recreates that QWidgetWindow when a widget is reparented
        //  into the window -- QWidget::setParent() -> destroy() -> ~QWidgetWindow
        //  -- as happens when a plugin adds a QWebEngineView to a layout, and
        //  ~QObject deletes its child QWindows, viewport included. Nothing in Qt
        //  puts it back, and QWindowContainer then dereferences the window it no
        //  longer has on the next layout pass.
        //
        //  QObject::destroyed is emitted at the top of ~QObject, before
        //  deleteChildren() runs, so watching the parent window gives us a
        //  moment where the viewport can still be detached and kept.
        //
        //  This mirrors GLView. The one Vulkan-specific consequence is that the
        //  VkSurfaceKHR does not survive the platform window being recreated;
        //  VulkanWindow detects that on the next expose and rebuilds.
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

        //
        //  The parent QWindow whose destruction is being watched, plus the
        //  connection to it so it can be rewired when the viewport window is
        //  re-parented. See watchParentWindow().
        //
        QWindow* m_watchedParentWindow;
        QMetaObject::Connection m_watchedParentConnection;
        bool m_reattachPending;
    };

} // namespace Rv
