//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
module: export_utils
{
    require system;
    require io;
    use commands;
    use rvtypes;
    use external_qprocess;
    use extra_commands;

    global int saveSessionCount=1;

    \: tempSessionName (string; string extension="rv")
    {
        require qt;
        let path = qt.QDir.tempPath();
        "%s/temp_%d_%d.%s" %  (path,
                           system.getpid(),
                           saveSessionCount++,
                           extension);
    }

    \: removeSession (VoidFunc; string name)
    {
        \: (void;)
        {
            if (commandLineFlag("debug_export") neq nil)
            {
                print("DEBUG: Leaving '%s' for inspection\n" % name);
            }
            else
            {
                require qt;
                try
                {
                    if (!qt.QFile(name).remove())
                    {
                        print("ERROR: Unable to remove '%s'\n" % name);
                    }
                }
                catch (...)
                {
                    print("ERROR: Unable to remove '%s'\n" % name);
                }
            }
        };
    }

    \: getColorNode (string; string groupType)
    {
        for_each (node; nodesOfType("RVDisplayColor"))
        {
            let pipeGroup = nodeGroup(node),
                dispGroup = nodeGroup(pipeGroup);

            if (nodeType(dispGroup) == groupType)
            {
                return node;
            }
        }

        return nil;
    }

    \: setExportDisplayConvert (void; string conversion)
    {
        let outColorNode = getColorNode("RVOutputGroup");
        //
        //  Maybe the user configured the output node themselves
        //
        if (conversion == "pass" || outColorNode eq nil) return;

        if (conversion == "default")
        {
            string colorProf = "";
            string stereoProf = "";
            try
            {
                let dnodes  = nodesOfType("RVDisplayGroup"),
                    dstereo = nodesInGroupOfType(dnodes.front(), "RVDisplayStereo"),
                    dpipes  = nodesInGroupOfType(dnodes.front(), "RVDisplayPipelineGroup"),
                    opipes  = nodesInGroupOfType("defaultOutputGroup", "RVDisplayPipelineGroup"),
                    ostereo = nodesInGroupOfType("defaultOutputGroup", "RVDisplayStereo");

                //
                //  Copy display properties and nodes to output
                //
                colorProf = tempSessionName("profile");
                writeProfile(colorProf, dpipes.front());
                readProfile(colorProf, opipes.front(), false);
                stereoProf = tempSessionName("profile");
                writeProfile(stereoProf, dstereo.front());
                readProfile(stereoProf, ostereo.front(), false);

                removeSession(colorProf)();
                removeSession(stereoProf)();
            }
            catch (...)
            {
                print("ERROR: Failed to copy display settings to output\n");
                if (qt.QFile(colorProf).exists()) removeSession(colorProf)();
                if (qt.QFile(stereoProf).exists()) removeSession(stereoProf)();
            }
        }
        else
        {
            int s = 0;
            int r = 0;
            float g = 1.0;

            if (conversion == "") ;
            else if (conversion == "sRGB")      s = 1;
            else if (conversion == "Rec709")    r = 1;
            else if (conversion == "Gamma 2.0") g = 2.0;
            else if (conversion == "Gamma 2.2") g = 2.2;
            else if (conversion == "Gamma 2.4") g = 2.4;

            setIntProperty  (outColorNode + ".color.sRGB",   int[]   {s});
            setIntProperty  (outColorNode + ".color.Rec709", int[]   {r});
            setFloatProperty(outColorNode + ".color.gamma",  float[] {g});
        }
    }

    \: makeTempSession (string; string conversion="default")
    {
        setExportDisplayConvert(conversion);

        let name = tempSessionName();
        saveSession(name, true);
        name;
    }

    \: removeTempSession(void;)
    {
        // Guess that the last session was the one we want to delete
        // NOTE: Adding this only for backwards compatibility
        saveSessionCount--;
        let name = tempSessionName();
        removeSession(name)();
    }

    \: rvio (ExternalProcess; string name, string[] inargs, (void;) cleanup = nil)
    {
        let cmd = system.getenv("RV_APP_RVIO");
        string[] args = {"-v", "-err-to-out" };
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

    \: rvio_blocking (void; string name, string[] args, (void;) cleanup = nil)
    {
        ExternalQProcess proc = rvio(name, args, cleanup);

        proc._proc.waitForFinished(-1);
    }

    \: markedRegionBoundaries ([(int,int,int)];)
    {
        let a = markedFrames(),
            b = markedFrames();

        a.pop_back();
        b.erase(0, 1);
        [(int,int,int)] ranges;

        for (int i=a.size()-1; i >= 0; i--)
        {
            ranges = (a[i], b[i]-1, i) : ranges;
        }
        
        ranges;
    }

    \: exportImageSequenceOverRange (ExternalProcess; 
                                     int start,
                                     int end,
                                     string prefix="",
                                     string imagetype="tif",
                                     bool blocking=false,
                                     string conversion="default")
    {
        let name = "%s.#" % prefix,
            temp = makeTempSession(conversion);

        string[] args = 
        { 
            temp,
            "-o", "%s.%s" % (name, imagetype),
            "-t", "%d-%d" % (start, end)
        };
        
        if (blocking)
        {
            rvio_blocking("Export Image Sequence", args, removeSession(temp));
            return nil;
        }
        else
        {
            return rvio("Export Image Sequence", args, removeSession(temp));
        }
    }

    \: exportMovieOverRange(ExternalProcess; 
                            int start,
                            int end,
                            string name="out.mov",
                            bool blocking=false,
                            string conversion="default")
    {
        let temp = makeTempSession(conversion);

        string[] args =
        {
            temp,
            "-o", name,
            "-t",  "%d-%d" % (start, end) 
        };

        if (blocking)
        {
            rvio_blocking("Export Movie", args, removeSession(temp));
            return nil;
        }
        else
        {
            return rvio("Export Movie", args, removeSession(temp));
        }
    }

    \: exportMarkedFrames (ExternalProcess; string filepat, string conversion="default")
    {
        use io;
        osstream timestr;
        let frames = markedFrames();

        for_index (i; frames)
        {
            let f = frames[i];
            if (i > 0) print(timestr, ",");
            print(timestr, "%d" % f);
        }

        let temp = makeTempSession(conversion);

        string[] args =
        {
            temp,
            "-o", filepat,
            "-t", string(timestr)
        };

        return rvio("Export Annotated Frames", args, removeSession(temp));
    }

    \: exportMarkedRegionsAsMovies (void; string prefix="", string movietype="mov", string conversion="default")
    {
        if (prefix == "") prefix = sessionFileName().substr(0,-3);

        for_each (inout; markedRegionBoundaries())
        {
            let (start, end, i) = inout;

            exportMovieOverRange(start, end,
                                 "%s_cut%d.%s" % (prefix, i, movietype),
                                 true, conversion);
        }
    }

    \: exportMarkedRegionsAsImages (void; 
                                    string prefix="",
                                    string imagetype="tif",
                                    string conversion="default")
    {
        if (prefix == "") prefix = sessionFileName().substr(0,-3);
        
        for_each (inout; markedRegionBoundaries())
        {
            let (start, end, i) = inout,
                name            = "%s_cut%d" % (prefix, i);

            exportImageSequenceOverRange(start, end,
                                         name, imagetype,
                                         true, conversion);
        }
    }
}
