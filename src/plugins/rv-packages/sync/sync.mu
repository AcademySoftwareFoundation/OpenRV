//*****************************************************************************/
// Copyright (c) 2021 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************/

//
//  Syncronize two (or more) RVs with one another
//

module: sync {
use rvtypes;
use app_utils;
use math;
use math_util;
use commands;
use extra_commands;
use gl;
use glu;
use glyph;
use rvui;
use io;
use mode_manager;
require sync_mode;

global int SYNC_VERSION_CURRENT = 5;
global int SYNC_VERSION_WITH_CHECK_SYNC_VERSION_SUPPORT = 5;
global int SYNC_VERSION_MIN_COMPATIBLE = 4;
global float POINTER_TIME_TO_LIVE = 10.0;

\: deb(void; string s) 
{ 
    if (false) 
    { 
        require qt;

        let d = qt.QDateTime.currentDateTime();
        print(\"sync(%s,%s): %s\\n\" % (myNetworkPort(), d.toString(\"mm:ss:zzz\", qt.QCalendar()), s));
    }
}

//
//  NOTE: these can be the node type or a regex matching against a
//  specific full property name like color001.color.exposure, etc., or
//  the equivalent including a type name. They're used to filter what
//  state is sent/received.
//

global regex MatchThing         = ".*";
global regex MatchColor         = "#RVColor\\.(color\\.(gamma|exposure|contrast|scale|offset|hue|saturation)|CDL\\.*)";
global regex MatchSessionMatte  = "#RVSession\\.matte\\.*";
global regex MatchViewTransform = "#RVDispTransform2D\\..*";
global regex MatchDisplayColor  = "#RVDisplayColor\\..*|#OCIODisplay\\.color\\..";
global regex MatchStereo        = "#RVSourceStereo\\..*";
global regex MatchDisplayStereo = "#RVDisplayStereo\\..*";
global regex MatchFormat        = "#RVFormat\\..*|#RVTransform2D\\..*";
global regex MatchAudio         = "#RVSoundTrack\\..*|#RVFileSource\\.group\\.(audioOffset|volume|balance|crossover)";
global regex MatchAnnotation    = "#RVPaint\\..*|#RVBrush\\..*";

global regex MatchFolderG       = "#RVFolderGroup\\..*";
global regex MatchStackG        = "#RVStackGroup\\..*";
global regex MatchSwitchG       = "#RVSwitchGroup\\..*";
global regex MatchSourceG       = "#RVSourceGroup\\..*";
global regex MatchSequenceG     = "#RVSequenceGroup\\..*";
global regex MatchLayoutG       = "#RVLayoutGroup\\..*";
global regex MatchStack         = "#RVStack\\..*";
global regex MatchSequence      = "#RVSequence\\..*";
global regex MatchSwitch        = "#RVSwitch\\..*";
global regex MatchTransform2D   = "#RVTransform2D\\..*";
global regex MatchRetime        = "#RVRetime\\..*";
global regex MatchFileSource    = "(#RVFileSource|#RVImageSource)\\.(cut|request)\\..*|#RVFileSource\\.group\\.(rangeOffset|rangeStart)";

//
//  These are to never be synced
//
global regex MatchTagProperty   = ".*\\.tag\\..*";

//
//  These props are handled using insert to lower the bandwidth. We don't
//  want to send the entire property everytime they change because this can
//  lead to geometry growth in message size when the property is being
//  appended to.
//

global regex MatchNoStateChange = "#RVPaint\\..*\\.(points|width)";

//
//  We need these prop values to actually be maximal across the sync session,
//  but they don't need to be continuous, so just make sure we get the global
//  max (actually supremum) each time they change.  (only implemented for int
//  props so far.)
//

global regex MatchMaxStateChange = "#RVPaint\\..*\\.(nextId|nextAnnotationId)";


\: syncGlyph (void; bool outline)
{
    if (!outline) return;
    let t = elapsedTime();

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glRotate(t * 180.0, 0, 0, 1);
    glColor(Color(.7, .7, .7, 1));

    for (float r = .5; r >= .1; r -= .15)
        drawCircleFan(0, 0, r, 0, 1, .3, true);

    glColor(Color(1, 1, 1, .3));
    drawCircleFan(0, 0, .5, .15, .35, .3, false);
    drawCircleFan(0, 0, .5, .65, .85, .3, false);
    
    glColor(Color(.25,.25,.25,1));
    glEnable(GL_POINT_SMOOTH);
    glPointSize(3);
    glBegin(GL_POINTS);
    glVertex(0,0);
    glEnd();
    glPopMatrix();
}

documentation: """
The RemoteSync mode tracks the remote cursor position, frame, and
play/stop status and other state.

The cursor position is relative to the viewed image's space not the
pixels. This makes it possible to sync RVs that have different window
sizes and viewing transforms.
"""

class: RemoteSync : MinorMode
{
    class: DelayedEvent
    {
        string eventName;
        string contents;
        string originator;
    }

    class: PointQueue
    {
        maxSize := 15;

        int     _head;
        int     _size;
        Point[] _points;

        \: _nextIndex (int; int index)
        {
            if (index == maxSize-1) then 0 else index+1;
        }

        method: _internalIndex (int; int externalIndex)
        {
            if (externalIndex < 0 || externalIndex > _size-1)
            {
                throw exception ("ERROR: PointQueue illegal index: %s" % externalIndex);
            }

            int ii = _head - externalIndex;

            return if (ii < 0) then (maxSize + ii) else ii;
        }

        method: size (int; )
        {
            return _size;
        }

        method: add (void; Point p)
        {
            _head = _nextIndex(_head);
            _points[_head] = p;

            if (_size != maxSize) ++_size;
        }

        method: removeOldest (void; )
        {
            if (_size > 0)  --_size;
        }

        method: elem (Point; int index)
        {
            return _points[_internalIndex(index)];
        }

        method: PointQueue (PointQueue; )
        {
            _head = 0;
            _size = 0;
            _points = Point[]();
            _points.resize(maxSize);
        }
    }

    class: SyncContact
    {
        string      _contact;
        string      _session;
        bool        _pointerActive;
        float       _pointerTime;
        string      _label;
        float       _twidth;
        float       _theight;
        string      _source;
        float       _x;
        float       _y;
        string[][]  _sourceMedia;
        int         _version;
        PointQueue  _pointerTrail;

        method: SyncContact (SyncContact; string c, string s, bool pa, string l, float tw, float th)
        {
            _contact = c;
            _session = s;
            _pointerActive = pa;
            _pointerTime = theTime();
            _label = l;
            _twidth = tw;
            _theight = th;

            _sourceMedia = string[][]();
            _pointerTrail = PointQueue();
        }

        method: PointerIsActive (bool; )
        {
            // First validate that the pointer is active
            if (!_pointerActive) return false;

            // Then validate its time to live
            if ((theTime() - _pointerTime) > POINTER_TIME_TO_LIVE)
            {
                _pointerActive = false;
            }

            return _pointerActive;
        }
    }

    
    SyncContact[]  _contacts;
    bool           _hasOlderContactVersions;
    bool           _networkActive;
    bool           _lockFrame;
    string         _lockFrameOriginator;
    bool           _lock;
    string         _lockOriginator;
    int            _receiveLockCount;
    bool           _viewChangeLock;
    [regex]        _sendPropList;
    [regex]        _blockPropList;
    string[]       _sendPipelineGroups;
    string[]       _blockPipelineGroups;
    bool           _sendTime;
    bool           _blockTime;
    bool           _sendMediaChanges;
    bool           _acceptMediaChanges;
    bool           _showFeedback;
    bool           _showContactsPointers;
    bool           _showPointerTrails;
    bool           _showContactErrors;
    bool           _broadcastExternalEvents;
    bool           _syncWithContacts;
    bool           _lockRange;
    string         _lockRangeOriginator;
    int            _labelSize;
    bool           _lockMark;
    string         _lockMarkOriginator;
    bool           _lockStateChange;
    string         _firstConnect;
    int            _accumulate;
    DelayedEvent[] _delayedEvents;
    float          _pointerTrailTime;
    SyncContact[]  _virtualContacts;

    method: receiveLockPush(void;)
    {
        ++_receiveLockCount;
        deb("receive lock pushed, now %s" % _receiveLockCount);
    }

    method: receiveLockPop(void;)
    {
        if (_receiveLockCount > 0) --_receiveLockCount;
        deb("receive lock popped, now %s" % _receiveLockCount);
    }

    method: receiveLockLocked(bool;)
    { 
        return (_receiveLockCount > 0);
    }

    method: receiveLockFilter(void; (void; Event) handler, Event event)
    {
        deb ("receiveLockFilter event %s lockCount %s" % (event.name(), _receiveLockCount));
        event.reject();

        if (receiveLockLocked()) return;

        handler(event);
    }

    // Determine whether or not a frame related event should be broadcasted
    // Return true if the frame related event should be sent to all the contacts
    // Otherwise return false.
    method: broadcastFrameEvent(bool;)
    { 
        // An event can be internal, that is originating from the current instance 
        // of RV or it can be external, that is originating from one of the contacts.
        // The current method makes sure that internal events are broadcasted to all 
        // the contacts, and external events are broadcasted to the contacts as long 
        // as _broadcastExternalEvents is enabled.
        // Note: _broadcastExternalEvents is enabled by default but it is automatically 
        // disabled when SyncWithContactsContacts is active. When SyncWithContactsContacts
        // is enabled then a full mesh network topology is formed with all the connected 
        // RVs. In this case we do not want external events to be rebroadcasted otherwise 
        // this would lead to mayhem as the same event would be broadcasted multiple 
        // times to all the contacts.
        // Note: _lockFrame is true when it originates from an external events
        return (!_lockFrame || _broadcastExternalEvents);
    }

    // Determine whether or not a play related event should be broadcasted
    // Return true if the play related event should be sent to all the contacts
    // Otherwise return false.
    method: broadcastPlayEvent(bool;)
    { 
        // Note: _lock is true when it originates from an external events
        return (!_lock || _broadcastExternalEvents);
    }

    // Determine whether or not a range related event should be broadcasted
    // Return true if the play related event should be sent to all the contacts
    // Otherwise return false.
    method: broadcastRangeEvent(bool;)
    { 
        // Note: _lock is true when it originates from an external events
        return (!_lockRange || _broadcastExternalEvents);
    }

    // Determine whether or not a mark related event should be broadcasted
    // Return true if the play related event should be sent to all the contacts
    // Otherwise return false.
    method: broadcastMarkEvent(bool;)
    { 
        // Note: _lock is true when it originates from an external events
        return (!_lockMark || _broadcastExternalEvents);
    }

    method: remoteEvalEach (void; string contents)
    {
        for_each (c; _contacts)
        {
            deb ("remoteEvalEach '%s'" % contents);
            remoteSendEvent("remote-sync-eval",
                            c._session,
                            "try {" + contents + "} catch (...) {;}",
                            //contents,
                            string[] {c._contact});
        }
    }

    method: broadcastEvalEach (void; string contents, string originator=nil)
    {
        for_each (c; _contacts)
        {
            // Do not send the event to the originator of the event if any was 
            // specified.
            if (originator eq nil || c._contact != originator)
            {
                deb ("broadcastEvalEach '%s'" % contents);
                remoteSendEvent("remote-sync-eval",
                                c._session,
                                contents,
                                string[] {c._contact});
            }
        }
    }

    method: remoteSyncEval (void; Event event)
    {
        receiveLockPush();

        let t = event.contents();
        deb ("remoteSyncEval '%s'" % t);

        try
        {
            event.setReturnContent(runtime.eval(t, ["commands"]));
        }
        catch (exception exc)
        {
            let s = "ERROR: remote-sync-eval: %s\nERROR: remote-sync-eval: '%s'\n" % (exc, t);
            print(s);
            event.setReturnContent(s);
        }

        receiveLockPop();

        if (_broadcastExternalEvents)
        {
            broadcastEvalEach (t, event.sender());
        }
    }

    method: sendEach (void; 
                      string eventName,
                      string contents,
                      string originator=nil,
                      bool force=false,
                      int minVersion=0)
    {
        if (viewNode() != "" || force)
        {
            if (_accumulate > 0)
            {
                //
                //  When accumulating, concatenate the contents onto the
                //  last delayed event if they match. Otherwise, just make
                //  a new delayed event.
                //

                if (!_delayedEvents.empty() && 
                    _delayedEvents.back().eventName == eventName &&
                    (eventName == "remote-sync-graph-state" ||
                     eventName == "remote-sync-new-property" ||
                     eventName == "remote-sync-delete-property"))
                {
                    let last = _delayedEvents.back();
                    last.contents = "%s&%s" % (last.contents, contents);
                }
                else
                {
                    _delayedEvents.push_back(DelayedEvent(eventName, contents, originator));
                }
            }
            else
            {
                //
                //  Add our name and session to each event so the other sync
                //  mode can correctly identify us. 
                //

                let outContents = "%s|%s|%s" % (contents, sessionName(), if (originator neq nil) then originator else "");
                
                for_each (c; _contacts)
                {
                    // Respect the minimim version and do not send the event 
                    // to the originator of the event if any was specified.
                    if ( c._version >= minVersion  && 
                         (originator eq nil || c._contact != originator) )
                    {
                        deb ("sendEach: sending '%s' to %s" % (eventName, c._contact));
                        remoteSendEvent(eventName,
                                        c._session,
                                        outContents,
                                        string[] {c._contact});
                    }
                }
            }
        }
    }
    
    method: findSyncContact (SyncContact; string name, string session="")
    {
        for_each (c; _contacts)
        {
            if (c._contact == name && (c._session == session || session == ""))
            {
                return c;
            }
        }

        return nil;
    }

    method: findVirtualSyncContact (SyncContact; string name, string session="")
    {
        for_each (c; _virtualContacts)
        {
            if (c._contact == name && (c._session == session || session == ""))
            {
                return c;
            }
        }

        return nil;
    }

    method: addBlockPattern (void; regex pattern)
    {
        for_each (r; _blockPropList) if (r eq pattern) return;
        _blockPropList = pattern : _blockPropList;
    }

    method: removeBlockPattern (void; regex pattern)
    {
        [regex] newList;
        for_each (r; _blockPropList) 
            if (r neq pattern) newList = r : newList;
        _blockPropList = newList;
    }

    method: addSendPattern (void; regex pattern)
    {
        for_each (r; _sendPropList) if (r eq pattern) return;
        _sendPropList = pattern : _sendPropList;
    }

    method: removeSendPattern (void; regex pattern)
    {
        [regex] newList;
        for_each (r; _sendPropList) 
            if (r neq pattern) newList = r : newList;
        _sendPropList = newList;
    }

    method: hasBlockPattern (bool; regex pattern)
    {
        for_each (r; _blockPropList) if (r eq pattern) return true;
        return false;
    }

    method: hasSendPattern (bool; regex pattern)
    {
        for_each (r; _sendPropList) if (r eq pattern) return true;
        return false;
    }

    method: modeToggled (void; Event event)
    {
        deb ("modeToggled %s %s" % (event.contents(), event.sender()));

        sendEach("remote-mode-toggled", event.contents());
    }

    method: syncModeToggled (void; Event event)
    {
        let c = event.contents().split("|"),
            m = c[0],
            a = bool(c[1]);

        deb ("syncModeToggled %s %s m '%s' a %s" % (event.contents(), event.sender(), m, a));

        receiveLockPush();
        try 
        {
            if (m == "wipe")
            {
                State state = data();
                if      (state.wipe neq nil && a != state.wipe._active) state.wipe.toggle();
                else if (state.wipe eq nil && a) rvui.toggleWipe();
            }
        }
        catch (...) {;}

        receiveLockPop();
    }

    method: move (void; Event event)
    {
        event.reject();

        State state = data();
        recordPixelInfo(event);

        if (state.pixelInfo eq nil || state.pixelInfo.empty()) return;

        let pinfo  = state.pixelInfo.front(),
            ip     = state.pointerPosition;

        //
        //  NOTE: this is MY pointer position, not the remote one
        //

        if ("" != pinfo.name)
        {
            let name = sourceNameWithoutFrame(pinfo.name).split("/").front(),
                normalizedIP = eventToImageSpace(name, ip, true);

            sendEach("remote-sync-pointer", "%s;%f;%f" % (name, normalizedIP.x, normalizedIP.y));
        }
    }

    method: leave (void; Event event)
    {
        sendEach("remote-sync-leave", "-");
        event.reject();
    }

    method: enter (void; Event event)
    {
        sendEach("remote-sync-enter", "-");
        event.reject();
    }

    method: frameChanged (void; Event event)
    {
        //
        //  Only pass on to sync partners if we're not playing, we're not in
        //  this code as the result of responding to a sync partner, and we're
        //  not in "turn-around" (looping etc).
        //
        if (!isPlaying() && broadcastFrameEvent() && event.contents() != "turn-around" && event.contents() != "buffering") 
        {
            string f = string(frame());
            State state = data();
            if (state.pushed) f = f + ";;scrubbed";
            sendEach("remote-sync-frame-changed", f, _lockFrameOriginator);
        }

        event.reject();
    }
                                                
    method: playStart (void; Event event)
    {
        //
        //  Only pass on to sync partners if we're not in this code as the
        //  result of responding to a sync partner, and we're not in
        //  "turn-around" (looping etc).
        //
        deb ("received play-start: lock %s, turn-around %s" %
                (_lockFrame, event.contents() == "turn-around", event.contents() == "buffering"));

        if (broadcastFrameEvent() && event.contents() != "turn-around" && event.contents() != "buffering") 
        {
            sendEach("remote-sync-play-start", string(frame()), _lockFrameOriginator);
        }
        event.reject();
    }

    method: playStop (void; Event event)
    {
        //
        //  Only pass on to sync partners if we're not in this code as the
        //  result of responding to a sync partner, and we're not in
        //  "turn-around" (looping etc).
        //
        deb ("received play-stop: lock %s, turn-around %s buffering %s" %
                (_lockFrame, event.contents() == "turn-around", event.contents() == "buffering"));

        if (broadcastFrameEvent() && event.contents() != "turn-around" && event.contents() != "buffering") 
        {
            sendEach("remote-sync-play-stop", string(frame()), _lockFrameOriginator);
        }
        event.reject();
    }

    method: playInc (void; Event event)
    {
        deb ("received play-inc: framelock %s, lock %s, playing %s" %
                (_lockFrame, _lock, isPlaying()));
        if (broadcastPlayEvent()) 
        {
            bool playing = isPlaying();
            if (playing) stop();
            sendEach("remote-sync-play-inc", string(inc()), _lockOriginator);
            if (playing) play();
        }

        event.reject();
    }

    method: newInPoint (void; Event event)
    {
        if (broadcastRangeEvent() && !_viewChangeLock) sendEach("remote-sync-new-in-point", string(inPoint()), _lockRangeOriginator);
    }

    method: newOutPoint (void; Event event)
    {
        if (broadcastRangeEvent() && !_viewChangeLock) sendEach("remote-sync-new-out-point", string(outPoint()), _lockRangeOriginator);
    }

    method: newMarkFrame (void; Event event)
    {
        if (broadcastMarkEvent()) sendEach("remote-sync-mark-frame", event.contents(), _lockMarkOriginator);
    }

    method: removeMarkFrame (void; Event event)
    {
        if (broadcastMarkEvent()) sendEach("remote-sync-unmark-frame", event.contents(), _lockMarkOriginator);
    }

    method: parseGreeting (Event event)
    {
        let parts = event.contents().split("|");
        event.reject();
        (parts[0], parts[1]);
    }

    method: parseSimpleContents ((string, SyncContact); string contents, Event event)
    {
        let parts = contents.split("|");
        assert(parts.size() >= 2);
        event.reject();
        (parts[0], findSyncContact(event.sender(), parts[1]));
    }

    method: parseSimpleEventContents ((string, SyncContact); Event event)
    {
        event.reject();
        parseSimpleContents(event.contents(), event);
    }

    method: parseContents ((string, SyncContact, string); string contents, Event event)
    {
        let parts = contents.split("|");
        assert(parts.size() >= 2);
        event.reject();

        // Extract the contact (SyncContact)
        let contact = findSyncContact(event.sender(), parts[1]);

        // Extract the originator (string) if any was specified (optional)
        let originator = if (parts.size()>2) then parts[2] else nil;

        // If no originator was specified then fall make it the contact's name
        if ((originator eq nil) || (originator=="") || (originator == "nil"))
        {
            originator = if (contact neq nil) then contact._contact else "Unknown";
        }

        return (parts[0], contact, originator);
    }

    method: parseEventContents ((string, SyncContact, string); Event event)
    {
        event.reject();
        parseContents(event.contents(), event);
    }

    method: parseCompoundEventContents ((string[], SyncContact); Event event)
    {
        let parts = event.contents().split("|");
        assert(parts.size() >= 2);
        event.reject();
        (parts[0].split("&"), findSyncContact(event.sender(), parts[1]));
    }

    method: greetings (void; Event event)
    {
        let c = event.sender(),
            s = event.contents();

        deb ("**** greetings from %s %s" % (c, s));
        if (findSyncContact(c, s) eq nil)
        {
            gltext.size(_labelSize);
            let label = c.split("@").front(),
                b     = gltext.bounds(label),
                tw    = b[2],
                th    = b[3];

            _contacts.push_back(SyncContact(c, s, false, label, tw, th));

            try
            {
                commands.logMetricsWithProperties("Using Sync Review", "{\"type\":\"Connected to Session\"}");
            }
            catch (...)
            {
                try
                {
                    commands.logMetrics("Using Sync Review");
                }
                catch (...) {;}
            }

            deb ("**** added contact, sending sync-greeting");

            remoteSendEvent("remote-sync-greeting",
                            s,
                            sessionName(),
                            string[] {c});

            remoteSendEvent("remote-sync-version",
                            s,
                            "%d|%s" % (SYNC_VERSION_CURRENT, sessionName()),
                            string[] {c});

            if (_contacts.size() == 1 && ! remoteConnectionIsIncoming(c))
            {
                // request the initial state of the session after the first connection only
                remoteSendEvent("remote-pull-session",
                            s,
                            sessionName(),
                            string[] {c});
            }

            if (_syncWithContacts)
            {
                remoteSendEvent("connect-remote-friends",
                                s,
                                "%s&%d|%s" % (myNetworkHost(),myNetworkPort(),sessionName()),
                                string[] {c});
            }
        }

        displayFeedback("Sync ON", 4, syncGlyph);

        //sendSources();
        event.reject();
    }

    method: quitSync (void; Event event)
    {
        toggle();
    }

    method: connectionStop (void; Event event)
    {
        let sender = event.contents(),
            contactToRemove = -1;

        deb ("connectionStop ************* %s contacts" % _contacts.size());
        for_index (i; _contacts)
        {
            let c = _contacts[i];

            deb ("comparing c._contact '%s' sender '%s'" % (c._contact, sender));
            if (c._contact == sender) contactToRemove = i;
        }
        if (contactToRemove != -1)
        {
            let c = _contacts.back();

            _contacts[contactToRemove] = c;
            _contacts.pop_back();
        }
        deb ("connectionStop ************* done %s contacts" % _contacts.size());
        if (_contacts.size() == 0) quitSync(event);
        event.reject();
    }

    method: goodbye (void; Event event)
    {
        let sender = event.sender(),
            contents = event.contents();

        for_index (i; _contacts)
        {
            let c = _contacts[i];

            if (c._contact == sender && c._session == contents)
            {
                _contacts[i] = _contacts.back();
                _contacts.pop_back();
                break;
            }
        }
        if (_contacts.size() == 0) quitSync(event);
        event.reject();
    }

    method: syncVersion (void; Event event)
    {
        let (contents, c) = parseSimpleEventContents(event);

        if (c neq nil)
        {
            c._version = int(contents);
            if (c._version < SYNC_VERSION_CURRENT) _hasOlderContactVersions = true;
        }
        else
        {
            print("SYNC ERROR: GOT VERSION BUT NO CONTACT \"%s\" from %s\n" 
                  % (contents, c));
        }

        if (_firstConnect neq nil)
        {
            //  If we're connecting for the first time, it's time to
            //  push/pull session data if that was requested on the
            //  command line.
            //
            deb ("_firstConnect %s" % _firstConnect);
            if      (_firstConnect == "pull") pullSession(nil);
            else if (_firstConnect == "push") pushSessionToAll(nil);
            _firstConnect = nil;
        }

        checkSyncVersion(c._version);

        event.reject();
    }

    method: syncPointer (void; Event event)
    {
        let (contents, c, originator) = parseEventContents(event),
            parts   = contents.split(";"),
            source  = "",
            x       = 0.0,
            y       = 0.0,
            broadcastEvent = false;

        if (parts.size() > 2)
        {
            source  = parts[0].split("/").front();
            x       = float(parts[1]);
            y       = float(parts[2]);

            if (c neq nil && "" != source)
            {
                // Determine the contact to use for the pointer 
                if (originator == c._contact)
                {
                    // Typical case: 
                    // The pointer event originates from a known contact
                    // We can broadcast this event since it originates from a contact.
                    broadcastEvent = _broadcastExternalEvents;
                }
                else
                {
                    // Special case: 
                    // The originator of the remote-sync-pointer is NOT one of
                    // the contacts.
                    // Note: this happens when broadcasting external events 
                    // (leaf to leaf mode), when a leaf pointer is to be updated 
                    // on all the other leaves.
                    // In this case we'll use a virtual contact to hold the 
                    // pointer information
                    let vc = findVirtualSyncContact(originator, c._session);
                    if (vc eq nil)
                    {
                        gltext.size(_labelSize);
                        let label = originator.split("@").front(),
                            b     = gltext.bounds(label),
                            tw    = b[2],
                            th    = b[3];

                        vc = SyncContact(originator, c._session, false, label, tw, th);
                        _virtualContacts.push_back(vc);
                    }
                    c = vc;
                }

                c._source        = source;
                c._x             = x;
                c._y             = y;
                c._pointerActive = true;
                c._pointerTime   = theTime();

                if (_showPointerTrails) c._pointerTrail.add (Point(x,y));

                redraw();

                if (broadcastEvent) 
                {
                    sendEach("remote-sync-pointer", "%s;%f;%f" % (source, x, y), event.sender());
                }
            }
        }

        event.reject();
    }

    method: syncEnter (void; Event event)
    {
        let (_, c) = parseSimpleEventContents(event);
        if (c neq nil) c._pointerActive = true;
        event.reject();
    }

    method: syncLeave (void; Event event)
    {
        let (_, c) = parseSimpleEventContents(event);
        if (c neq nil) c._pointerActive = false;
        event.reject();
    }

    method: niceName (string; string contact)
    {
        let parts = contact.split("@"),
            c = contact;

        if (parts.size() == 2 && (parts[1] == "localhost" || parts[1] == "127.0.0.1"))
        {
            c = parts[0];
        }
        return c;
    }

    method: syncFrameChanged (void; Event event)
    {
        if (!_lockFrame)
        {
            let (f, c, originator) = parseEventContents(event),
                frame    = int(f),
                parts    = f.split(";;"),
                scrubbed = if (parts.size() > 1) then (parts[1] == "scrubbed") else false;

            if (frame != frame() && c neq nil)
            {
                deb ("received remote-sync-frame-changed %s: playing %s buffering %s" % (frame, isPlaying(), isBuffering()));
                _lockFrame = true;
                _lockFrameOriginator = event.sender();
                setFrame(frame);
                _lockFrameOriginator = nil;
                _lockFrame = false;

                if (scrubbed)
                {
                    State state = data();
                    if (state.scrubAudio) scrubAudio(true, 1.0 / fps(), 1);
                    else scrubAudio(false);
                }

                if (_showFeedback)
                {
                    displayFeedback("%s Changed Frame to %d" % (niceName(originator), frame),
                                    1.0,
                                    syncGlyph);
                }
            }
        }
    }

    method: syncPlayStart (void; Event event)
    {
        deb ("received remote-sync-play-start: lock %s, playing %s, buffering %s" %
                (_lockFrame, isPlaying(), isBuffering()));
        if (!isPlaying() && !_lockFrame && !isBuffering()) 
        {
            let (contents, c, originator) = parseEventContents(event),
                f                         = int(contents);

            _lockFrame = true;
            _lockFrameOriginator = event.sender();
            setFrame(f);
            togglePlayVerbose(_showFeedback);
            _lockFrameOriginator = nil;
            _lockFrame = false;

            if (_showFeedback)
            {
                let g = if isPlayingForwards() 
                            then xformedGlyph(triangleGlyph, 180) 
                            else triangleGlyph;

                displayFeedback("PLAY - %s" % niceName(originator), 2, g);
            }
            if (!isPlaying() && !isBuffering()) play();
        }
    }

    method: syncPlayStop (void; Event event)
    {
        deb ("received remote-sync-play-stop: lock %s, playing %s" %
                (_lockFrame, isPlaying()));
        if (isPlaying() || isBuffering())
        {
            let (contents, c, originator) = parseEventContents(event),
                f                         = int(contents);

            _lockFrame = true;
            _lockFrameOriginator = event.sender();
            togglePlayVerbose(_showFeedback);
            setFrame(f);
            _lockFrameOriginator = nil;
            _lockFrame = false;

            if (_showFeedback) displayFeedback("STOP - %s" % niceName(originator), 2, squareGlyph);

            if (isPlaying() || isBuffering()) stop();
        }
    }

    method: syncPlayInc (void; Event event)
    {
        deb ("received remote-sync-play-inc: lock %s, playing %s" %
                (_lockFrame, isPlaying()));
        let (contents, _) = parseSimpleEventContents(event);
        _lock = true;
        _lockOriginator = event.sender();
        setInc(int(contents));
        _lockOriginator = nil;
        _lock = false;
    }

    method: syncStop (void; Event event)
    {
        _networkActive = false;
    }

    method: syncStart (void; Event event)
    {
        _networkActive = true;
    }

    method: syncNewInPoint (void; Event event)
    {
        let (contents, c, originator) = parseEventContents(event),
            f                         = int(contents);

        deb ("received remote-sync-in-point %s: playing %s buffering %s" % (f, isPlaying(), isBuffering()));
        _lockRange = true;
        _lockRangeOriginator = event.sender();
        setInPoint(f);
        _lockRangeOriginator = nil;
        _lockRange = false;

        if (_showFeedback) displayFeedback("%s Set IN Point" % niceName(originator),
                                           1,
                                           syncGlyph);

    }

    method: syncNewOutPoint (void; Event event)
    {
        let (contents, c, originator) = parseEventContents(event),
            f                         = int(contents);

        deb ("received remote-sync-out-point %s: playing %s buffering %s" % (f, isPlaying(), isBuffering()));
        _lockRange = true;
        _lockRangeOriginator = event.sender();
        setOutPoint(f);
        _lockRangeOriginator = nil;
        _lockRange = false;

        if (_showFeedback) displayFeedback("%s Set OUT Point" % niceName(originator),
                                           1,
                                           syncGlyph);
    }

    method: syncNewMarkFrame (void; Event event)
    {
        let (contents, c, originator) = parseEventContents(event),
            f                         = int(contents);

        _lockMark = true;
        _lockMarkOriginator = event.sender();
        markFrame(f, true);
        _lockMarkOriginator = nil;
        _lockMark = false;

        if (_showFeedback) displayFeedback("%s Marked Frame %d" % (niceName(originator), f),
                            1,
                            syncGlyph);
    }

    method: syncRemoveMarkFrame (void; Event event)
    {
        let (contents, c, originator) = parseEventContents(event),
            f                         = int(contents);

        _lockMark = true;
        _lockMarkOriginator = event.sender();
        markFrame(f, false);
        _lockMarkOriginator = nil;
        _lockMark = false;

        if (_showFeedback) displayFeedback("%s Unmarked Frame %d" % (niceName(originator), f),
                            1,
                            syncGlyph);
    }

    method: syncSources (void; Event event)
    {
        //
        //  NOTE: this whole thing will fail if filenames have a '|' a
        //  ';' or a '\n' in them since we're using those as
        //  separators. It might better to use a control character.
        //

        //
        //  When we start sync early, while still loading sources, 
        //  this section throws, but later events will match
        //  everything up, so just ignore errors here.
        //
        try
        {
        let (contents, c) = parseSimpleEventContents(event),
            sources       = contents.split("\n");

        c._sourceMedia = string[][]();
        
        for_each (s; sources)
        {
            c._sourceMedia.push_back(string[]());

            for_each (m; s.split(";"))
            {
                if (m != "") c._sourceMedia.back().push_back(m);
            }
        }
        }
        catch (...) {;}

        if (_firstConnect neq nil)
        {
            //  If we're connecting for the first time, it's time to
            //  push/pull session data if that was requested on the
            //  command line.
            //
            deb ("_firstConnect %s" % _firstConnect);
            if      (_firstConnect == "pull") pullSession(nil);
            else if (_firstConnect == "push") pushSessionToAll(nil);
            _firstConnect = nil;
        }
    }

    method: syncPlayAllFrames (void; Event event)
    {
        _lock = true;
        _lockOriginator = event.sender();
        setRealtime(false);
        _lockOriginator = nil;
        _lock = false;
    }

    method: syncRealtime (void; Event event)
    {
        _lock = true;
        _lockOriginator = event.sender();
        setRealtime(true);
        _lockOriginator = nil;
        _lock = false;
    }

    method: syncPlayMode (void; Event event)
    {
        let (contents, c) = parseSimpleEventContents(event);

        _lock = true;
        _lockOriginator = event.sender();
        setPlayMode(int(contents));
        _lockOriginator = nil;
        _lock = false;
    }


    method: syncFps (void; Event event)
    {
        let (contents, c) = parseSimpleEventContents(event);

        _lock = true;
        _lockOriginator = event.sender();
        setFPS(float(contents));
        _lockOriginator = nil;
        _lock = false;
    }

    method: syncReadLutComplete (void; Event event)
    {
        let (contents, c) = parseSimpleEventContents(event);

        deb ("received remote-sync-read-lut-complete %s" % contents);

        let args   = contents.split(";;"),
            lut    = args[0],
            node   = args[1],
            pgroup = pipelineGroupTypeOfNode(node);

        if (acceptPipelineGroupAllowed(pgroup))
        {
            receiveLockPush();
            try 
            { 
                readLUT(lut, node); 
            } 
            catch (...) {;}
            receiveLockPop();
        }
    }

    method: sourcesChanged (void; Event event)
    {
        //sendSources();
        event.reject();
    }

    method: mediaAddedTo (void; Event event)
    {
        deb ("media added %s" % event.contents());

        if (!_sendMediaChanges) return;

            let args      = event.contents().split(";;"),
                source    = args[0],
                media     = args[2],
                newMedia  = redoPathSwapVars (media);

            remoteEvalEach ("""
                require sync; 
                if (sync.acceptingMediaChanges()) 
                commands.addToSource(\"%s\", \"%s\", \"sync\");
                """ 
                % (source, newMedia));
        }

    method: sourceMediaRelocated (void; Event event)
    {
        deb ("media relocated %s" % event.contents());

        if (!_sendMediaChanges) return;

            let args      = event.contents().split(";;"),
                source    = args[0],
                oldM      = args[1],
                newM      = args[2],
                newMedia  = redoPathSwapVars (newM);

            remoteEvalEach ("""
                require sync; 
                if (sync.acceptingMediaChanges()) 
                commands.relocateSource(\"%s\", \"%s\", \"%s\");
                """ 
                % (source, oldM, newMedia));
        }

    method: sourceMediaSet (void; Event event)
    {
        deb ("media set %s" % event.contents());

        if (!_sendMediaChanges) return;

            let args      = event.contents().split(";;"),
                source    = args[0],
                media = getStringProperty("%s.media.movie" % source),
                newMedia = string[]();

            for_each (m; media)
            {
                newMedia.push_back (redoPathSwapVars (m));
                deb ("    %s -> %s" % (m, newMedia.back()));   
            }
            remoteEvalEach ("""
                require sync; 
                if (sync.acceptingMediaChanges()) 
                {
                    let cmode = cacheMode();
                    commands.setCacheMode(commands.CacheOff);
                    commands.setSourceMedia(\"%s\", %s, \"sync\");
                    commands.setCacheMode(cmode);
                }
                """ 
                % (source, newMedia));
        }

    method: sessionClearEverything (void; Event event)
    {
        deb ("session cleared");
            remoteEvalEach ("require rvui; rvui.clearEverything();");
        }

    method: fpsChanged (void; Event event)
    {
        if (broadcastPlayEvent()) sendEach("remote-sync-fps", string(fps()), _lockOriginator, true /*force*/);
        event.reject();
    }

    method: playAllFramesMode (void; Event event)
    {
        if (broadcastPlayEvent()) sendEach("remote-sync-play-all-frames", "", _lockOriginator, true /*force*/);
        event.reject();
    }

    method: realtimePlayMode (void; Event event)
    {
        if (broadcastPlayEvent()) sendEach("remote-sync-realtime", "", _lockOriginator, true /*force*/);
        event.reject();
    }

    method: playModeChanged (void; Event event)
    {
        if (broadcastPlayEvent()) sendEach("remote-sync-play-mode", "%s" % playMode(), _lockOriginator, true /*force*/);
        event.reject();
    }

    method: readLutComplete (void; Event event)
    {
        deb ("readLutComplete %s %s" % (event.contents(), event.sender()));

        let args   = event.contents().split(";;"),
            lut    = args[0],
            node   = args[1],
            pgroup = pipelineGroupTypeOfNode(node),
            newLut = redoPathSwapVars (lut);

        if (sendPipelineGroupAllowed(pgroup))
        {
            sendEach("remote-sync-read-lut-complete", newLut + ";;" + node);
        }
    }

    method: syncError (void; Event event)
    {
        let badContact = event.contents();
        int badIndex = -1;

        for_index (i; _contacts)
        {
            if (_contacts[i]._contact == badContact) badIndex = i;
        }

        if (badIndex != -1) _contacts.erase(badIndex, 1);
        if (_contacts.empty()) toggle();
    }

    method: syncErrorShow (void; Event event)
    {
        string showContactError = "yes";
        if (!_showContactErrors) showContactError = "no";
        event.setReturnContent(showContactError);
    }

    method: handleNewProperty (void; Event event)
    {
        deb ("newProperty %s rl %s" % (event.contents(), _receiveLockCount));

        let args    = event.contents().split(";"),
            type    = args[0],
            dims    = args[1].split(","),
            ndims   = dims.size(),
            xsize   = int(dims[0]),
            ysize   = int(if ndims > 1 then int(dims[1]) else 0),
            zsize   = int(if ndims > 1 then int(dims[2]) else 0),
            wsize   = int(if ndims > 1 then int(dims[3]) else 0),
            prop    = args.back(),
            parts   = prop.split("."),
            node    = parts[0],
            ntype   = nodeType(node),
            info    = propertyInfo(prop),
            altprop = "#%s.%s.%s" % (ntype, parts[1], parts[2]);

        string out;

        for_each (r; _sendPropList)
        {
            if (r.match(prop) || r.match(altprop))
            {
                out = "%s/%s/" % (prop, altprop);
                string fmt = nil;

                case (type)
                {
                    "float"  -> { fmt = "newNDProperty(\"%s\", FloatType, (%d,%d,%d,%d))"; }
                    "int"    -> { fmt = "newNDProperty(\"%s\", IntType, (%d,%d,%d,%d))"; }
                    "string" -> { fmt = "newNDProperty(\"%s\", StringType, (%d,%d,%d,%d))"; }
                    "byte"   -> { fmt = "newNDProperty(\"%s\", ByteType, (%d,%d,%d,%d))"; }
                }

                out += fmt % (prop, xsize, ysize, zsize, wsize);

                sendEach("remote-sync-new-property", out);
            }
        }
    }

    method: deletedProperty (void; Event event)
    {
        let prop = event.contents();

        for_each (r; _sendPropList)
        {
            if (r.match(prop))
            {
                //
                //  We don't care if this fails so just tell it to
                //  catch on the other end
                //

                sendEach("remote-sync-delete-property", 
                            "try { deleteProperty(\"%s\") } catch (...) {;}" % prop);
            }
        }
    }

    method: nodeDeleted (void; Event event)
    {
        let name = event.contents();

        remoteEvalEach ("deleteNode(\"%s\");" % name);
    }

    method: nodeInputsChanged (void; Event event)
    {
        let name = event.contents(),
            type = nodeType(name);

        if (type != "RVDisplayGroup" && type != "Root")
        {
            let inputs = nodeConnections(name, false)._0;

            remoteEvalEach ("setNodeInputs(\"%s\", %s);" % (name, inputs));
        }
    }

    method: viewNodeChangeInit (void; Event event)
    {
        _viewChangeLock = true;
    }

    method: viewNodeChange (void; Event event)
    {
        _viewChangeLock = false;

        let name = event.contents();

        remoteEvalEach ("setViewNode(\"%s\");" % name);
    }

    method: newNode (void; Event event)
    {
        let name = event.contents(),
            t = nodeType(name);

        if (t != "RVFileSource")
        {
            remoteEvalEach ("commands.newNode(\"%s\", \"%s\");" % (t, name));
        }
        else 
        {
            deb ("source added %s" % name);
            let media = getStringProperty("%s.media.movie" % name),
                newMedia = string[]();

            for_each (m; media)
            {
                newMedia.push_back (redoPathSwapVars (m));
                deb ("    %s -> %s" % (m, newMedia.back()));   
            }
            remoteEvalEach ("""
                {
                    let cmode = cacheMode();
                    commands.setCacheMode(commands.CacheOff);
                    commands.addSource(%s, \"sync\");
                    commands.setCacheMode(cmode);
                }
                    """ % newMedia);
        }
    }

    method: pipelineGroupTypeOfNode(string; string nodeName)
    {
        let pgt = "";

        while (true)
        {
            pgt = if (nodeName neq nil) then nodeType(nodeName) else "";

            let match = regex(".*PipelineGroup$").match(pgt);

            if (nodeName eq nil || match) break;

            nodeName = nodeGroup(nodeName);
        }

        return pgt;
    }

    method: sendPipelineGroupAllowed (bool; string pgType)
    {
        if (pgType == "") return true;

        for_each (g; _sendPipelineGroups)
        {
            if ((g == "General" && pgType == "RVPipelineGroup") ||
                pgType == "RV" + g + "PipelineGroup")
            {
                return true;
            }
        }
        return false;
    }

    method: acceptPipelineGroupAllowed (bool; string pgType)
    {
        if (pgType == "") return true;

        for_each (g; _blockPipelineGroups)
        {
            if ((g == "General" && pgType == "RVPipelineGroup") ||
                pgType == "RV" + g + "PipelineGroup")
            {
                return false;
            }
        }
        return true;
    }

    method: stateChange (void; Event event)
    {
        deb ("stateChange cont '%s' rl %s lsc %s" % (event.contents(), _receiveLockCount, _lockStateChange));

        if (_lockStateChange)
        {
            _lockStateChange = false;
            return;
        }

        let prop    = event.contents(),
            parts   = prop.split("."),
            node    = parts[0],
            pgroup  = pipelineGroupTypeOfNode(node),
            ntype   = nodeType(node),
            info    = propertyInfo(prop),
            altprop = "#%s.%s.%s" % (ntype, parts[1], parts[2]);

        deb ("stateChange prop '%s' altpop '%s' pgrouptype '%s'" % (prop, altprop, pgroup));

        let doSend      = false,
            noStateProp = true,
            maxProp     = false;

        for_each (r; _sendPropList)
        {
            if ((r.match(prop) || r.match(altprop)))
            {
                doSend = true;

                if (!(MatchNoStateChange.match(prop) || MatchNoStateChange.match(altprop)))
                {
                    noStateProp = false;
                }
                else if (MatchMaxStateChange.match(prop) || MatchMaxStateChange.match(altprop))
                {
                    maxProp = true;
                }
                break;
            }
        }

        //
        //  Override above decision if this prop is in pipeline group, and user
        //  has asked to sync this group.  This allows syncing of arbitrary
        //  node types and properties in pipeline groups.
        //

        if (pgroup != "" && sendPipelineGroupAllowed(pgroup))
        {
            doSend = true;
            noStateProp = false;
            maxProp = false;
        }

        if (doSend)
        {
            string out = "%s/%s/" % (prop, altprop);

            if (! noStateProp)
            {
                case (info.type)
                {
                    FloatType ->
                    {
                        out += "setFloatProperty(\"%s\", %s, true)" %
                            (prop, getFloatProperty(prop));
                    }

                    HalfType ->
                    {
                        out += "setHalfProperty(\"%s\", %s, true)" %
                            (prop, getHalfProperty(prop));
                    }

                    IntType ->
                    {
                        out += "setIntProperty(\"%s\", %s, true)" %
                            (prop, getIntProperty(prop));
                    }

                    StringType ->
                    {
                        out += "setStringProperty(\"%s\", %s, true)" %
                            (prop, getStringProperty(prop));
                        if (altprop == "#RVDisplayStereo.stereo.type")
                        {
                            out += "; setHardwareStereoMode(%s);" % string(regex.match("hardware", getStringProperty(prop)[0]));
                        }
                    }

                    ByteType ->
                    {
                        out += "setByteProperty(\"%s\", %s, true)" %
                            (prop, getByteProperty(prop));
                    }
                }

                deb ("    sending '%s'" % out);
                sendEach("remote-sync-graph-state", out);
            }
            else if (maxProp)
            {
                if (info.type == IntType)
                {
                    //
                    //  Only for ints so far.
                    //
                    out += "setIntProperty(\"%s\", int[] { math.max(getIntProperty(\"%s\").front()+1,%s) }, true)" %
                        (prop, prop, getIntProperty(prop).front());

                    deb ("    sending '%s'" % out);
                    sendEach("remote-sync-graph-state", out);
                }
            }
        }
    }

    method: stateWillInsert (void; Event event)
    {
        deb ("stateWillInsert '%s'" % event.contents());
        _lockStateChange = true;
    }

    method: stateInsert (void; Event event)
    {
        let pload   = event.contents().split(";"),
            prop    = pload[0],
            index   = int(pload[1]),
            n       = int(pload[2]),
            parts   = prop.split("."),
            node    = parts[0],
            ntype   = nodeType(node),
            info    = propertyInfo(prop),
            altprop = "#%s.%s.%s" % (ntype, parts[1], parts[2]);

        string out;

        for_each (r; _sendPropList)
        {
            if (r.match(prop) || r.match(altprop))
            {
                out = "%s/%s/" % (prop, altprop);

                case (info.type)
                {
                    FloatType ->
                    {
                        out += "insertFloatProperty(\"%s\", %s)" % 
                            (prop, getFloatProperty(prop, index, n));
                    }

                    IntType ->
                    {
                        out += "insertIntProperty(\"%s\", %s)" % 
                            (prop, getIntProperty(prop, index, n));
                    }

                    StringType ->
                    {
                        out += "insertStringProperty(\"%s\", %s)" % 
                            (prop, getStringProperty(prop, index, n));
                    }

                    HalfType ->
                    {
                        out += "insertHalfProperty(\"%s\", %s)" % 
                            (prop, getHalfProperty(prop, index, n));
                    }

                    ByteType ->
                    {
                        out += "insertByteProperty(\"%s\", %s)" % 
                            (prop, getByteProperty(prop, index, n));
                    }
                }

                sendEach("remote-sync-graph-state", out, nil /*originator*/, false /*force*/, 2 /*minVersion*/);
            }
        }

        _lockStateChange = false;
    }

    method: syncState (void; Event event)
    {
        deb ("syncState cont '%s'" % event.contents()); 
        let (contentsArray, c) = parseCompoundEventContents(event);

        for_each (contents; contentsArray)
        {
            let parts         = contents.split("/"),
                propName      = parts[0],
                propParts     = propName.split("."),
                node          = propParts[0],
                pgroup        = pipelineGroupTypeOfNode(node),
                altPropName   = parts[1];

            string[] cmdParts = string[]();
            for (int i = 2; i < parts.size(); i++)
            {
                cmdParts.push_back(parts[i]);
            }
            string cmd = string.join(cmdParts, "/");

            if (viewNode() != "")
            {
                for_each (r; _blockPropList)
                {
                    if (r.match(propName) || r.match(altPropName)) return;
                }
                if (! acceptPipelineGroupAllowed(pgroup)) return;

                receiveLockPush();

                try
                {
                    deb ("    syncState eval"); 
                    runtime.eval(cmd, ["commands"]);
                }
                catch (...)
                {
                    // just ignore it
                    propName += " -- (IGNORED)";
                }

                redraw();
                receiveLockPop();

                if (_showFeedback)
                {
                    displayFeedback("%s : %s" % (niceName(c._contact), propName.split(".").back()),
                                    1,
                                    syncGlyph);
                }
            }
        }
        deb ("    syncState cont '%s' done" % event.contents()); 
        let contentsParts = event.contents().split("|");
        sendEach("remote-sync-graph-state", contentsParts[0], event.sender() /*originator*/, false /*force*/, 2 /*minVersion*/);
    }

    method: sessionAsByteArray (byte[]; )
    {
        let tmpDir = cacheDir(),
            target = tmpDir + "/" + "SessionToRemote.rv";

        //  Save session that is compressed binary, but _does_ contain all
        //  property values.
        //
        saveSession(target, true, true, false);

        let f = ifstream(target, stream.In | stream.Binary),
            b = read_all_bytes(f);

        return b;
    }

    method: pullSession (void; Event event)
    {
        if (_contacts.size() != 1)
        {
            print ("ERROR: can't request session from more than one connection.\n");
            return;
        }
        sendEach ("remote-pull-session", "", nil /*originator*/, true /*force*/);

        if (_showFeedback) displayFeedback("Requesting Session Data ... ", 1, syncGlyph);
    }

    method: pushSessionToAll (void; Event event)
    {
        let b = sessionAsByteArray();

        if (_showFeedback) displayFeedback("Sending Session Data ... ", 1, syncGlyph);

        deb ("************* pushSessionToAll %s" % _contacts);
        for_each (c; _contacts)
        {
            deb ("******* pushing session to '%s'" % c._contact);
            remoteSendDataEvent("remote-push-session",
                            c._session,
                            "session-data",
                            b,
                            string[] {c._contact});
        } 
        if (!_lockFrame)
        {
            string f = string(frame());
            sendEach("remote-sync-frame-changed", f);
        }
    }

    method: remotePullSession (void; Event event)
    {
        if (_showFeedback) displayFeedback("Sending Session Data ... ", 1, syncGlyph);

        let contact = findSyncContact(event.sender());

        remoteSendDataEvent("remote-push-session",
                        contact._session,
                        "session-data",
                        sessionAsByteArray(),
                        string[] {contact._contact});

        if (!_lockFrame)
        {
            string f = string(frame());
            sendEach("remote-sync-frame-changed", f);
        }
    }

    method: remotePushSession (void; Event event)
    {
        deb ("remotePushSession");
        let contact = findSyncContact(event.sender()),
            tmpDir = cacheDir(),
            target = tmpDir + "/" + "SessionFrom" + contact._contact + ".rv",
            f = ofstream(target, stream.Out | stream.Binary),
            d = event.dataContents();

        f.write(d);
        f.close();

        receiveLockPush();

        try
        {
            let cmode = cacheMode();
            setCacheMode(CacheOff);
            let smode = getStringProperty("#RVDisplayStereo.stereo.type").back();

            addSource(target, "explicit");

            setCacheMode(cmode);
            setStringProperty("#RVDisplayStereo.stereo.type", string[]{smode});
            setHardwareStereoMode(smode == "hardware" && stereoSupported());
        }
        catch (...) {;}
        deb ("remotePushSession: session read complete");

        receiveLockPop();

        State state = data();
        state.feedback = 0;
        redraw();
    }

    method: sendSources (void;)
    {
        let o = io.osstream();

        for_each (n; nodes())
        {
            if (nodeType(n) == "RVFileSource")
            {
                for_each (s; getStringProperty("%s.media.movie" % n))
                {
                    print(o, s);
                    print(o, ";");
                }

                print(o, "\n");
            }
        }

        sendEach("remote-sync-sources", string(o), nil /*originator*/, true /*force*/);
    }

    method: acceptPattern (void; regex pattern, Event event)
    {
        if (hasBlockPattern (pattern)) removeBlockPattern (pattern);
        else addBlockPattern (pattern);
    }

    method: checkAcceptPattern ((int;); regex pattern)
    {
        \: (int;)
        {
            if RemoteSync.hasBlockPattern (this, pattern) 
                then UncheckedMenuState
                else CheckedMenuState;
        };
    }

    method: toggleFeedback (void; Event event)
    {
        _showFeedback = !_showFeedback;
        writeSetting("Sync", "showFeedback", SettingsValue.Bool(_showFeedback));
    }

    method: feedbackState (int; )
    {
        return if (this._showFeedback) then CheckedMenuState else UncheckedMenuState;
    }

    method: toggleShowContactsPointers (void; Event event)
    {
        _showContactsPointers = !_showContactsPointers;
        writeSetting("Sync", "showContactsPointers", SettingsValue.Bool(_showContactsPointers));
    }
    method: showContactsPointersState (int; )
    {
        return if (this._showContactsPointers) then CheckedMenuState else UncheckedMenuState;
    }
    method: togglePointerTrails (void; Event event)
    {
        _showPointerTrails = !_showPointerTrails;
        writeSetting("Sync", "showPointerTrails", SettingsValue.Bool(_showPointerTrails));
    }

    method: pointerTrailsState (int; )
    {
        return if (this._showPointerTrails) then CheckedMenuState else UncheckedMenuState;
    }

    method: toggleShowContactErrors (void; Event event)
    {
        _showContactErrors = !_showContactErrors;
        writeSetting("Sync", "showContactErrors", SettingsValue.Bool(_showContactErrors));
    }

    method: showContactErrorsState (int; )
    {
        return if (this._showContactErrors) then CheckedMenuState else UncheckedMenuState;
    }

    method: toggleSyncWithContacts (void; Event event)
    {
        _syncWithContacts = !_syncWithContacts;
        writeSetting("Sync", "syncWithContactsContactsV2", SettingsValue.Bool(_syncWithContacts));
    }

    method: syncWithContactsState (int; )
    {
        return if (this._syncWithContacts) then CheckedMenuState else UncheckedMenuState;
    }

    method: sendPattern (void; regex pattern, Event event)
    {
        if (hasSendPattern (pattern)) removeSendPattern (pattern);
        else addSendPattern (pattern);
    }

    method: checkSendPattern ((int;); regex pattern)
    {
        \: (int;)
        {
            if RemoteSync.hasSendPattern (this, pattern) 
                then CheckedMenuState
                else UncheckedMenuState;
        };
    }

    //
    //  Pipeline Group filtering
    //

    method: addSendPipelineGroup (void; string group)
    {
        _sendPipelineGroups.push_back(group);
    }

    method: removeSendPipelineGroup (void; string group)
    {
        string[] newList = string[]();
        for_each (g; _sendPipelineGroups) if (g != group) newList.push_back(g);
        _sendPipelineGroups = newList;
    }

    method: hasSendPipelineGroup (bool; string group)
    {
        for_each (g ; _sendPipelineGroups) if (g == group) return true;

        return false;
    }

    method: sendPipelineGroup (void; string group, Event event)
    {
        if (hasSendPipelineGroup (group)) removeSendPipelineGroup (group);
        else addSendPipelineGroup (group);
    }

    method: checkSendPipelineGroup ((int;); string group)
    {
        \: (int;)
        {
            if RemoteSync.hasSendPipelineGroup (this, group) 
                then CheckedMenuState
                else UncheckedMenuState;
        };
    }

    method: addBlockPipelineGroup (void; string group)
    {
        _blockPipelineGroups.push_back(group);
    }

    method: removeBlockPipelineGroup (void; string group)
    {
        string[] newList = string[]();
        for_each (g; _blockPipelineGroups) if (g != group) newList.push_back(g);
        _blockPipelineGroups = newList;
    }

    method: hasBlockPipelineGroup (bool; string group)
    {
        for_each (g ; _blockPipelineGroups) if (g == group) return true;

        return false;
    }

    method: acceptPipelineGroup (void; string group, Event event)
    {
        if (hasBlockPipelineGroup (group)) removeBlockPipelineGroup (group);
        else addBlockPipelineGroup (group);
    }

    method: checkAcceptPipelineGroup ((int;); string group)
    {
        \: (int;)
        {
            if RemoteSync.hasBlockPipelineGroup (this, group) 
                then UncheckedMenuState
                else CheckedMenuState;
        };
    }

    method: checkOneContact (int; )
    {
        let s = if (this._contacts.size() == 1) then NeutralMenuState else DisabledMenuState;
        return s;
    }

    method: saveSettings (void; Event event)
    {

        \: flagsForPattern (string[]; [regex] patternList)
        {
            string[] flags;

            for_each (r; patternList)
            {
                if      (r eq MatchAudio) flags.push_back("audio");
                else if (r eq MatchFormat) flags.push_back("format");
                else if (r eq MatchDisplayStereo) flags.push_back("displayStereo");
                else if (r eq MatchViewTransform) flags.push_back("view");
                else if (r eq MatchStereo) flags.push_back("stereo");
                else if (r eq MatchColor) flags.push_back("color");
                else if (r eq MatchDisplayColor) flags.push_back("displayColor");
                else if (r eq MatchAnnotation) flags.push_back("annotation");
            }

            flags;
        }

        let sendFlags   = flagsForPattern(_sendPropList),
            blockFlags  = flagsForPattern(_blockPropList);

        _blockPropList = MatchTagProperty : _blockPropList;

        writeSetting("Sync", "send", SettingsValue.StringArray(sendFlags));
        writeSetting("Sync", "block", SettingsValue.StringArray(blockFlags));
        writeSetting("Sync", "sendMediaChanges", SettingsValue.Bool(_sendMediaChanges));
        writeSetting("Sync", "acceptMediaChanges", SettingsValue.Bool(_acceptMediaChanges));

        writeSetting("Sync", "sendPipelineGroups", SettingsValue.StringArray(_sendPipelineGroups));
        writeSetting("Sync", "blockPipelineGroups", SettingsValue.StringArray(_blockPipelineGroups));
    }

    method: beginAccumulate (void; Event event)
    {
        _accumulate++;
    } 

    method: endAccumulate (void; Event event)
    {
        if (_accumulate > 0) 
        {
            _accumulate--;
            if (_accumulate == 0) flushEvents(event);
        }
    }
    
    method: flushEvents (void; Event event)
    {
        //
        //  Send all delayed events
        //

        for_each (ev; _delayedEvents) sendEach(ev.eventName, ev.contents, ev.originator, true /*force*/);
        _delayedEvents.clear();
    }

    method: getReceiveLock (void; Event event)
    {
        event.reject();
        deb ("getReceiveLock event '%s'" % event.name());
        receiveLockPush();
    }

    method: releaseReceiveLock (void; Event event)
    {
        event.reject();
        deb ("releaseReceiveLock event '%s'" % event.name());
        receiveLockPop();
    }

    method: checkSendMediaChanges (int; )
    {
        if (_sendMediaChanges) return CheckedMenuState; 
        else                   return UncheckedMenuState;
    }

    method: checkAcceptMediaChanges (int; )
    {
        if (_acceptMediaChanges) return CheckedMenuState; 
        else                     return UncheckedMenuState;
    }

    method: toggleSendMediaChanges (void; Event event)
    {
        _sendMediaChanges = (! _sendMediaChanges);
    }

    method: toggleAcceptMediaChanges (void; Event event)
    {
        _acceptMediaChanges = (! _acceptMediaChanges);
    }

    method: connectToFriend (void; Event event)
    {
        deb ("connectToFriend '%s'" % event.contents()); 
        let (contentsArray, sender) = parseCompoundEventContents(event);
        let name = contentsArray[0],
            host = contentsArray[1],
            port = contentsArray[2];

        deb ("name: %s host: %s port %s\n" % (name,host,port)); 
        bool connected = false;
        for_each (c; _contacts)
        {
            if (c._contact == name) connected = true;
        }
        if (!connected)
        {
            deb ("connecting to name: %s host: %s port %s\n" % (name,host,port)); 
            remoteConnect(name,host,int(port));
            deb ("connected to name: %s host: %s port %s\n" % (name,host,port)); 
        }
    }

    method: connectRemoteFriends (void; Event event)
    {
        deb ("connectRemoteFriends '%s'\n" % event.contents()); 
        let (contentsArray, sender) = parseCompoundEventContents(event);
        let name = sender._contact,
            host = contentsArray[0],
            port = contentsArray[1];

        deb ("name: %s host: %s port %s\n" % (name,host,port)); 
        for_each (c; _contacts)
        {
            deb ("comparing c._contact '%s' sender '%s'" % (c._contact, name));
            if (c._contact != name)
            {
                deb ("session: %s\n" % sessionName());
                remoteSendEvent("connect-to-friend",
                                sessionName(),
                                "%s&%s&%s|%s" % (name,host,port,sessionName()),
                                string[] {c._contact});
                deb ("Told friends\n");
            }
        }
    }

    method: RemoteSync (RemoteSync; string name)
    {
        init (name,
                  nil,
                  [("pointer--move", move, "Track pointer"),
                   ("stylus-pen--move", move, "Track stylus pointer"),
                   ("pointer--enter", enter, "Track pointer enter"),
                   ("pointer--leave", leave, "Track pointer leave"),
                   ("frame-changed", frameChanged, "Update remote frame"),
                   ("play-start", playStart, "Update remote play"),
                   ("play-stop", playStop, "Update remote stop"),
                   ("play-inc", playInc, "Update remote play increment"),
                   ("new-in-point", receiveLockFilter(newInPoint, ), "Update remote in point"),
                   ("new-out-point", receiveLockFilter(newOutPoint, ), "Update remote out point"),
                   ("mark-frame", receiveLockFilter(newMarkFrame, ), "Update remote mark"),
                   ("unmark-frame", receiveLockFilter(removeMarkFrame, ), "Update remote mark"),
                   //("new-source", sourcesChanged, "Inform remote about media"),
                   //("source-modified", sourcesChanged, "Inform remote about media"),
                   ("source-modified", receiveLockFilter(mediaAddedTo, ), "Inform remote about media change"),
                   ("media-relocated", receiveLockFilter(sourceMediaRelocated, ), "Inform remote about media change"),
                   ("source-media-set", receiveLockFilter(sourceMediaSet, ), "Inform remote about media change"),
                   ("fps-changed", fpsChanged, "Update remote fps"),
                   ("play-all-frames-mode", playAllFramesMode, "Update remote play mode"),
                   ("realtime-play-mode", realtimePlayMode, "Update remote play mode"),
                   ("play-mode-changed", playModeChanged, "Update remote play mode (pingpong, once, loop)"),
                   ("read-lut-complete", receiveLockFilter(readLutComplete, ), "Update remote lut load"),
                   ("remote-sync-greeting", greetings, "Hookup to remote contact"),
                   ("remote-sync-version", syncVersion, "Note remote contact sync version"),
                   ("remote-sync-goodbye", goodbye, "Release from remote contact"),
                   ("remote-connection-stop", connectionStop, "Release from remote contact"),
                   ("remote-connection-start", syncWithConnectedRVs, "Sync with new connections"),
                   ("remote-sync-pointer", syncPointer, "Update remote pointer"),
                   ("remote-sync-enter", syncEnter, "Update remote pointer"),
                   ("remote-sync-leave", syncLeave, "Update remote pointer"),
                   ("remote-sync-frame-changed", receiveLockFilter(syncFrameChanged, ), "Start"),
                   ("remote-sync-play-start", receiveLockFilter(syncPlayStart, ), "Start playing"),
                   ("remote-sync-play-stop", receiveLockFilter(syncPlayStop, ), "Stop playing"),
                   ("remote-sync-play-inc", receiveLockFilter(syncPlayInc, ), "Change Play Inc"),
                   ("remote-sync-new-in-point", receiveLockFilter(syncNewInPoint, ), "Set In Point"),
                   ("remote-sync-new-out-point", receiveLockFilter(syncNewOutPoint, ), "Set Out Point"),
                   ("remote-sync-mark-frame", receiveLockFilter(syncNewMarkFrame, ), "Set Mark"),
                   ("remote-sync-unmark-frame", receiveLockFilter(syncRemoveMarkFrame, ), "Unset Mark"),
                   ("remote-sync-sources", receiveLockFilter(syncSources, ), "Update Loaded Sources"),
                   ("remote-sync-play-all-frames", syncPlayAllFrames, "Update Play Mode"),
                   ("remote-sync-realtime", syncRealtime, "Update Play Mode"),
                   ("remote-sync-play-mode", syncPlayMode, "Update Play Mode (pingpong, once, loop)"),
                   ("remote-sync-read-lut-complete", syncReadLutComplete, "Update Read LUT)"),
                   ("remote-sync-fps", receiveLockFilter(syncFps, ), "Update fps"),
                   ("remote-network-stop", syncStop, "Stop"),
                   ("remote-network-start", syncStart, "Start"),
                   ("remote-contact-error", syncError, "Contact error"),
                   ("remote-contact-error-show", syncErrorShow, "Show Contact error"),
                   ("graph-state-change", receiveLockFilter(stateChange, ), "Graph State Changed"),
                   ("graph-state-change-insert", receiveLockFilter(stateInsert, ), "Graph State Changed"),
                   ("graph-state-will-insert", receiveLockFilter(stateWillInsert, ), "Graph State Changed"),
                   ("graph-new-property", receiveLockFilter(handleNewProperty, ), "Graph Property Created"),
                   ("graph-property-deleted", receiveLockFilter(deletedProperty, ), "Graph Property Deleted"),
                   ("remote-sync-graph-state", receiveLockFilter(syncState, ), "Update Graph State"),
                   ("remote-sync-new-property", receiveLockFilter(syncState, ), "New Graph Property"),
                   ("remote-sync-delete-property", receiveLockFilter(syncState, ), "Delete Graph Property"),
                   ("remote-pull-session", receiveLockFilter(remotePullSession, ), "Remote requests session data"),
                   ("remote-push-session", receiveLockFilter(remotePushSession, ), "Remote provides session data "),
                   ("internal-sync-begin-accumulate", beginAccumulate, "Accumulate events (delay sending)"),
                   ("internal-sync-end-accumulate", endAccumulate, "End accumulate events"),
                   ("internal-sync-flush", flushEvents, "Force flush accumulated events"),
                   ("remote-sync-eval", receiveLockFilter(remoteSyncEval, ), "Remote Eval, Events Locked Out"),
                   ("new-node", receiveLockFilter(newNode, ), "New IP Node Created"),
                   ("before-graph-view-change", receiveLockFilter(viewNodeChangeInit, ), "View Node Changed"),
                   ("after-graph-view-change", receiveLockFilter(viewNodeChange, ), "View Node Changed"),
                   ("graph-node-inputs-changed", receiveLockFilter(nodeInputsChanged, ), "Node Inputs Changed"),
                   ("after-node-delete", receiveLockFilter(nodeDeleted, ), "Node Deleted"),
                   ("mode-toggled", receiveLockFilter(modeToggled, ), "GUI Mode Toggled"),
                   ("remote-mode-toggled", receiveLockFilter(syncModeToggled, ), "Remote GUI Mode Toggled"),
                   ("session-clear-everything", receiveLockFilter(sessionClearEverything, ), "Session Cleared"),
                   ("before-session-read", getReceiveLock, "Seassion Load Started"),
                   ("after-session-read", releaseReceiveLock, "Seassion Load Finished"),
                   ("connect-remote-friends", connectRemoteFriends, "Share new connection with contacts"),
                   ("connect-to-friend", connectToFriend, "Connect with any new contact"),
                   ],
                  Menu {
                    {"Sync", Menu {
                        {"Open Network Dialog...", ~showNetworkDialog, nil, nil},
                        {"Copy Session URL", copySessionUrl, nil, nil},
                        {"_", nil, nil, nil},
                        {"Pull Session", pullSession, nil, checkOneContact},
                        {"Push Session To All Contacts", pushSessionToAll, nil, nil},
                        {"_", nil, nil, nil},
                        {"Sync with Contacts' Contacts", toggleSyncWithContacts, nil, syncWithContactsState},
                        {"Sync With Connected RVs", ~toggleSync, nil, syncWithConnectedRVsState},
                        {"_", nil, nil, nil},
                        {"Settings", Menu {
                            {"Show Sync Messages", toggleFeedback, nil, feedbackState},
                            {"Show Contacts Pointers", toggleShowContactsPointers, nil, showContactsPointersState},
                            {"Show Pointer Trails", togglePointerTrails, nil, pointerTrailsState},
                            {"Show Contacts Network Errors", toggleShowContactErrors, nil, showContactErrorsState},
                            {"_", nil, nil, nil},
                            {"Send", nil, nil, \: (int;) { DisabledMenuState;} },
                            {"   Color", sendPattern (MatchColor,), nil, checkSendPattern (MatchColor)},
                            {"   Pan and Zoom", sendPattern (MatchViewTransform,), nil, checkSendPattern (MatchViewTransform)},
                            {"   Stereo Settings", sendPattern (MatchStereo,), nil, checkSendPattern (MatchStereo)},
                            {"   Annotation", sendPattern (MatchAnnotation,), nil, checkSendPattern (MatchAnnotation)},
                            {"   Media Changes", toggleSendMediaChanges, nil, checkSendMediaChanges},
                            {"   Display Stereo Settings", sendPattern (MatchDisplayStereo,), nil, checkSendPattern (MatchDisplayStereo)},
                            {"   Display Color Settings", sendPattern (MatchDisplayColor,), nil, checkSendPattern (MatchDisplayColor)},
                            {"   Matte Settings", sendPattern (MatchSessionMatte,), nil, checkSendPattern (MatchSessionMatte)},
                            {"   Image Format Settings", sendPattern (MatchFormat,), nil, checkSendPattern (MatchFormat)},
                            {"   Audio Settings", sendPattern (MatchAudio,), nil, checkSendPattern (MatchAudio)},
                            {"   Pipeline Group Changes", Menu {
                                {"Linearize", sendPipelineGroup ("Linearize",), nil, checkSendPipelineGroup ("Linearize")},
                                {"Color", sendPipelineGroup ("Color",), nil, checkSendPipelineGroup ("Color")},
                                {"Look", sendPipelineGroup ("Look",), nil, checkSendPipelineGroup ("Look")},
                                {"View", sendPipelineGroup ("View",), nil, checkSendPipelineGroup ("View")},
                                {"Display", sendPipelineGroup ("Display",), nil, checkSendPipelineGroup ("Display")},
                                {"General", sendPipelineGroup ("General",), nil, checkSendPipelineGroup ("General")}}},
                            {"_", nil, nil, nil},
                            {"Accept", nil, nil, \: (int;) { DisabledMenuState;} },
                            {"   Color", acceptPattern (MatchColor,), nil, checkAcceptPattern (MatchColor)},
                            {"   Pan and Zoom", acceptPattern (MatchViewTransform,), nil, checkAcceptPattern (MatchViewTransform)},
                            {"   Stereo", acceptPattern (MatchStereo,), nil, checkAcceptPattern (MatchStereo)},
                            {"   Annotation", acceptPattern (MatchAnnotation,), nil, checkAcceptPattern (MatchAnnotation)},
                            {"   Media Changes", toggleAcceptMediaChanges, nil, checkAcceptMediaChanges},
                            {"   Display Stereo Settings", acceptPattern (MatchDisplayStereo,), nil, checkAcceptPattern (MatchDisplayStereo)},
                            {"   Display Color Settings", acceptPattern (MatchDisplayColor,), nil, checkAcceptPattern (MatchDisplayColor)},
                            {"   Matte Settings", acceptPattern (MatchSessionMatte,), nil, checkAcceptPattern (MatchSessionMatte)},
                            {"   Image Format Settings", acceptPattern (MatchFormat,), nil, checkAcceptPattern (MatchFormat)},
                            {"   Audio Settings", acceptPattern (MatchAudio,), nil, checkAcceptPattern (MatchAudio)},
                            {"   Pipeline Group Changes", Menu {
                                {"Linearize", acceptPipelineGroup ("Linearize",), nil, checkAcceptPipelineGroup ("Linearize")},
                                {"Color", acceptPipelineGroup ("Color",), nil, checkAcceptPipelineGroup ("Color")},
                                {"Look", acceptPipelineGroup ("Look",), nil, checkAcceptPipelineGroup ("Look")},
                                {"View", acceptPipelineGroup ("View",), nil, checkAcceptPipelineGroup ("View")},
                                {"Display", acceptPipelineGroup ("Display",), nil, checkAcceptPipelineGroup ("Display")},
                                {"General", acceptPipelineGroup ("General",), nil, checkAcceptPipelineGroup ("General")}}},
                            {"_", nil, nil, nil},
                            {"Save as Default Settings", saveSettings, nil, nil}}},
                       {"_", nil, nil, nil},
                       {"Quit Sync", quitSync, nil, nil} }}}
                  ); 

        _networkActive           = true;
        _lockFrame               = false;
        _lockFrameOriginator     = nil;
        _receiveLockCount        = 0;
        _viewChangeLock          = false;
        _sendTime                = true;
        _blockTime               = false;
        _sendMediaChanges        = true;
        _acceptMediaChanges      = true;
        _showFeedback            = true;
        _showContactsPointers    = true;
        _showPointerTrails       = false;
        _showContactErrors       = true;
        _broadcastExternalEvents = true;
        _syncWithContacts        = false;
        _lockRange               = false;
        _lockRangeOriginator     = nil;
        _labelSize               = 12;
        _lockMark                = false;
        _lockMarkOriginator      = nil;
        _lock                    = false;
        _lockOriginator          = nil;
        _lockStateChange         = false;
        _hasOlderContactVersions = false;
        _delayedEvents           = DelayedEvent[]();
        _accumulate              = 0;

        _firstConnect = nil;
        if (commandLineFlag("syncPullFirst") neq nil) _firstConnect = "pull";
        if (commandLineFlag("syncPushFirst") neq nil) _firstConnect = "push";

        let SettingsValue.StringArray sendFlags =
            readSetting("Sync", "send",
                        SettingsValue.StringArray(string[]{"annotation", "color", "view", "stereo"}));

        let SettingsValue.StringArray blockFlags =
            readSetting("Sync", "block", SettingsValue.StringArray(string[]()));

        let SettingsValue.StringArray sendPipelineGroups = readSetting("Sync", "sendPipelineGroups",
                        SettingsValue.StringArray(string[]{"Color", "Look", "View"}));
        _sendPipelineGroups = sendPipelineGroups;

        let SettingsValue.StringArray blockPipelineGroups = readSetting("Sync", "blockPipelineGroups",
                        SettingsValue.StringArray(string[]{}));
        _blockPipelineGroups = blockPipelineGroups;

        let SettingsValue.Bool b1 = readSetting ("Sync", "sendMediaChanges", SettingsValue.Bool(_sendMediaChanges));
        _sendMediaChanges = b1;

        let SettingsValue.Bool b2 = readSetting ("Sync", "acceptMediaChanges", SettingsValue.Bool(_acceptMediaChanges));
        _acceptMediaChanges = b2;
        
        let SettingsValue.Bool b3 = readSetting ("Sync", "showFeedback", SettingsValue.Bool(_showFeedback));
        _showFeedback = b3;

        let SettingsValue.Bool b4 = readSetting ("Sync", "showPointerTrails", SettingsValue.Bool(_showPointerTrails));
        _showPointerTrails = b4;

        let SettingsValue.Bool b5 = readSetting ("Sync", "syncWithContactsContactsV2", SettingsValue.Bool(_syncWithContacts));
        _syncWithContacts = b5;

        let SettingsValue.Bool b6 = readSetting ("Sync", "showContactsPointers", SettingsValue.Bool(_showContactsPointers));
        _showContactsPointers = b6;

        let SettingsValue.Bool b7 = readSetting ("Sync", "showContactErrors", SettingsValue.Bool(_showContactErrors));
        _showContactErrors = b7;

        // The _syncWithContacts setting can be overriden from the command line
        let syncWithContactsContactsCmdLine = commandLineFlag("syncWithContactsContacts");
        if (syncWithContactsContactsCmdLine neq nil) 
        {
            _syncWithContacts = syncWithContactsContactsCmdLine == "true";
        }

        // The _broadcastExternalEvents setting can be overriden from the command line
        let syncBroadcastExternalEventsCmdLine = commandLineFlag("syncBroadcastExternalEvents");
        if (syncBroadcastExternalEventsCmdLine neq nil) 
        {
            _broadcastExternalEvents = syncBroadcastExternalEventsCmdLine == "true";
        }
        if (_syncWithContacts)
        {
            // Make sure to disable broadcasting of external events if _syncWithContacts is enabled
            // These two settings are mutually exclusive
            // When _syncWithContacts is enabled, a full mesh network topology is created, 
            // therefore broadcasting external events on top of that would create mayhem.
            _broadcastExternalEvents = false;
        }

        \: listFromArray ([regex]; string[] array)
        {
            [regex] list;

            for_each (flag; array)
            {
                case (flag)
                {
                    "color"         -> { list = MatchColor : list; }
                    "view"          -> { list = MatchViewTransform : list; }
                    "audio"         -> { list = MatchAudio : list; }
                    "format"        -> { list = MatchFormat : list; }
                    "stereo"        -> { list = MatchStereo : list; }
                    "displayStereo" -> { list = MatchDisplayStereo : list; }
                    "displayColor"  -> { list = MatchDisplayColor : list; }
                    "annotation"    -> { list = MatchAnnotation : list; }
                }
            }

            list;
        }

        _sendPropList = listFromArray(sendFlags);
        _blockPropList = listFromArray(blockFlags);

        _blockPropList = MatchTagProperty : _blockPropList;

        addSendPattern (MatchFolderG);
        addSendPattern (MatchStackG);
        addSendPattern (MatchSwitchG);
        addSendPattern (MatchSourceG);
        addSendPattern (MatchSequenceG);
        addSendPattern (MatchLayoutG);
        addSendPattern (MatchStack);
        addSendPattern (MatchSequence);
        addSendPattern (MatchSwitch);
        addSendPattern (MatchTransform2D);
        addSendPattern (MatchRetime);
        addSendPattern (MatchFileSource);

        _contacts = SyncContact[]();
        _virtualContacts = SyncContact[]();

        bind("source-group-complete", \: (void; Event event)
        {
            event.reject();
            State s = data();
            if (s.scrubAudio && audioCacheMode() != CacheGreedy)
            {
                setAudioCacheMode(CacheGreedy);
            }
        });
    }

    method: copySessionUrl (void; Event ev)
    {
        if (sync_mode.theMode() eq nil) 
        { 
            return;
        }

        sync_mode.theMode().copySessionUrl(ev);
    }

    method: syncWithConnectedRVsState (int; )
    {
        if (sync_mode.theMode() eq nil) 
        { 
            return DisabledMenuState;
        }

        return sync_mode.theMode().syncWithConnectedRVsState();
    }

    method: syncWithContact (void; string contact)
    {
        remoteNetwork(true);

        let c = string[] {contact};

        remoteSendEvent("remote-eval", "*", "require sync; sync.remoteStartUp();", c);

        remoteSendEvent("remote-sync-greeting",
                        "*",
                        sessionName(),
                        c);
    }

    method: unsyncWithContact (void; string contact)
    {
        remoteNetwork(true);

        let c = string[] {contact};

        remoteSendEvent("remote-sync-goodbye",
                        "*",
                        sessionName(),
                        c);

        let newContacts = SyncContact[] ();
        for_each (oldContact; _contacts)
        {
            if (oldContact._contact != contact) newContacts.push_back (oldContact);
        }
        _contacts = newContacts;
    }

    method: _drawCross (void; Point p, float t)
    {
        let x0 = Vec2(2, 0),
            x1 = Vec2(10, 0),
            y0 = Vec2(0, 2),
            y1 = Vec2(0, 10);

       float f = t/2.0;

        glLineWidth(3.0);

        glColor(0.5-t,0.5-t,0.5-t,1);

        glBegin(GL_LINES);
        glVertex(p + x0); glVertex(p + x1);
        glVertex(p - x0); glVertex(p - x1);
        glVertex(p + y0); glVertex(p + y1);
        glVertex(p - y0); glVertex(p - y1);
        glEnd();

        glLineWidth(1.0);
        glColor(0.5+t,0.5+t,0.5+t,1);

        glBegin(GL_LINES);
        glVertex(p + x0); glVertex(p + x1);
        glVertex(p - x0); glVertex(p - x1);
        glVertex(p + y0); glVertex(p + y1);
        glVertex(p - y0); glVertex(p - y1);
        glEnd();
    }
    
    method: render (void; Event event)
    {
        if (!_networkActive || !_showContactsPointers) return;

        let imageInfos = sourcesRendered();
        let renderedSourceNames = string[]{};
        for_each (ii; imageInfos)
        {
            let parts = ii.name.split("/");
            renderedSourceNames.push_back(parts.front());
        }

        // Render pointers for both real and virtual contacts
        let contactsCount = _contacts.size();
        let allContactsCount = _contacts.size() + _virtualContacts.size();
        for (let c_index=0; c_index<allContactsCount; c_index++)
        {
            let c = if (c_index<contactsCount) then _contacts[c_index] else _virtualContacts[c_index-contactsCount];

            if (c neq nil &&
                (!c.PointerIsActive() ||
                 c._source eq nil ||
                 c._source == "")) 
            {
                continue;
            }

            // Look through the rendered sources, if we don't match the source
            // from the contact pointer, then just ignore this draw.
            let cSourceRendered = false;
            let sourceCount = renderedSourceNames.size();
            for (let i=0; i<sourceCount; i++)
            {
                let renderedSourceName = renderedSourceNames[i];
                if (renderedSourceName == c._source)
                {
                    cSourceRendered = true;
                    break;
                }
            }
            if (cSourceRendered == false)
            {
                continue;
            }

            try
            {
                let domain = event.domain(),
                    cname  = c._label,
                    tw     = c._twidth,
                    th     = c._theight,
                    flip   = event.domainVerticalFlip();

                setupProjection(domain.x, domain.y, flip);

                glPushAttrib(GL_ENABLE_BIT);
                //
                //  Bracket all the following in a try block so that if it throws, we
                //  still pop the attribute statck.
                //
                try
                {

                if (_showPointerTrails)
                {
                    int trailSize = c._pointerTrail.size();
                    for (int i = trailSize-1; i > 1; --i)
                    {
                        let elemp  = c._pointerTrail.elem(i),
                            trailp = Point(elemp.x, if flip then -1.0 * elemp.y else elemp.y);
                        _drawCross (
                                imageToEventSpace (c._source, trailp, true),
                                0.5*float(trailSize-i)/float(trailSize));
                    }
                    //
                    //  Only adjust these data structures if this render is the "main"
                    //  render (as opposed to "render-output-device").
                    //
                    if (event.name() == "render" && trailSize > 1)
                    {
                        if (theTime() - _pointerTrailTime > 0.04)
                        {
                            c._pointerTrail.removeOldest(); 
                            _pointerTrailTime = theTime();
                        }
                        redraw();
                    }
                }

                let ypos = if flip then -1.0 * c._y else c._y,
                    p    = imageToEventSpace (c._source, Point(c._x, ypos), true);

                _drawCross (p, 1.0);

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                drawTextWithCartouche(p.x - tw/2.0,
                                      (p - Point(0,10)).y - th * 2.0,
                                      cname,
                                      _labelSize,
                                      Color(1,1,1,1),
                                      Color(0,0,0,.15));

                }
                catch (...) {;}
                glPopAttrib();
            }
            catch (...)
            {
                ; // just ignore problems with the remote end
                  // right now
            }
        }
    }

    method: syncWithConnectedRVs (void; Event event)
    {
        deb ("************* %s" % remoteApplications());
        deb ("************* %s" % remoteConnections());
        for_index (i; remoteConnections()) 
        {
            let c = remoteConnections()[i];
            if ("rv" == remoteApplications()[i] && findSyncContact(c) eq nil )
            {
                deb ("***************** syncing with '%s'" % c);
                syncWithContact (c);
            }
        }
    }

    method: activate (void;)
    {
        use SettingsValue;
        State state = data();
        ModeManagerMode mm = state.modeManager;
        state.registerQuitMessage("sync", "You have an active sync connection");

        try
        {
            let StringArray modeNames = readSetting("Sync",
                                                    "extraModes",
                                                    StringArray(string[]{}));

            for_each (mode; modeNames)
            {
                let entry = mm.findModeEntry(mode);

                if (entry neq nil)
                {
                    mm.loadEntry(entry);
                    mm.activateEntry(entry, true);
                }
            }
        }
        catch (...)
        {
            ; // ignore
        }
        syncWithConnectedRVs (nil);
    }

    method: deactivate (void;)
    {
        State state = data();
        state.unregisterQuitMessage("sync");

        for_index (i; remoteConnections()) 
        {
            if ("rv" == remoteApplications()[i])
            {
                unsyncWithContact(remoteConnections()[i]);
            }
        }
    }

    method: disconnect (void;)
    {
        for_index (i; remoteConnections()) 
        {
            if ("rv" == remoteApplications()[i])
            {
                remoteDisconnect(remoteConnections()[i]);
            }
        }
    }
}

\: acceptingMediaChanges (bool;)
{
    State state = data();
    if (state.sync neq nil) 
    {
        RemoteSync s = state.sync;
        return s._acceptMediaChanges;
    }
    return false;
}

\: startUp (void;)
{
    State state = data();
    remoteNetwork(true);

    if (state.sync eq nil)
    {
        let s = RemoteSync("sync"); 
        state.sync = s;
        if (!s._active) s.toggle();

        try
        {
            commands.logMetricsWithProperties("Using Sync Review", "{\"type\": \"Started a Session\"}");
        }
        catch (...)
        {
            try
            {
                commands.logMetrics("Using Sync Review");
            }
            catch (...) {;}
        }
    }
    else
    {
        if (!state.sync._active) state.sync.toggle();
    }
}

\: remoteStartUp (void;) { startUp(); }

\: checkSyncVersion (void; int remoteVersion)
{
    if (remoteVersion < SYNC_VERSION_MIN_COMPATIBLE)
    {
        alertPanel(true,
                ErrorAlert,
                "ERROR: Sync version mismatch. Shutting down sync.",
                "Remote version = %s, we require version %s or above." 
                    % (remoteVersion, SYNC_VERSION_MIN_COMPATIBLE),
                "Ok", nil, nil);

        State state = data();
        if (state.sync neq nil && state.sync._active) 
        {
            RemoteSync s = state.sync;
            s.disconnect();
        }
    }
}

}
