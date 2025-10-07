#
# Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
import rv.commands
import os
import sys
import math


def concat_seperator():
    import platform

    p = platform.system()
    if p == "Windows":
        return ";"
    else:
        return ":"


class State:
    """The session State object for all things python"""

    def __init__(self):
        self.minorModes = []


class Mode(object):
    """A mode is a feature unit. It comes in two varieties: major and
    minor. The mode can be declared in its own file and provide key bindings, a
    menu, multiple event tables if needed, a render function, and is
    identifiable by name and icon. When active the minor mode can render and
    its menu is visible. When inactive, the minor mode is completely gone from
    RV."""

    def __init__(self):
        self._active = False
        self._modeName = None
        self._menu = None
        self._supportFilesPath = None
        self._drawOnEmpty = False
        self._drawOnPresentation = False

    def isActive(self):
        return self._active

    def modeName(self):
        return self._modeName

    def drawOnEmpty(self):
        return self._drawOnEmpty

    def drawOnPresentation(self):
        return self._drawOnPresentation

    def activate(self):
        "Called right after activation"
        self._active = True
        pass

    def deactivate(self):
        "Called right before deactivation"
        self._active = False
        pass

    def toggle(self):
        self._active = not self._active

        if self._active:
            rv.commands.activateMode(self._modeName)
            self.activate()
        else:
            self.deactivate()
            rv.commands.deactivateMode(self._modeName)

        rv.commands.redraw()
        rv.commands.sendInternalEvent("mode-toggled", "%s|%s" % (self._modeName, self._active), "Mode")

    def layout(self, event):
        "Layout any margins or precompute anything necessary for rendering"
        pass

    def render(self, event):
        "The render function is called on each active minor mode."
        pass

    def supportPath(self, module, packageName=None):
        if packageName is None:
            packageName = module.__name__
        return os.path.join(
            os.path.dirname(os.path.dirname(module.__file__)),
            "SupportFiles",
            packageName,
        )

    def qmlPath(self, module, packageName=None):
        if packageName is None:
            packageName = module.__name__
        return os.path.join(os.path.dirname(os.path.dirname(module.__file__)), "QML", packageName)

    def configPath(self, packageName):
        """Returns a path to a writable directory where temporary, and
        configuration files specific to a package are/should be located. The
        directory will be created if it does not yet exist."""

        envvar = os.getenv("TWK_APP_SUPPORT_PATH")
        userDir = envvar.split(concat_seperator())[0]
        directory = os.path.join(os.path.join(userDir, "ConfigFiles"), packageName)

        try:
            if not os.path.exists(directory):
                sys.mkdir(directory, 0x1FF)
        except Exception:
            print("ERROR: mode config path failed to create directory %s" % directory)

        return directory


class MinorMode(Mode):
    """MinorModes are modes which are non-exclusive: there can be many minor
    modes active at the same time."""

    def __init__(self):
        Mode.__init__(self)

    def init(
        self,
        name,
        globalBindings,
        overrideBindings,
        menu=None,
        sortKey=None,
        ordering=0,
    ):
        self._modeName = name
        self._drawOnEmpty = False
        self._drawOnPresentation = False

        rv.commands.defineMinorMode(name, sortKey, ordering)

        if globalBindings is not None:
            for b in globalBindings:
                (event, func, docs) = b
                rv.commands.bind(self._modeName, "global", event, func, docs)

        if overrideBindings is not None:
            for b in overrideBindings:
                (event, func, docs) = b
                rv.commands.bind(self._modeName, "global", event, func, docs)

        self.setMenu(menu)

    def setMenu(self, menu):
        if menu is None:
            menu = []
        self._menu = menu
        rv.commands.defineModeMenu(self._modeName, self._menu, False)

    def setMenuStrict(self, menu):
        if menu is None:
            menu = []
        self._menu = menu
        rv.commands.defineModeMenu(self._modeName, self._menu, True)

    def defineEventTable(self, tableName, bindings):
        for b in bindings:
            (event, func, docs) = b
            rv.commands.bind(self._modeName, tableName, event, func, docs)

    def defineEventTableRegex(self, tableName, bindings):
        for b in bindings:
            (event, func, docs) = b
            rv.commands.bindRegex(self._modeName, tableName, event, func, docs)

    def urlDropFunc(self, url):
        f = None
        s = None
        return (f, s)


class Widget(MinorMode):
    """The Widget class is the base class for HUD widgets.  Each Widget must
    provide a set of bindings, a render function (to draw itself) and an
    *accurate* bounds which will determine where events are sent."""

    def __init__(self):
        MinorMode.__init__(self)

    def layout(self, event):
        pass

    def render(self, event):
        pass

    def init(
        self,
        name,
        globalBindings,
        overrideBindings,
        menu=None,
        sortKey=None,
        ordering=0,
    ):
        self._x = 0
        self._y = 0
        self._w = 100
        self._h = 100
        self._downPoint = (0, 0)
        self._dragging = False
        self._inCloseArea = False
        self._containsPointer = False
        self._whichMargin = -1
        MinorMode.init(self, name, globalBindings, overrideBindings, menu, sortKey, ordering)

    def toggle(self):
        self._active = not self._active

        rv.commands.writeSettings("Tools", "show_" + self._modeName, self._active)
        if self._active:
            rv.commands.activateMode(self._modeName)
        else:
            rv.commands.deactivateMode(self._modeName)
            self.updateMargins(False)

        rv.commands.redraw()

        rv.commands.sendInternalEvent("mode-toggled", "%s|%s" % (self._modeName, self._active), "Mode")

    def updateMargins(self, activated):
        #
        #  Below api push/pop is required because setMargins() causes
        #  margins-changed event to be sent which then triggers arbitrary mu
        #  code, which could of course allocate memory.
        #
        if activated:
            if self._whichMargin != -1:
                #
                #  updateMargins with activated==true should only be
                #  called inside a render() call, since it's only then
                #  that the "event device" is set.
                #
                m = list(rv.commands.margins())
                marginValue = self.requiredMarginValue()

                if m[self._whichMargin] < marginValue:
                    m[self._whichMargin] = marginValue
                    rv.commands.setMargins(tuple(m))
        else:
            if self._whichMargin != -1:
                #
                #  Change only the margin this mode is rendering in, but change it
                #  on self._allself._ devices.
                #
                m = [-1.0, -1.0, -1.0, -1.0]
                m[self._whichMargin] = 0.0
                rv.commands.setMargins(tuple(m))
                rv.commands.setMargins(tuple(m), True)

    def updateBounds(self, minp, maxp):
        self._x = minp[0]
        self._y = minp[1]
        self._w = maxp[0] - minp[0]
        self._h = maxp[1] - minp[1]

        if rv.commands.isModeActive(self._modeName) and not self._dragging:
            rv.commands.setEventTableBBox(self._modeName, "global", minp, maxp)
            self.updateMargins(True)

    def contains(self, p):
        if p[0] >= self._x and p[0] <= self._x + self._w and p[1] >= self._y and p[1] <= self._y + self._h:
            return True

        return False

    def requiredMarginValue(self):
        vs = rv.commands.viewSize()

        if self._whichMargin == -1:
            #
            #  No margin required
            #
            return 0.0
        elif self._whichMargin == 0:
            #
            #  Left margin
            #
            return self._x + self._w
        elif self._whichMargin == 1:
            #
            #  Right margin
            #
            return vs[0] - self._x
        elif self._whichMargin == 2:
            #
            #  Top margin
            #
            return vs[1] - self._y
        elif self._whichMargin == 3:
            #
            #  Bottom margin
            #
            return self._y + self._h
        return 0.0

    def drawInMargin(self, whichMargin):
        self._whichMargin = whichMargin

    def storeDownPoint(self, event):
        self._downPoint = event.pointer()

    def drag(self, event):
        if self._whichMargin != -1:
            return

        self._dragging = True

        pp = event.pointer()
        dp = self._downPoint
        ip = (pp[0] - dp[0], pp[1] - dp[1])

        if not self._inCloseArea:
            self._x += ip[0]
            self._y += ip[1]

        self.storeDownPoint(event)

        rv.commands.redraw()

    def near(self, event):
        domain = event.subDomain()
        p = event.relativePointer()
        tl = (0.0, domain[1])
        pc = (p[0] - tl[0], p[1] - tl[1])
        d = math.sqrt(pc[0] * pc[0] + pc[1] * pc[1])
        m = 20
        return d < m

    def move(self, event):
        self._dragging = False

        near = self.near(event)
        if near != self._inCloseArea:
            rv.commands.redraw()
        self._inCloseArea = near

        #
        #  If we've left the widget, don't draw the close button.
        #
        self._containsPointer = self.contains(event.pointer())
        if not self._containsPointer:
            self._inCloseArea = False

        event.reject()

    def release(self, event, closeFunc=None):
        self._dragging = False

        near = self.near(event)
        if near:
            if closeFunc is not None:
                closeFunc()
            elif self._active:
                self.toggle()
            self._inCloseArea = False

        self.storeDownPoint(event)
