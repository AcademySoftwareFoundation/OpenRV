//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
module: window_mode {
use rvtypes;
use commands;
use app_utils;
use extra_commands;

documentation: """
WindowMinorMode adds a Window menu and some functions to control the
window. It should appear just before the Help Menu.

NOTE: this can't be dependent on rvui since its required by it
"""

class: WindowMinorMode : MinorMode
{
    method: fitWindowToImage(void; Event event)
    {
        if (isFullScreen()) toggleFullScreen();
        require math;

        State state = data();

        //
        //  Find the rendered image that corresponds to the current view
        //

        let images = renderedImages();
        let w = 0.0, h = 0.0, useNext=false;

        for_each (i; images)
        {
            let g = nodeGroup(i.node);

            if (useNext || (g neq nil && g == viewNode()))
            {
                //
                //  Sequence commonly resizes itself to match output view, so
                //  drill down to first input in this case.
                //
                if (nodeType(g) == "RVSequenceGroup") useNext = true;
                else
                {
                    w = float(i.width) * i.pixelAspect;
                    h = float(i.height);
                }
            }
        }

        if (w == 0 || h == 0) return;

        let scale = scale(),
            totalView = viewSize(),
            m = margins(),
            d = Vec2(totalView.x - m[0] - m[1], totalView.y - m[2] - m[3]),
            ia = w / h,
            va = double(d.x) / double(d.y),
            wide = va > ia,
            ns = (if wide then h / double(d.y) else w / double(d.x));

        int width = int(w * scale/ns + 0.5);
        int height = int(h * scale/ns + 0.5);

        //
        // Set the new dimensions while maintaining the same zoom scale
        //

        setViewSize(width,height);
        frameImage();

        //
        // Determine if the window is out of bounds on the screen and pull it in
        //

        let mainWin = commands.mainWindowWidget(),
            mainRect = mainWin.frameGeometry(),
            usedRect = qt.QApplication.desktop().availableGeometry(mainWin);

        let outRight = usedRect.width() + usedRect.x() - mainWin.x() - mainRect.width(),
            outBelow = usedRect.height() + usedRect.y() - mainWin.y() - mainRect.height(),
            outLeft  = mainWin.x() - usedRect.x(),
            outAbove = mainWin.y() - usedRect.y();

        let deltaX = math.min(outRight,outLeft),
            deltaY = math.min(outBelow,outAbove);
        if (deltaX > 0) deltaX = 0;
        if (deltaX == outLeft) deltaX = -1*deltaX;
        if (deltaY > 0) deltaY = 0;
        if (deltaY == outAbove) deltaY = -1*deltaY;

        let newPos = qt.QPoint(mainWin.x() + deltaX, mainWin.y() + deltaY);
        if (deltaX != 0 || deltaY != 0) mainWin.setPos(newPos);

        redraw();
    }

    method: centerFit (void; Event event)
    {
        if (isFullScreen()) toggleFullScreen();
        centerResizeFit();
    }

    \: consoleState (int;)
    {
        if isConsoleVisible() then CheckedMenuState else UncheckedMenuState;
    }

    method: WindowMinorMode (WindowMinorMode;)
    {
        Menu menu = {
            {"Window", Menu {
                {"Full Screen",     ~toggleFullScreen, "`", nil},
                {"_", nil},
                {"Fit",             fitWindowToImage,     "w"},
                {"Center",          ~center,        nil},
                {"Center Fit",      centerFit, "W"},
                {"_", nil},
                {"Console",  ~showConsole, nil, consoleState},
                {"_", nil},
                }}
        };

        this.init("window",
                  [ ("key-down--control--F", ~toggleFullScreen, "Toggle Fullscreen Mode"),
                    ("key-down--`", ~toggleFullScreen, "Toggle Fullscreen Mode"),
                    ("key-down--w", fitWindowToImage, "Resize Window to Fit"),
                    ("key-down--W", centerFit, "Fit Window to Pixels") ],
                  nil,
                  menu,
                  "y",
                  9998);  // always second to last
    }
}

}
