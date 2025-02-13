//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

module: maya_tools {

use rvtypes;
use commands;

require io;
require extra_commands;
require runtime;
require session_manager;

require qt;
require io;

function: deb(void; string s) { if (false) print (s + "\n"); }

class: MayaTools : MinorMode
{ 
    regex    _regexIFF;
    regex    _regexOK;
    bool     _mayaSource;
    string[] _origSourceGroups;

    class: Prefs
    {
        bool viewLatestPlayblast;

	method: writePrefs (void; )
	{
	    writeSetting ("MayaUtils", "viewLatestPlayblast", SettingsValue.Bool(viewLatestPlayblast));
	}

	method: readPrefs (void; )
	{
	    let SettingsValue.Bool b1 = readSetting ("MayaUtils", "viewLatestPlayblast", SettingsValue.Bool(true));

	    viewLatestPlayblast = b1;
	}

	method: Prefs (Prefs; )
	{
	    readPrefs();
	}
    }

    Prefs _prefs;

    method: toggleViewLatestPlayblast (void; Event e)
    {
        _prefs.viewLatestPlayblast = !_prefs.viewLatestPlayblast;

        _prefs.writePrefs();
    }

    method: showingViewLatestPlayblast (int; )
    {
        return if (_prefs.viewLatestPlayblast == true) then CheckedMenuState else UncheckedMenuState; 
    }

    method: addMayaSource (void; string media)
    {
        let repairedMedia = repairMayaPath(media),
	    source = addSourceVerbose (string[] { media });

	updateMayaSource (nodeGroup(source));
    }

    method: repairMayaPath (string; string inpath)
    {
	deb ("repairMayaPath");
	string outpath = nil;

	if (_regexOK.match(inpath))
        //
        //  Maya2014 now produces valid paths (at least sometimes; need to
        //  test).
        //
	{
	    outpath = inpath;
	}
        else
	if (_regexIFF.match(inpath) && !io.path.exists(inpath))
	{
	    outpath = regex.replace("#", inpath, "#.");
	}
	else
	if (!io.path.exists(inpath)) 
	{
	    if (io.path.exists(inpath + ".mov"))
	    {
		outpath = inpath + ".mov";
	    }
	    if (io.path.exists(inpath + ".avi"))
	    {
		outpath = inpath + ".avi";
	    }
	}
        else 
        //
        //  Path exists, so might as well pass it through ...
        //
        {
            outpath = inpath;
        }

	if (outpath neq nil) print ("INFO: repairMayaPaths '%s' -> '%s' \n" % (inpath, outpath));

	return outpath;
    }

    method: autoRepairMayaPath (void; Event event)
    {
	deb ("autoRepairMayaPath");
	deb ("autoRepairMayaPath '%s'" % event.contents());
        event.reject();

        let previous = event.returnContents(),
            parts    = string.split(event.contents(), ";;"),
            hasTag   = parts.size() > 1,
            inpath   = if (previous != "") then previous else parts[0];

	string outpath  = nil;

	deb ("    license %s" % system.getenv("MAYA_LICENSE", "none"));
	if (nil neq system.getenv("MAYA_LICENSE", nil)) 
	{
	    //  if (!hasTag || (hasTag && parts[1] != "explicit" && parts[1] != "session"))
	    if (!hasTag || (hasTag && parts[1] == "rvpush"))
	    {
		outpath = repairMayaPath (inpath);
		deb ("    outpath '%s'" % outpath);
		if (outpath neq nil)
		{
		    event.setReturnContent (outpath);
		    _mayaSource = true;
		}
	    }
	}
	deb ("autoRepairMayaPath complete");
    }

    method: recordSourceGroups (void; Event event)
    {
	deb ("recordSourceGroups");
        event.reject();

	_origSourceGroups = nodesOfType("RVSourceGroup");
	deb ("recordSourceGroups complete");
    }

    method: compareSelected (void; Event event, string style)
    {
        if (session_manager.theMode() eq nil) return;

        let nodeType = if (style == "tile") then "RVLayoutGroup" else "RVStackGroup",
            selected = session_manager.theMode().selectedNodes();

        if (selected.size() < 2) return;

        let node = newNode (nodeType, nodeType.substr(2,0) + "000000");

        setNodeInputs (node, selected);
        setViewNode (node);

        extra_commands.setUIName (node, "Compare %s views" % selected.size());

        if (style == "tile")
        {
            extra_commands.set ("#RVLayoutGroup.layout.mode", "packed");
        }
        else
        {
            rvui.toggleWipe();
        }
    }

    method: combineMenus (rvtypes.Menu; rvtypes.Menu a, rvtypes.Menu b)
    {
        if (a eq nil) return b;
        if (b eq nil) return a;

        rvtypes.Menu n;
        for_each (i; a) n.push_back(i);
        for_each (i; b) n.push_back(i);

        return n;
    }


    method: showHelpFile (void; Event event)
    {
        let helpFile = io.path.join(supportPath("maya_tools", "maya_tools"), "maya_tools_help.html");

        if (runtime.build_os() == "WINDOWS")
        {
            let wpath = regex.replace("/", helpFile, "\\\\");
            system.defaultWindowsOpen(wpath);
        }
        else commands.openUrl("file://" + helpFile);
    }

    method: disabledFunc (int;) { DisabledMenuState; }

    method: doNothing (void; Event event) { ; }

    method: doNothingState (int; ) { NeutralMenuState; }

    method: oneViewSelected (int; )
    {
        if (session_manager.theMode() eq nil) return DisabledMenuState;

        let nodes = session_manager.theMode().selectedNodes();

        return if (nodes.size() == 1) then NeutralMenuState else DisabledMenuState;
    }

    method: viewsSelected (int; )
    {
        if (session_manager.theMode() eq nil) return DisabledMenuState;

        let nodes = session_manager.theMode().selectedNodes();

        return if (nodes.size() >= 2) then NeutralMenuState else DisabledMenuState;
    }

    method: currentTarget (string; )
    {
	for_each (s; nodesOfType("RVSourceGroup"))
	{
	    if (propertyExists(s + "_source.maya.target")) return s;
	}
	return nil;
    }

    method: markAsTarget (void; string sourceName)
    {
        if (nodeType(sourceName) != "RVFileSource") return;

	for_each (s; nodesOfType("RVSourceGroup"))
	{
	    if (propertyExists(s + "_source.maya.target")) 
	    {
		deleteProperty (s + "_source.maya.target"); 
		let newName = regex.replace (" \(playblast_target\)", extra_commands.uiName(s), "");
		extra_commands.setUIName (s, newName);
	    }
	    if (s + "_source" == sourceName)
	    {
		newProperty (s + "_source.maya.target", IntType, 1);
		setIntProperty (s + "_source.maya.target", int[] {1}, true);
		extra_commands.setUIName (s, extra_commands.uiName(s) + " (playblast_target)");
	    }
	}
    }

    method: markCurrentAsTarget (void; Event event)
    {
        if (session_manager.theMode() eq nil) return;

        let nodes = session_manager.theMode().selectedNodes();

        if (nodes.size() != 1) return;

	markAsTarget (nodes.front() + "_source");
    }

    method: updateTargetInputs (void; string newTarget)
    {
	let target = currentTarget();
	if (target eq nil) return;

	for_each (v; viewNodes())
	{
            if (v == "defaultSequence" || v == "defaultLayout" || v == "defaultStack") continue;

	    let inputs = nodeConnections(v, false)._0,
	        needsUpdate = false;

	    for_index (i; inputs)
	    {
	        if (inputs[i] == target)
		{
		    inputs[i] = newTarget;
		    needsUpdate = true;
		}
	    }
	    if (needsUpdate) setNodeInputs(v, inputs);
	}
    }

    method: updateMayaSource (void; string ns)
    {
	let t = qt.QDateTime.currentDateTime().toString("h:mm",  qt.QCalendar());

	extra_commands.setUIName (ns, extra_commands.uiName(ns) + " " + t);

	newProperty (ns + "_source.maya.playblast", IntType, 1);
	setIntProperty (ns + "_source.maya.playblast", int[] {1}, true);

	if (currentTarget() neq nil)
	{
	    updateTargetInputs (ns);
	    markAsTarget (ns + "_source");
	}

	if (_prefs.viewLatestPlayblast) setViewNode (ns);
    }

    method: afterPlayblastLoad (void; Event event)
    {
	event.reject();

        if (_mayaSource)
	{
	    let newSourceGroups = nodesOfType("RVSourceGroup");
	    for_each (ns; newSourceGroups)
	    {
	        let foundIt = false;
		for_each (s; _origSourceGroups) if (s == ns) { foundIt = true; break; }
		if (!foundIt) 
		{
		    updateMayaSource (ns);
		}
	    }
	}
	_mayaSource = false;
    }

    method: installMel (void; Event event)
    {
        let envHomePath    = system.getenv ("HOME", nil),
            homeDir        = if (envHomePath neq nil) then qt.QDir(envHomePath) else qt.QDir.home(),
            scriptSubPath  = "Library/Preferences/Autodesk/maya/scripts",
            scriptPath     = homeDir.canonicalPath() + "/" + scriptSubPath;

        if (! qt.QFileInfo(scriptPath).exists() && ! homeDir.mkpath(scriptSubPath))
        {
            let msg = "Can't create directory '%s'" % scriptPath;
            print ("can't create '%s'\n" % scriptSubPath);
            commands.alertPanel (true, commands.ErrorAlert, msg, "", "OK", nil, nil);
            return;
        }

        scriptPath += "/playblastWithRV.mel";

        if (qt.QFileInfo(scriptPath).exists())
        {
	    let ans = commands.alertPanel (true, commands.InfoAlert, "\nYou already have a 'playblastWithRV.mel' file in your scripts directory, perhaps from a previous install.  Do you want to overwrite this file ?\n",
		    "", "Overwrite", "Leave Unchanged", nil);

	    if (ans != 0) return;
        }

	let newScriptFile = qt.QFile (scriptPath),
	    newScriptStream = qt.QTextStream (newScriptFile);

	if (! newScriptFile.open (qt.QFile.WriteOnly | qt.QFile.Text | qt.QFile.Truncate))
	{
	    let msg = "cannot open script '%s' for writing." % scriptPath;
	    commands.alertPanel (true, commands.ErrorAlert, msg, "", "OK", nil, nil);
	    return;
	}

	let contents = """
	global proc playblastWithRV (string $file, string $fps)
	{
	    $cmd = "unset QT_MAC_NO_NATIVE_MENUBAR; %s -tag playblast" + " merge [ " + $file + " -fps " + $fps + " ]";
	    system ($cmd);
	}
	""" % regex.replace ("RV[64]*$", system.getenv ("RV_APP_RV"), "rvpush");

	io.print (newScriptStream, contents);

	newScriptFile.close();

        commands.alertPanel (true, commands.InfoAlert, "\nInstallation complete !", "", "OK", nil, nil);
    }

    method: buildMenu (Menu; )
    {
        Menu m1 = Menu {
            {"Wipe Selected Playblasts",   compareSelected(, "wipe"), nil, viewsSelected},
            {"Tile Selected Playblasts",   compareSelected(, "tile"), nil, viewsSelected},
            {"Mark Selected as Target",   markCurrentAsTarget, nil, oneViewSelected},
            {"_", nil},
            {"Preferences", nil, nil, disabledFunc},
            {"    View Latest Playblast", toggleViewLatestPlayblast, nil, showingViewLatestPlayblast},
        };

        Menu m2 = nil;
	if (runtime.build_os() == "DARWIN")
        {
            m2 = rvtypes.Menu {
                {"_", nil},
                {"Install Maya Support File", installMel, nil, doNothingState},
            };
        }

        Menu m3 = rvtypes.Menu {
            {"_", nil},
            {"Help ...", showHelpFile, nil, doNothingState},
        };

        rvtypes.Menu m = rvtypes.Menu {
            {"Maya", combineMenus (combineMenus (m1, m2), m3)}
        };

        return m;
    }

    method: MayaTools (MayaTools;)
    {
	_regexIFF = "^.*\.#(iff|IFF)$";
	_regexOK = "^.*\.#\.(iff|IFF|jpg|JPG)$";
	_mayaSource = false;
	_prefs = Prefs();

        this.init ("maya-tools",
		[("incoming-source-path", autoRepairMayaPath, "Repair broken paths from playblasts"),
		 ("before-progressive-loading", recordSourceGroups, ""),
		 ("after-progressive-loading", afterPlayblastLoad, "")],
		nil,
		buildMenu());
    }
}

\: createMode (Mode;)
{
    return MayaTools();
}

}
