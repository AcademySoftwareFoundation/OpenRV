//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

module: scrub_offset 
{

use rvtypes;
use commands;

require rvui;
require extra_commands;
require app_utils;
require math;

function: deb (void; string s) { if (false) print (s + "\n"); }
 
class: ScrubOffset : MinorMode
{
    string _currentSource;
    Point  _startPos;
    int    _startOffset;
    int    _startSourceFrame;
    int    _startSourceStart;
    int    _startSourceEnd;

    method: getSource (string; Point pos)
    {
	for_each (s; imagesAtPixel (pos, nil, true))
	{
	    if (s.inside) 
	    {
		return nodeGroup(s.node) + "_source";
	    }
	}
	return nil;
    }

    method: release (void; Event event)
    {
        popEventTable("scrub-offset-table");
	toggle();
	redraw();
    }

    method: mySourceFrame (int; int f)
    {
	let infos = metaEvaluate (f, nil);
        for_each (i; infos) if (i.node == _currentSource) return i.frame;
        return 0;
    }

    method: initState (void; Point p)
    {
        _startPos         = p;
        _startOffset      = getIntProperty (_currentSource + ".group.rangeOffset").front();
        _startSourceFrame = mySourceFrame(frame()) - _startOffset;

        let info = nodeRangeInfo (_currentSource);

        _startSourceStart = info.start - _startOffset;
        _startSourceEnd   = info.end - _startOffset;

        deb ("start current frame %s start %s end %s" % (_startSourceFrame, _startSourceStart, _startSourceEnd));
    }

    method: push (void; Event event)
    {
	_currentSource = getSource(event.pointer());

	if (_currentSource neq nil)
	{
            //
            //  Check for Retime in evaluation path.
            //
            for_each (rt; extra_commands.nodesInEvalPath (frame(), "RVRetime"))
            {
                if (getFloatProperty(rt + ".visual.scale").front()  != 1.0 ||
                    getFloatProperty(rt + ".visual.offset").front() != 0.0 ||
                    getIntProperty(rt + ".warp.active").front()     != 0   ||
                    getIntProperty(rt + ".explicit.active").front() != 0)
                {
                    extra_commands.displayFeedback ("Sorry, offset scrubbing will not work with retiming", 2.0);
                    rvtypes.State s = data();
                    s.pushed = false;
                    return;
                }
            }
            initState (event.pointer());

            pushEventTable("scrub-offset-table");

            extra_commands.displayFeedback ("%s" % extra_commands.uiName(_currentSource), 2.0);
	}
	event.reject();
    }

    method: activate (void; )
    {
	extra_commands.displayFeedback ("Click-Scrub to set Range Offset", 2000.0);
	MinorMode.activate(this);
    }

    method: drag (void; Event event)
    {
	let diff            = event.pointer()[0] - _startPos[0],
            initialTarget   = _startSourceFrame + int(diff/3.0),
            targetFrame     = math.max(_startSourceStart, math.min(_startSourceEnd, initialTarget)),
            off             = getIntProperty (_currentSource + ".group.rangeOffset").front(),
            currentFrame    = mySourceFrame(frame()) - off,
            currentFrameM1  = mySourceFrame(frame()-1) - off,
            currentFrameP1  = mySourceFrame(frame()+1) - off,
	    offsetDiff      = targetFrame - _startSourceFrame,
	    newOffset       = _startOffset - offsetDiff,
            outOfBounds     = false;

        if (    currentFrame   == _startSourceStart &&
                currentFrameP1 == _startSourceStart &&
                targetFrame > currentFrame) 
        { 
            outOfBounds = true; 
            targetFrame = currentFrame + 1; 
        }
        else
        if (    currentFrame   == _startSourceEnd &&
                currentFrameM1 == _startSourceEnd &&
                targetFrame < currentFrame) 
        { 
            outOfBounds = true; 
            targetFrame = currentFrame - 1; 
        }

        deb ("target %s offset: %s -> %s" % (targetFrame, _startOffset, newOffset));

	setIntProperty (_currentSource + ".group.rangeOffset", int[] { newOffset });
        extra_commands.displayFeedback("%s offset %d" % (extra_commands.uiName(_currentSource), newOffset), 2.0);

        let newCurrentFrame = mySourceFrame(frame()) - newOffset;

        deb ("    new current frame %s" % newCurrentFrame);

        /*
        bool outOfBounds = ((newCurrentFrame == _startSourceEnd   && currentFrame == _startSourceEnd) ||
                            (newCurrentFrame == _startSourceStart && currentFrame == _startSourceStart));
                            */

        if (!outOfBounds && newCurrentFrame != targetFrame) 
        {
            deb ("pending oldcur %s newcur %s target %s" % (currentFrame, newCurrentFrame, targetFrame));
            setFrame (frame() + (targetFrame - newCurrentFrame));
        }

        if (offsetDiff != 0) initState (event.pointer());

	redraw();
    }

    method: ScrubOffset (ScrubOffset; string name)
    {
	_currentSource = nil;
	_startPos = Point {0, 0};

        this.init(name,
		  [ ("pointer-1--push", push,  "") ],
		  nil,
                  nil);

	app_utils.bind("default", "scrub-offset-table", "pointer-1--drag", drag, "");
	app_utils.bind("default", "scrub-offset-table", "pointer-1--release", release, "");
    }
}


\: createMode (Mode;)
{
    return ScrubOffset("scrub-offset");
}

}
