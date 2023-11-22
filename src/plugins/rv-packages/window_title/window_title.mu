//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
module: window_title {
use rvtypes;
use commands;
use extra_commands;
require qt;

class: WindowTitle : MinorMode
{ 
    string _title;
    string _mystate;
    string _titlePrefix;
    bool _isRelease;
    qt.QTimer _playStopTimer;
    qt.QTimer _updateTimer;

    method: mediaAtFrame (string; int frame)
    {
        if (!isSessionEmpty())
        {
            try
            {
                let sourceNames = sourcesAtFrame(frame),
                    media       = sourceMedia(sourceNames.front());
                
                return media._0.split("/").back();
            }
            catch (...)
            {
                ;
            }
        }

        return "";
    }

    method: set (void; string s)
    {
        if (_title != s) 
        {
            _title = s;
            if (_isRelease)
            {
                setWindowTitle(_title);
            }
            else
            {
                setWindowTitle(_titlePrefix + _title);
            }
        }
    }

    method: updateTitle (void; Event event)
    {
        if (event neq nil) event.reject();

	//
	//  If we're loading media, do nothing, since we'll get
	//  notification when progressive loading is complete and we
	//  don't want to slow it down.
	//
	if (loadTotal() != 0) return;

        try
        {
            let f = frame();

            if (isPlaying())
            {
                set("%s -- Playing" % mediaAtFrame(f));
            }
            else
            {
                let m = mediaAtFrame(f);

                if (m == "") set("Untitled");
                else set("%s -- Frame %d" % (m, sourceFrame(f)));
            }
        }
        catch (...) {;}
    }

    method: updateIfStateChanged (void; )
    {
        if (isPlaying()  && _mystate != "playing")
        {
            updateTitle(nil);
            unbind(_modeName, "global", "frame-changed");
            _mystate = "playing";
        }
        else 
        if (!isPlaying() && _mystate != "stopped")
        {
            updateTitle(nil);
            bind(_modeName, "global", "frame-changed", updateTitle, "");
            _mystate = "stopped";
        }
    }

    method: playChanged (void; Event event)
    {
        event.reject();
        if (!_playStopTimer.active()) _playStopTimer.start();
    }

    method: updateAfterDelay (void; Event event)
    {
	event.reject();

        _updateTimer.start();
    }

    method: updateTimeout (void; )
    {
	updateTitle(nil);
    }

    method: WindowTitle(WindowTitle; string name)
    {
        init(name,
             nil,
             [("play-start", playChanged, ""),
              ("play-stop", playChanged, ""),
              ("after-graph-view-change", updateAfterDelay, ""),
              ("after-progressive-loading", updateAfterDelay, ""),
              ("source-group-complete", updateAfterDelay, ""),
              ("graph-node-inputs-changed", updateAfterDelay, ""),
              ("frame-changed", updateTitle, "")],
             nil);

        _title = "";
        _mystate = "unknown";

        // Compute the title prefix.
        _titlePrefix = getReleaseVariant();
        _isRelease = _titlePrefix == "RELEASE";
        if (!_isRelease)
        {
            _titlePrefix = "(%s) " % _titlePrefix;
        }

        _playStopTimer = qt.QTimer(mainWindowWidget());
        _playStopTimer.setSingleShot(true);
        _playStopTimer.setInterval(100);

        qt.connect (_playStopTimer, qt.QTimer.timeout, updateIfStateChanged);

        _updateTimer = qt.QTimer(mainWindowWidget());
	_updateTimer.setSingleShot(true);
	_updateTimer.setInterval(100);

	qt.connect (_updateTimer, qt.QTimer.timeout, updateTimeout);
    }
}

\: createMode (Mode;)
{
    return WindowTitle("window-title");
}
}
