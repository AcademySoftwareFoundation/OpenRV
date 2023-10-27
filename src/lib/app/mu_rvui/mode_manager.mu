//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
module: mode_manager
{
use rvtypes;
use commands;
use gl;
use glyph;
use app_utils;
use io;
use system;
use app_utils;
use python;
require qt;

documentation: """
This is a minor mode that wraps a corresponding python minor
mode. This makes it possible to control a python mode from Mu
""";

class: PyMinorMode : MinorMode
{
    PyObject _pymode;
    PyObject _activeFunc;
    PyObject _activateFunc;
    PyObject _deactivateFunc;
    PyObject _renderFunc;
    PyObject _layoutFunc;

    method: PyMinorMode (PyMinorMode; PyObject obj)
    {
	//
	//  We're keeping a reference to this mode, so INCREF
	//

        _pymode = obj;
	Py_INCREF(_pymode);

        let pyrvui   = PyImport_Import("rv.rvui"),
            onEmptyF = PyObject_GetAttr(pyrvui, "modeDrawOnEmpty"),
            onPresF  = PyObject_GetAttr(pyrvui, "modeDrawOnPresentation"),
            nameF    = PyObject_GetAttr(pyrvui, "modeName");

        _drawOnEmpty        = to_bool(PyObject_CallObject(onEmptyF, _pymode));
        _drawOnPresentation = to_bool(PyObject_CallObject(onPresF, _pymode));
        _modeName           = to_string(PyObject_CallObject(nameF, _pymode));

        _activeFunc         = PyObject_GetAttr(pyrvui, "modeActive");
        _activateFunc       = PyObject_GetAttr(pyrvui, "modeActivate");
        _deactivateFunc     = PyObject_GetAttr(pyrvui, "modeDeactivate");

	//
	//  These are also references to the mode, so INCREF since we are
        //  holding these refs.
	//

        _renderFunc         = PyObject_GetAttr(_pymode, "render");
	Py_INCREF(_pymode);
        _layoutFunc         = PyObject_GetAttr(_pymode, "layout");
	Py_INCREF(_pymode);

	//
	//  And one more for fun!  OK, sorry, I just don't know why this
	//  is necessary, but if we don't do this, python modes eventually
	//  get garbage collected.
	//
	Py_INCREF(_pymode);

        _active             = to_bool(PyObject_CallObject(_activeFunc, _pymode));
    }

    method: isActive (bool;)
    {
        _active = to_bool(PyObject_CallObject(_activeFunc, _pymode));
	return _active;
    }

    method: activate (void;)
    {
        PyObject_CallObject(_activateFunc, _pymode);
        //PyObject_CallObject(_setModeStatusFunc, (_pymode, true));
    }

    method: deactivate (void;)
    {
        PyObject_CallObject(_deactivateFunc, _pymode);
    }

    method: layout (void; Event event)
    {
        PyObject_CallObject(_layoutFunc, event);
    }

    method: render (void; Event event)
    {
        // NOTE: _renderFunc is partially evalled by python so no need to
        // give it _pymode as the "self" argument. Just pass in the event.
        PyObject_CallObject(_renderFunc, event);
    }
}


documentation: """
ModeManagerMode finds a file called "rvload" in the MU_MODULE_PATH and
if it exists parses it to create menu items and possibly immediately
load modes. Menu items and hot keys are dynamically created to toggle
unloaded modes as well.

With 3.8.3 we introduced a new rvload format that includes the
rv version required by the package.  For backwards compatibility
we store this version 2 data in a file called "rvload2".

3.12.X adds the ability to load python modules containing modes and to
manage them from the mode_manager along with the existing Mu modes.
""";

class: ModeManagerMode : MinorMode
{
    class: Package
    {
        string  file;
        string  name;
        string  base;
        int     major;
        int     minor;
        Package existing;
    }

    class: ModeEntry
    {
        string   name;
        Package  pkg;
        string   menu;
        string   accel;
        string   event;
        bool     loaded;
        bool     active;
        bool     optload;
        string[] requiresModes;
        string   baseDir;
        Mode     mode;
    }

    [ModeEntry]     _modes;
    Package[]       _packages;
    string[]        _doNotLoadPackages;
    string[]        _optionalPackages;
    (int;)[]        _stateFuncRefs;
    (void; Event)[] _toggleFuncRefs;
    bool            _verbose;
    string[]        _preLoads;
    string[]        _otherLoads;
    string[]        _rejectLoads;

    method: showInfo (void; string msg)
    {
        if (_verbose) print("INFO: %s\n" % msg);
    }

    method: showWarning (void; string msg)
    {
        if (_verbose) print("WARNING: %s\n" % msg);
    }

    method: isPreload (bool; string name)
    {
        for_each (m; _preLoads) if (m == name) return true;
        return false;
    }

    method: isRejectLoad (bool; string name)
    {
        for_each (m; _rejectLoads) if (m == name) return true;
        return false;
    }

    function: toLower (string mixed)
    {
        let ret = "",
            diff = 'a' - 'A';

        for (int i = 0; i < mixed.size(); ++i)
        {
            let c = mixed[i];
            ret += if (c >= 'A' && c <= 'Z') then (c + diff) else c;
        }

        return ret;
    }

    //
    //  Note that we are dealing with full path names here, and esp on windows,
    //  we may get arbitrary combinations of upper and lower case.  Convert everything
    //  to lower before we compare.
    //

    method: userUnloaded (bool; string name)
    {
        for_each (p; _doNotLoadPackages) if (toLower(name) == toLower(p)) return true;
        return false;
    }

    method: userOptLoaded (bool; string name)
    {
        for_each (p; _optionalPackages) if (toLower(name) == toLower(p)) return true;
        return false;
    }

    method: modeEntryStateFunc ((int;); ModeEntry entry)
    {
        \: (int; )
        {
            if entry.mode eq nil
                 then UncheckedMenuState
                 else if entry.mode._active
                        then CheckedMenuState
                        else UncheckedMenuState;
        };
    }

    method: findPackage (Package; string name)
    {
        for_each (p; _packages) if (p.file == name) return p;
        return nil;
    }

    method: findPackageByBase (Package; string name)
    {
        for_each (p; _packages) if (p.base == name) return p;
        return nil;
    }

    method: findModeEntry (ModeEntry; string name)
    {
        for_each (entry; _modes)
        {
            if (entry.name == name) return entry;
        }

        return nil;
    }

    method: loadPythonEntry (PyMinorMode; ModeEntry entry)
    {
        use path;

        if (_verbose)
        {
        print("INFO: Loading python entry %s\n" % entry.name);
        }

        let pymodname = without_extension(entry.name),
            pymodule = PyImport_Import(pymodname);

        if (is_nil(pymodule))
        {
            print("ERROR: python module %s could not be imported\n" % pymodname);
            return nil;
        }

        let modname = PyModule_GetName(pymodule),
            attr = PyObject_GetAttr(pymodule, "createMode");

	if (_verbose)
	{
	    print ("INFO: Loading mode from python module '%s'\n" % to_string(PyObject_GetAttr(pymodule, "__file__")));
	}

        if (is_nil(attr) || !PyFunction_Check(attr))
        {
            print("ERROR: python module %s has no createMode() function" % modname);
            return nil;
        }

        let arg = PyTuple_New(0);
        Py_INCREF(arg);
        let pymode = PyObject_CallObject(attr, arg);
        return if is_nil(pymode) then nil else PyMinorMode(pymode);
    }

    method: loadEntry (void; ModeEntry entry)
    {
        if (!entry.loaded)
        {
            State state = data();

            for_each (m; entry.requiresModes)
            {
                let mentry = findModeEntry(m);

                if (mentry eq nil)
                {
                    throw exception("no entry for mode %s required by %s\n" % (m, entry.name));
                }
                else
                {
                    loadEntry(mentry);
                }
            }
            let loadStartTime = theTime();
            PyMinorMode pymode = nil;

            if (!runtime.load_module(entry.name))
            {
                try
                {
                    pymode = loadPythonEntry(entry);
                }
                catch (exception exc)
                {
                    print("ERROR: while loading python module: %s\n" % exc);
                }

                if (pymode eq nil) throw exception("failed in runtime.load_module");
            }

            if (pymode eq nil)
            {
		if (_verbose)
		{
		    let foundIt = false;
		    for_each (t; runtime.module_locations())
		    {
			if (t._0 == entry.name)
			{
			    foundIt = true;
			    print ("INFO: Loading Mu mode from '%s'\n" % t._1);
			}
		    }
		    if (! foundIt) print ("WARNING: modeManager can't find module '%s'" % entry.name);
		}
                let sname = runtime.intern_name("%s.createMode" % entry.name);
                (Mode;) modeFunc = runtime.lookup_function(sname);
                entry.mode = modeFunc();
            }
            else
            {
                entry.mode = pymode;
            }

            entry.loaded = true;
            state.minorModes.push_back(entry.mode);

            let loadEndTime = theTime();
            showInfo("Loaded \"%s\" from %s, installed in %sPackages (%0.3f seconds)" % (entry.name, entry.pkg.name, entry.baseDir, (loadEndTime-loadStartTime)));
        }
    }

    method: activateEntry (void; ModeEntry entry, bool activate)
    {
        try
        {
            if (!entry.loaded) loadEntry(entry);
        }
        catch (exception exc)
        {
            showWarning("unable to load \"%s\" : %s" % (name, exc));
        }

        if (entry.mode neq nil && entry.mode._active != activate)
        {
            entry.mode.toggle();
        }
    }

    method: deactivateAll (void; Event event)
    {
        event.reject();

        for_each (entry; _modes)
        {
            if (entry.loaded && (entry.mode neq nil))
            {
                if (entry.mode._active) activateEntry(entry, false);
            }
        }
    }

    method: toggleEntry (void; ModeEntry entry)
    {
        try
        {
            if (!entry.loaded) loadEntry(entry);
            activateEntry(entry, !entry.mode._active);
        }
        catch (exception exc)
        {
            showWarning("unable to load \"%s\" : %s" % (name, exc));
        }
    }

    method: activateMode (Mode; string name, bool activate)
    {
        let entry = findModeEntry(name);

        if (entry neq nil)
        {
            activateEntry(entry, activate);
            return entry.mode;
        }
        else
        {
            return nil;
        }
    }

    method: toggleModeByName (Mode; string name)
    {
        let entry = findModeEntry(name);

        if (entry neq nil)
        {
            toggleEntry(entry);
            return entry.mode;
        }
        else
        {
            return nil;
        }
    }

    \: toggleModeEntry (void; Event event, ModeEntry entry, ModeManagerMode mm)
    {
        mm.toggleEntry(entry);
    }

    method: parseMenuEntry (MenuItem; ModeEntry entry)
    {
        let itemPath = reverse(entry.menu.split("/"));
        MenuItem itemTree = nil;

        for_index (i; itemPath)
        {
            if (i == 0)
            {
                (int;) st = modeEntryStateFunc(entry);
                (void; Event) tg = toggleModeEntry(,entry,this);

                //_stateFuncRefs.push_back(st);
                //_toggleFuncRefs.push_back(tg);

                itemTree = MenuItem(itemPath[i], tg, entry.accel, st);
            }
            else
            {
                itemTree = MenuItem(itemPath[i], Menu(itemTree));
            }
        }

        itemTree;
    }

    method: loadOrSkip (string;
                        string name,
                        Package pkg,
                        string base,
                        string reqVersion,
                        bool optload)
    {
        let fullPath  = path.join(path.join(base, "Packages"), pkg.file),
            rvVersion = getenv("TWK_APP_VERSION"),
            reqVparts = reqVersion.split("."),
            rvVparts  = rvVersion.split("."),
            preload = isPreload(name);

        if (pkg.existing neq nil)
        {
            return "package %s already loaded" % pkg.existing.name;
        }

        for_index (i; reqVparts)
        {
            let pver = int(reqVparts[i]),
                rvver = int(rvVparts[i]);

            if (pver > rvver) return "requires RV %s" % reqVersion;
            if (pver < rvver) break;
        }

        if (userUnloaded(fullPath) && !preload) return "unloaded by user pref";
        if (optload && !userOptLoaded(fullPath) && !preload) return "optional package not loaded";
        if (isRejectLoad(pkg.base) || isRejectLoad(pkg.name)) return "rejected from command line";

        //
        //  Already loaded?
        //

        for_each (entry; _modes)
        {
            if (entry.name == name) return "already loaded from %s" % entry.pkg.name;
        }

        return nil;
    }

    method: addPackage (Package; string name)
    {
        use path;
        let ext     = extension(name),
            base    = without_extension(name).split("-").front(),
            existing = findPackageByBase(base);

        int major = 1;
        int minor = 0;

        if (ext == "rvpkg")
        {
            try
            {
                let vparts = name.split("-")[1].split(".");
                major = int(vparts[0]);
                minor = int(vparts[1]);
            }
            catch (...)
            {
                print("ERROR: bad rvpkg package name \"%s\" expecting name-X.X.rvpkg\n" % name);
                return nil;
            }
        }

        _packages.push_back(Package(name, without_extension(name), base, major, minor, existing));
        return _packages.back();
    }

    method: findOrAddPackage (Package; string name)
    {
        let pkg = findPackage(name);
        if pkg eq nil then addPackage(name) else pkg;
    }

    method: addModeEntry (ModeEntry;
                          string name,
                          Package pkg,
                          string menu,
                          string accel,
                          string event,
                          bool loaded,
                          bool active,
                          bool optload,
                          string[] requiredModes,
                          string baseDir)
    {
        let entry = ModeEntry(name, pkg, menu, accel, event,
                              loaded, active, optload,
                              requiredModes, baseDir, nil);

        _modes = entry : _modes;
        entry;
    }

    method: loadModesFromFile (void; string filename)
    {
        State state = data();

        let infile = ifstream(filename),
            cnFile = qt.QFileInfo(filename).canonicalFilePath(),
            sdir   = path.dirname(path.dirname(cnFile)),
            lines  = read_all(infile).split("\n\r"),
            fileV  = 0;

        for_index (i; lines)
        {
            let line  = lines[i],
                parts = line.split(",");

            if (i == 0)
            {
                fileV = int(line);

                if (fileV < 0 || fileV > 4)
                {
                    print("ERROR: Can't parse version %s mode file\n" % line);
                    print("ERROR: No modes loaded\n");
                    infile.close();
                    return;
                }

                continue;
            }

            \: nilOrThing (string; string s) { if s == "nil" then nil else s; }

            if (parts.size() >= 7 && parts[0] != "#")
            {
                let name    = parts[0],
                    pkg     = findOrAddPackage(parts[1]),
                    menu    = nilOrThing(parts[2]),
                    accel   = nilOrThing(parts[3]),
                    event   = nilOrThing(parts[4]),
                    preload = isPreload(name),
                    loaded  = bool(parts[5]) || preload,
                    active  = bool(parts[6]) || preload,
                    reqV    = (if fileV == 1 then "3.6.0" else parts[7]),
                    optload = (if fileV >= 3 && parts.size() >= 9 then bool(parts[8]) else false),
                    openrvversion = (if fileV >= 4 && parts.size() >= 10 then parts[9] else "1.0.0");

                parts.erase(0, 7);
                if (fileV > 1) parts.erase(0, 1);
                if (fileV > 2) parts.erase(0, 1);
                if (fileV > 3) parts.erase(0, 1);

                let reqVersion = (if getApplicationType()=="OpenRV" then openrvversion else reqV);
                let skipReason = loadOrSkip(name, pkg, sdir, reqVersion, optload);

                if (0 == optionsNoPackages() && skipReason eq nil)
                {
                    let entry = addModeEntry(name, pkg, menu, accel, event,
                                             false, false, optload, parts, sdir);

                    if (loaded)
                    {
                        try
                        {
                            loadEntry(entry);
                            if (active) entry.mode.toggle();
                        }
                        catch (exception exc)
                        {
                            showWarning("unable to load \"%s\" : %s" % (name, exc));
                        }
                    }
                }
                else
                {
                    showInfo("Skipped loading of \"%s\" from \"%s\" (%s)" %
                             (name, pkg.name, skipReason));
                }
            }
        }

        infile.close();
    }

    method: load (void; Event event)
    {
        let mudirs    = getenv("MU_MODULE_PATH"),
            parts     = string.split(mudirs, path.concat_separator()),
            processed = string[]();

        for_each (dir; parts)
        {
            let skip = false,
                qd = qt.QDir(dir),
                cp = qd.canonicalPath();

            for_each (d; processed) if (d == cp) { skip = true; break; }
            if (skip) continue; // we already scanned this dir
            processed.push_back(cp);

            for_each (cf; string[] {"rvload", "rvload2"})
            {
                let file = "%s/%s" % (cp,cf);
                try
                {
                    if (path.exists(file)) loadModesFromFile(file);
                }
                catch (exception exc)
                {
                    print("ERROR: unable to read \"%s\" : %s\n" % (file, exc));
                }
            }
        }

        Menu top;

        for_each (m; _modes)
        {
            if (m.menu neq nil) top.push_back(parseMenuEntry(m));

            if (m.event neq nil)
            {
                bind(m.event, \: (void; Event ev)
                {
                    toggleModeEntry(ev, m, this);
                });
            }
        }

        defineModeMenu(name(), top);

        //
        //  We should be able to dump these references at this
        //  point, since defineModeMenu should have called
        //  retainExternal on them, but we still get a crash that
        //  seems to indicate the GC collected them.  Keep the refs
        //  forever for now.
        //
        //  all working now seemingly.  leave the code in for now.
        //
        _stateFuncRefs.clear();
        _toggleFuncRefs.clear();
        event.reject();

        for_each (m; _otherLoads)
        {
            showInfo("Forcing load of %s" % m);
            let entry = addModeEntry(m, findOrAddPackage("nopackage"),
                                     nil, nil, nil, false, false, false, nil, nil);

            try
            {
                loadEntry(entry);
                entry.mode.toggle();
            }
            catch (exception exc)
            {
                print("ERROR: Failed to load %s : %s\n" % (m, exc));
            }
        }

        if (_verbose)
        {
            for_each (p; _packages) showInfo("Using package %s" % p.name);
        }
    }

    method: viewChange (void; Event event, bool on)
    {
        event.reject();

        let vnode = viewNode(),
            vtype = nodeType(vnode),
            name  = if vtype.substr(0,2) == "RV" then vtype.substr(2,0) + "_edit_mode" else "",
            entry = findModeEntry(name);

        if (entry neq nil)
        {
            activateEntry(entry, on);
            if (on) sendInternalEvent("view-edit-mode-activated");
        }
    }

    method: toggleModeEvent (void; Event event)
    {
        event.reject();
        toggleModeByName(event.contents());
    }

    method: ModeManagerMode (ModeManagerMode;)
    {
        _packages    = Package[]();
        _verbose     = commandLineFlag("ModeManagerVerbose", "false") == "true";
        _preLoads    = commandLineFlag("ModeManagerPreload", "").split(",");
        _otherLoads  = commandLineFlag("ModeManagerLoad", "").split(",");
        _rejectLoads = commandLineFlag("ModeManagerReject", "").split(",");

        init("ModeManager",
             nil,
             [("state-initialized", load, "Load installed modes"),
              ("mode-manager-toggle-mode", toggleModeEvent, "Internal event to toggle a mode"),
              ("before-session-deletion", deactivateAll, "Deactivate all modes before deletion"),
              ("before-graph-view-change", viewChange(,false), "Deactivate mode when view changes"),
              ("after-graph-view-change", viewChange(,true), "Activate mode when view changes")],
             nil,
             "zzzzzz");

        try
        {
            _doNotLoadPackages = packageListFromSetting("doNotLoadPackages");
            _optionalPackages  = packageListFromSetting("optionalPackages");
        }
        catch (...)
        {
            ;
        }

        _stateFuncRefs = (int;)[]();
        _toggleFuncRefs = (void; Event)[]();
    }
}

}
