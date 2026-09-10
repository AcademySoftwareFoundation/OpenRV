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
        //
        //  Native Vulkan viewport window (renders + presents on its own
        //  surface).
        //
        m_vulkanWindow = new VulkanWindow(doc, noResize);

        //
        //  Embed the native window in the widget tree.
        //
        m_container = QWidget::createWindowContainer(m_vulkanWindow, this);
        m_container->setFocusPolicy(Qt::StrongFocus);

        //
        //  Create the platform surface up-front: Qt can only hand out a
        //  VkSurfaceKHR for a window that has one, and RV queries the
        //  presentation device during startup before the window is shown.
        //
        m_vulkanWindow->create();

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(m_container);

        //
        //  Last-resort guard: if the viewport window is destroyed anyway (i.e.
        //  detaching it in parentWindowDestroyed() did not get there first),
        //  make sure nothing here is left holding it.
        //
        connect(m_vulkanWindow, &QObject::destroyed, this, [this]() { m_vulkanWindow = nullptr; });

        //
        //  The device drives the Vulkan surface (the window) for presentation,
        //  and uses the container QWidget for event / coordinate translation
        //  (height-based y-flip, mapToGlobal, mouse grab).
        //
        ostringstream str;
        str << UI_APPLICATION_NAME " Main Window (Vulkan)" << "/" << m_doc;
        m_videoDevice = new QTVulkanVideoDevice(nullptr, str.str(), m_vulkanWindow, m_container);
        m_vulkanWindow->setVideoDevice(m_videoDevice);
        m_vulkanWindow->setEventWidget(m_container);

        setObjectName((m_doc && m_doc->session()) ? m_doc->session()->name().c_str() : "no session");
        setFocusProxy(m_container);

        //
        //  Realize the top-level's window now, and watch for Qt replacing it.
        //
        //  Unlike GLView this does not call createWinId() on the top level:
        //  that exists there to pin the window's composition to OpenGL before
        //  any render-to-texture widget joins the tree, and there is no such
        //  API to pin here -- the viewport presents through Vulkan on its own
        //  surface and composites with nothing.
        //
        watchParentWindow();
    }

    VulkanView::~VulkanView()
    {
        //
        //  Two things have to be undone before the device goes away, both of
        //  them consequences of the viewport window outliving the widget tree
        //  in the detached state (see parentWindowDestroyed()).
        //
        //  The window holds a raw back-pointer to the device and would keep
        //  using it -- VulkanWindow::event() and render() both dereference it
        //  -- so clear that first. And while detached the container has no
        //  parent widget, so it would not be destroyed along with this widget:
        //  it would survive as a stray top-level owning the viewport window,
        //  still pointing at a deleted device.
        //
        if (m_vulkanWindow)
        {
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

        //
        //  The container parents the viewport window to the top-level window
        //  while being shown, so the parent to watch only becomes known here --
        //  and one turn of the event loop later, since the container's own show
        //  is nested inside this one.
        //
        watchParentWindow();
        QTimer::singleShot(0, this, &VulkanView::watchParentWindow);
    }

    void VulkanView::watchParentWindow()
    {
        //
        //  Watch the top-level widget's window rather than the viewport
        //  window's current parent: it is the object Qt destroys, and it is
        //  knowable before the container gets around to re-parenting the
        //  viewport into it.
        //
        QWidget* topLevel = window();
        QWindow* topLevelWindow = topLevel ? topLevel->windowHandle() : nullptr;

        if (topLevelWindow == m_watchedParentWindow)
            return;

        if (m_watchedParentConnection)
            disconnect(m_watchedParentConnection);

        m_watchedParentWindow = topLevelWindow;

        if (topLevelWindow)
            m_watchedParentConnection = connect(topLevelWindow, &QObject::destroyed, this, &VulkanView::parentWindowDestroyed);
    }

    void VulkanView::parentWindowDestroyed()
    {
        //
        //  Emitted at the top of the window's ~QObject, before it deletes its
        //  children, so detaching here is what saves the viewport from being
        //  deleted along with it. The window becomes parentless for the moment;
        //  it is hidden so it cannot flash on screen as a stray top-level, and
        //  re-attached once the top-level has its new window.
        //
        QWindow* destroyedWindow = m_watchedParentWindow;
        m_watchedParentWindow = nullptr;

        //
        //  Only the viewport's actual parent matters. Before the container has
        //  re-parented it, the viewport still belongs to QWindowContainer's
        //  internal placeholder parent, and pulling it off that would break the
        //  container's own bookkeeping.
        //
        if (m_vulkanWindow && m_vulkanWindow->parent() == destroyedWindow)
        {
            m_vulkanWindow->hide();
            m_vulkanWindow->setParent(nullptr);
        }

        //
        //  Take the container out of the widget tree for the duration as well.
        //  Qt reaches window containers through QWindowContainer::parentWasMoved()
        //  on every layout pass and dereferences the top-level's windowHandle()
        //  without checking it -- and that is null from here until Qt recreates
        //  the window. A layout pass runs before then, inside this same
        //  reparent, so a container left in the tree faults there.
        //
        if (m_container)
        {
            if (layout())
                layout()->removeWidget(m_container);

            m_container->hide();
            m_container->setParent(nullptr);
        }

        if (m_reattachPending)
            return;

        m_reattachPending = true;
        QTimer::singleShot(0, this, &VulkanView::reattachVulkanWindow);
    }

    void VulkanView::reattachVulkanWindow()
    {
        m_reattachPending = false;

        if (!m_vulkanWindow)
            return;

        QWidget* topLevel = window();
        QWindow* topLevelWindow = topLevel ? topLevel->windowHandle() : nullptr;

        if (!topLevelWindow)
        {
            //
            //  Qt recreates the top-level's window lazily (on the next show), so
            //  keep waiting rather than forcing it here.
            //
            m_reattachPending = true;
            QTimer::singleShot(0, this, &VulkanView::reattachVulkanWindow);
            return;
        }

        //
        //  Put the container back first: re-parenting it makes QWindowContainer
        //  re-adopt the viewport window into the new top-level window itself.
        //
        if (m_container)
        {
            m_container->setParent(this);

            if (layout())
                layout()->addWidget(m_container);

            m_container->show();
            setFocusProxy(m_container);
        }

        if (m_vulkanWindow->parent() != topLevelWindow)
            m_vulkanWindow->setParent(topLevelWindow);

        m_vulkanWindow->show();

        watchParentWindow();

        //
        //  The container drives the viewport's geometry from its own, so nudge a
        //  layout pass to put the re-attached window back in place.
        //
        if (m_container)
        {
            m_container->updateGeometry();

            if (layout())
                layout()->activate();
        }

        //
        //  Re-parenting gives the viewport a new platform window, which
        //  invalidates the VkSurfaceKHR. VulkanWindow notices on its next
        //  expose and rebuilds; asking for a redraw is what gets it there.
        //
        if (m_doc && m_doc->session())
            m_doc->session()->askForRedraw();
    }

    void VulkanView::stopProcessingEvents()
    {
        if (m_vulkanWindow)
            m_vulkanWindow->stopProcessingEvents();
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
