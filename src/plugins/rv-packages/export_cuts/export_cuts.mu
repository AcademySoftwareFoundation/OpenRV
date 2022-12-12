//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
use rvtypes;
use extra_commands;
use commands;
use export_utils;
use rvui;
use io;
use external_qprocess;

module: export_cuts {

class: ExportCutsMode : MinorMode 
{
    class: QueueItem
    {
        int start;
        int end;
        string name;
    }

    QueueItem[] _queue;
    bool _done;

    //
    //  For 3.8: reimplement from export_utils to get around this nasty bug
    //
    method: rvio (ExternalProcess; string name, string[] inargs, (void;) cleanup = nil)
    {
        let cmd = system.getenv("RV_APP_RVIO");
        string[] args;  
        args.push_back("-v");
        args.push_back("-err-to-out");
        [string] argList;

        let lic = system.getenv("RV_APP_USE_LICENSE_FILE", nil);

        if (lic neq nil) 
        {
            args.push_back("-lic");
            args.push_back(lic);
        }

        for_each (a; inargs) args.push_back(a);
        for_index (i; args) argList = args[args.size()-i-1] : argList;

        rvioSetup();

        ExternalQProcess(name,
                         cmd, argList, 1,
                         ExternalProcess.Type.ReadOnly,
                         cleanup);
    }

    method: cleanup (void;)
    {
        State state = data();
        state.externalProcess = nil;
        removeTempSession();

        if (_queue.empty()) _done = true;
        else launchItem();
    }

    method: launchItem (void;)
    {
        State state = data();
        let item = _queue.pop_back();
        
        string[] args;
        args.push_back(makeTempSession());
        args.push_back("-o");
        args.push_back(item.name);
        args.push_back("-t");
        args.push_back("%d-%d" % (item.start, item.end));

        state.externalProcess = rvio("%d Left" % (_queue.size() + 1),
                                     args,
                                     cleanup);
    }

    method: exportFiles (void; Event event)
    {
        try
        {
            markedRegionBoundaries();
        }
        catch (...)
        {
            alertPanel(true, InfoAlert, "No Marks", "There are no marks", "OK", nil, nil);
            return;
        }

        State state = data();

        let filename = saveFileDialog(true),
            ext      = path.extension(filename),
            base     = path.without_extension(filename);

        osstream allfiles;
        [QueueItem] qlist;

        _queue.clear();

        for_each (inout; markedRegionBoundaries())
        {
            let (start, end, i) = inout,
                item = QueueItem(start, if (end == frameEnd()-1) then end+1 else end, "%s_cut%03d.%s" % (base, i, ext));

            qlist = item : qlist;
            //print(allfiles, "%s\n" % item.name);
        }

        for_each (i; qlist) _queue.push_back(i);

        int choice = alertPanel(true,
                                InfoAlert,
                                "%d %s files/sequences will be exported" % (_queue.size(), ext),
                                "%d %s files/sequences will be exported" % (_queue.size(), ext),
                                //string(allfiles), // 3.10 this should be here instead of above
                                "Export %d Files" % _queue.size(),
                                "Cancel",
                                nil);

        if (choice == 2)
        {
            displayFeedback("Export Cancelled");
            _queue.clear();
            _done = true;
            return;
        }

        _done = false;

        if (state.processInfo eq nil || !state.processInfo._active) toggleProcessInfo();
        launchItem();
    }

    method: ExportCutsMode (ExportCutsMode;)
    {
        _queue = QueueItem[]();
        _done = false;

        init("export-cuts-mode",
             nil,
             nil,
             Menu {
               {"File", Menu {
                  {"Export", Menu {
                     {"_", nil, nil, nil},
                     {"Marked Regions as Movie/Audio/Sequences...", exportFiles, nil, videoSourcesExistState}
                  }
               }}
           }});
    }

    //alertPanel(true,
                           //InfoAlert,
                           //"You can go back to work now -- its done", nil,
                           //"Great, Thanks...", nil, nil);
} 

\: createMode (Mode;)
{
    return ExportCutsMode();
}
    
} // end module
