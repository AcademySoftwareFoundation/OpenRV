//
//  Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//

#if defined(PLATFORM_LINUX) || defined(PLATFORM_WINDOWS)

#include <RvCommon/VulkanView.h>
#include <RvCommon/VulkanWindow.h>
#include <RvCommon/QTVulkanVideoDevice.h>
#include <RvCommon/RvDocument.h>
#include <RvApp/RvSession.h>
#include <IPCore/Session.h>

#include <QtCore/QTimer>
#include <QtGui/QWindow>
#include <QtWidgets/QVBoxLayout>

#include <sstream>

namespace Rv
{
    using namespace std;

    VulkanView::VulkanView(RvDocument* doc, QWidget* parent, bool noResize)
        : QWidget(parent)
        , m_doc(doc)
        , m_vulkanWindow(nullptr)
        , m_container(nullptr)
        , m_videoDevice(nullptr)
        , m_csize(1024, 576)
        , m_msize(128, 128)
        , m_watchedParentWindow(nullptr)
        , m_reattachPending(false)
    {
        m_vulkanWindow = new VulkanWindow(doc, noResize);
        m_container = QWidget::createWindowContainer(m_vulkanWindow, this);

        //  A doc-less view is a passive presentation output (see VulkanWindow.h).
        const bool passiveOutput = (m_doc == nullptr);

        m_container->setFocusPolicy(passiveOutput ? Qt::NoFocus : Qt::StrongFocus);

        //  RV queries the presentation device before the window is shown, and
        //  Qt only hands out a VkSurfaceKHR once the platform surface exists.
        m_vulkanWindow->create();

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(m_container);

        //  Last-resort guard in case parentWindowDestroyed() did not detach it.
        connect(m_vulkanWindow, &QObject::destroyed, this, [this]() { m_vulkanWindow = nullptr; });

        ostringstream str;
        if (m_doc)
        {
            str << UI_APPLICATION_NAME " Main Window (Vulkan)" << "/" << m_doc;
        }
        else
        {
            //  One output per screen, all doc-less: key the name on the view.
            str << UI_APPLICATION_NAME " Presentation (Vulkan)" << "/" << static_cast<const void*>(this);
        }
        //  No event widget, so no QTTranslator: VulkanWindow::event() then
        //  ignores all input for a passive output.
        m_videoDevice = new QTVulkanVideoDevice(nullptr, str.str(), m_vulkanWindow, passiveOutput ? nullptr : m_container);
        m_vulkanWindow->setVideoDevice(m_videoDevice);
        m_vulkanWindow->setEventWidget(passiveOutput ? nullptr : m_container);

        setObjectName((m_doc && m_doc->session()) ? m_doc->session()->name().c_str() : "no session");

        if (!passiveOutput)
        {
            setFocusProxy(m_container);
        }
        else
        {
            setFocusPolicy(Qt::NoFocus);
        }

        watchParentWindow();
    }

    VulkanView::~VulkanView()
    {
        //
        //  A detached viewport window (see parentWindowDestroyed()) outlives the
        //  widget tree and holds a raw pointer to the device, and a detached
        //  container is not deleted with this widget. Undo both here.
        //
        //  This must run before ~QWidget, which emits destroyed() from children
        //  after the VulkanView sub-object is gone; delivering it to our slots
        //  would trip Qt's assertObjectType or write through a dangling `this`.
        //
        if (m_watchedParentConnection)
        {
            disconnect(m_watchedParentConnection);
            m_watchedParentConnection = QMetaObject::Connection();
        }
        m_watchedParentWindow = nullptr;

        if (m_vulkanWindow)
        {
            disconnect(m_vulkanWindow, nullptr, this, nullptr);

            m_vulkanWindow->setVideoDevice(nullptr);
            m_vulkanWindow->setEventWidget(nullptr);
        }

        if (m_container && !m_container->parentWidget())
        {
            delete m_container;
            m_container = nullptr;
        }

        delete m_videoDevice;
    }

    void VulkanView::showEvent(QShowEvent* event)
    {
        QWidget::showEvent(event);

        //  The parent is only known once the container is shown, which is
        //  nested inside this show, so also check one event-loop turn later.
        watchParentWindow();
        QTimer::singleShot(0, this, &VulkanView::watchParentWindow);
    }

    void VulkanView::watchParentWindow()
    {
        //
        //  Watch the top-level widget's window: it is what Qt destroys, and it
        //  is known before the container re-parents the viewport into it. A
        //  top-level view (a presentation output) has no enclosing window.
        //
        QWidget* topLevel = window();
        if (topLevel == this)
        {
            return;
        }

        QWindow* topLevelWindow = topLevel ? topLevel->windowHandle() : nullptr;

        if (topLevelWindow == m_watchedParentWindow)
        {
            return;
        }

        if (m_watchedParentConnection)
        {
            disconnect(m_watchedParentConnection);
        }

        m_watchedParentWindow = topLevelWindow;

        if (topLevelWindow)
        {
            m_watchedParentConnection = connect(topLevelWindow, &QObject::destroyed, this, &VulkanView::parentWindowDestroyed);
        }
    }

    void VulkanView::parentWindowDestroyed()
    {
        //
        //  Emitted before the window deletes its children, so detaching here
        //  saves the viewport. It is hidden while parentless and re-attached
        //  once the top-level has its new window.
        //
        QWindow* destroyedWindow = m_watchedParentWindow;
        m_watchedParentWindow = nullptr;

        //  Leave it alone while it still belongs to QWindowContainer's
        //  placeholder parent.
        if (m_vulkanWindow && m_vulkanWindow->parent() == destroyedWindow)
        {
            m_vulkanWindow->hide();
            m_vulkanWindow->setParent(nullptr);
        }

        //
        //  QWindowContainer::parentWasMoved() dereferences the top-level's
        //  windowHandle() unchecked, and it is null until Qt recreates it, so
        //  the container must leave the tree too.
        //
        if (m_container)
        {
            if (layout())
            {
                layout()->removeWidget(m_container);
            }

            m_container->hide();
            m_container->setParent(nullptr);
        }

        if (m_reattachPending)
        {
            return;
        }

        m_reattachPending = true;
        QTimer::singleShot(0, this, &VulkanView::reattachVulkanWindow);
    }

    void VulkanView::reattachVulkanWindow()
    {
        m_reattachPending = false;

        if (!m_vulkanWindow)
        {
            return;
        }

        QWidget* topLevel = window();
        QWindow* topLevelWindow = topLevel ? topLevel->windowHandle() : nullptr;

        if (!topLevelWindow)
        {
            //  Qt recreates the top-level's window lazily; keep waiting.
            m_reattachPending = true;
            QTimer::singleShot(0, this, &VulkanView::reattachVulkanWindow);
            return;
        }

        //  Re-parenting the container makes it re-adopt the viewport window.
        if (m_container)
        {
            m_container->setParent(this);

            if (layout())
            {
                layout()->addWidget(m_container);
            }

            m_container->show();
            setFocusProxy(m_container);
        }

        if (m_vulkanWindow->parent() != topLevelWindow)
        {
            m_vulkanWindow->setParent(topLevelWindow);
        }

        m_vulkanWindow->show();

        watchParentWindow();

        if (m_container)
        {
            m_container->updateGeometry();

            if (layout())
            {
                layout()->activate();
            }
        }

        //  The new platform window invalidates the VkSurfaceKHR; the redraw
        //  makes VulkanWindow rebuild it.
        if (m_doc && m_doc->session())
        {
            m_doc->session()->askForRedraw();
        }
    }

    void VulkanView::stopProcessingEvents()
    {
        if (m_vulkanWindow)
        {
            m_vulkanWindow->stopProcessingEvents();
        }
    }

    bool VulkanView::firstPaintCompleted() const { return m_vulkanWindow && m_vulkanWindow->firstPaintCompleted(); }

    bool VulkanView::isInitialized() const { return m_vulkanWindow && m_vulkanWindow->isInitialized(); }

    void VulkanView::absolutePosition(int& x, int& y) const
    {
        if (m_vulkanWindow)
        {
            m_vulkanWindow->absolutePosition(x, y);
            return;
        }

        const QPoint gp = mapToGlobal(QPoint(0, 0));
        x = gp.x();
        y = gp.y();
    }

    float VulkanView::devicePixelRatio() const
    {
        return m_vulkanWindow ? m_vulkanWindow->devicePixelRatioF() : static_cast<float>(devicePixelRatioF());
    }

    bool VulkanView::supports10BitPresentation() { return VulkanWindow::supports10BitPresentation(); }

} // namespace Rv

#endif // PLATFORM_LINUX || PLATFORM_WINDOWS
