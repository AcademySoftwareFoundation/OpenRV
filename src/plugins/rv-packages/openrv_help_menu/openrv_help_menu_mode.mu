//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
module: openrv_help_menu_mode {
use rvtypes;
use runtime;
use commands;
use app_utils;
use extra_commands;
require system;
require io;
require qt;

documentation: """
OpenRVHelpMenuMinorMode adds a Help menu and some modal event tables that
implement things like describe key, etc. Help minor mode should always
appear list in the menu bar.
"""
class: OpenRVHelpMenuMinorMode : MinorMode
{
    \: showManual (void; Event ev, string env)
    {
        require system;
        State state = data();

        try
        {
            let m = system.getenv(env);

            if (runtime.build_os() == "WINDOWS")
            {
                let wpath = regex.replace("/", m, "\\\\");
                system.defaultWindowsOpen(wpath);
            }
            else openUrl("file://" + m);
        }
        catch (...)
        {
            int choice = alertPanel(true,
                                    WarningAlert,
                                    "Manual location unknown",
                                    "Looking for missing env var %s" % env,
                                    "OK", nil, nil);
        }
    }

    \: describeHelp (void; Event ev)
    {
        displayFeedback("Describe Options (Hit 'k' for Keys, 'e' for Events)", 10e6);
        redraw();
        pushEventTable("describe-mode");
    }

    \: describeKeyBinding (void; Event ev)
    {
        redraw();
        displayFeedback("Describe Options -> (Press Any Key For Description) ...", 10e6);
        pushEventTable("describe-mode");
        pushEventTable("describe-key-mode");
    }

    \: docbrowser (void; Event ev)
    {
        State s = data();
        mode_manager.ModeManagerMode mmm = s.modeManager;
        mmm.activateMode("doc_browser", true);
    }

    \: dumpBindings (void; Event ev)
    {
        print("<br><br><hr><center>");
        print("<H1>Current Bindings</H1>");
        print("Modes:");
        for_each (m; activeModes()) print(" %s" % m);

        print("<table border=0><tr><th>Event</th><th>Binding</th></tr>");

        for_each (b; bindings()) 
        {
            let (eventName, description) = b;
            if (description != "") print("<tr><td> %s </td><td> %s </td></tr>" % b);
        }

        print("</table></center><hr>\n");

        showConsole();
    }

    \: commandHelp (void;)
    {
        use autodoc;
        print("<br><br><hr><h1>RV Commands</h1><blockquote>");
        
        for_each (c; document_symbol("commands").split("\n"))
        {
            let l = regex.smatch("commands.([a-zA-Z]+) (.*)", c);
            
            if (l neq nil)
            {
                print("<font face=monospace><b>%s</b> %s</font><br>" % (l[1], l[2]));
            }
        }
        
        print("</blockquote><hr>\n");
        showConsole();
    }

    \:showEnv (void;)
    {
        print ("************ Environment Variables ******************\n");

        let envList = qt.QProcessEnvironment.systemEnvironment().toStringList();
        for_each (var; envList) print ("  %s\n" % var);

        print ("**************** Build Info ************************\n");
        print ("  OS:       %s\n" % runtime.build_os());
        print ("  Arch:     %s\n" % runtime.build_architecture());
        print ("  Compiler: %s\n" % runtime.build_compiler());
        print ("*****************************************************\n");

        showConsole();
    }

    \: opUrl (void; Event ev, string url)
    {
        openUrl (url);
    }


    \: inactiveState (int;) { DisabledMenuState; }

    method: OpenRVHelpMenuMinorMode (OpenRVHelpMenuMinorMode;)
    {
        Menu menuList = Menu {
                {"Online Resources",    nil, nil, inactiveState},
                {"   RV User's Manual",   opUrl(,"https://aswf-openrv.readthedocs.io/en/latest/rv-manuals/rv-user-manual/rv-user-manual-chapter-one.html"), nil},
                {"   RV Reference Manual", opUrl(,"https://aswf-openrv.readthedocs.io/en/latest/rv-manuals/rv-reference-manual/rv-reference-manual-chapter-one.html"), nil},
                {"   Mu User's Manual",    opUrl(,"https://github.com/AcademySoftwareFoundation/OpenRV/blob/main/docs/rv-manuals/rv-mu-programming.md"), nil},
                {"_",                   nil},
                {"   GTO File Format (.rv files)", opUrl(,"https://github.com/AcademySoftwareFoundation/OpenRV/blob/main/docs/rv-manuals/rv-gto.md"), nil},
                {"_",                   nil},
                {"   Academy Software Foundation Open RV", opUrl(,"https://github.com/AcademySoftwareFoundation/OpenRV"), nil},
                {"_",                   nil},
                {"Other Resource",    nil, nil, inactiveState},
                {"   Mu Command API Browser...",  docbrowser, nil},
                {"_",                   nil},
                {"Utilities",           nil, nil, inactiveState},
                {"   Describe...",         describeHelp, "?"},
                {"   Describe Key Binding...", describeKeyBinding, nil},
                {"   Show Current Bindings", dumpBindings, nil},
                {"   Show Environment",    ~showEnv}
        };

        Menu menu = Menu {{"Help", menuList}};

        \: bindDescribe (void; string name, EventFunc F)
        {
            bind("default", "describe-mode", name, F);
        }

        \: bindDescribeRegex (void; string name, EventFunc F)
        {
            bindRegex("default", "describe-mode", name, F);
        }

        \: bindDescribeKey (void; string name, EventFunc F)
        {
            bind("default", "describe-key-mode", name, F);
        }

        \: bindDescribeKeyRegex (void; string name, EventFunc F)
        {
            bindRegex("default", "describe-key-mode", name, F);
        }

        bindDescribeRegex("key-down--shift.*", noop);

        bindDescribe("key-down--k",\: (void; Event ev)
        {
            State state = data();
            displayFeedback("Describe Options -> (Press Any Key For Description) ...", 10e6);
            redraw();
            pushEventTable("describe-key-mode");
        });

        bindDescribe("key-down--e",\: (void; Event ev)
        {
            State state = data();
            displayFeedback("Describe Options -> (Escape Exits, Otherwise Show Event) ...", 10e6);
            redraw();
            pushEventTable("describe-any-event-mode");
        });

        bindDescribeKeyRegex("^key-down.*shift--shift*", noop);
        bindDescribeKeyRegex("^key-down.*alt--alt*", noop);
        bindDescribeKeyRegex("^key-down.*control--control*", noop);
        bindDescribeKeyRegex("^key-down.*meta--meta*", noop);
        bindDescribeKeyRegex("^key-down.*meta--super-left*", noop);
        bindDescribeKeyRegex("^key-down.*meta--super-right*", noop);
        bindDescribeKeyRegex("^key-down.*", \: (void; Event ev)
        {
            State state = data();
            repeat (2) popEventTable();
            let name = ev.name(),
                niceName = name.substr(10, name.size());

            try
            {
                displayFeedback("Key \"%s\" --> %s"
                                % (niceName, bindingDocumentation(ev.name())),
                            5.0);
            }
            catch (...)
            {
                displayFeedback("\"%s\" is Not Bound to Anything" % niceName, 5.0);
            }
            redraw();
        });

        bind("default", "describe-any-event-mode", "key-down--escape", \: (void; Event ev)
        {
            State state = data();
            displayFeedback("Finished Event Feedback Mode");
            repeat (2) popEventTable();
            redraw();
        });

        bindRegex("default", "describe-any-event-mode", "^(key|mod).*", \: (void; Event ev)
        {
            State state = data();
            displayFeedback("Event  \"%s\"    (value = %d) (modifiers = %s)"
                            % (ev.name(), ev.key(), ev.modifiers()), 5.0);
            redraw();
        });

        bindRegex("default", "describe-any-event-mode", "^(pointer|generic|puck|stylus|airbrush|mouse4D|rotating).*",
        \: (void; Event ev)
        {
            State state = data();
            displayFeedback("Event  \"%s\"    (modifiers = %s)"
                            % (ev.name(), ev.modifiers()), 5.0);
            redraw();
        });
        
        this.init("help",
                  [("key-down--?", describeHelp, "Show Help Options")],
                  nil,
                  menu,
                  "z",
                  9999);  // always last
    } 
}

\: createMode (Mode;)
{
    return OpenRVHelpMenuMinorMode();
}

}
