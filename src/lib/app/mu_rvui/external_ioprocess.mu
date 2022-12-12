//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
module: external_ioprocess {
use io;
use rvtypes;

class: ExternalIOProcess : ExternalProcess
{
    io.process  _proc;

    method: ExternalIOProcess (ExternalProcess; 
                             string name,
                             string path, 
                             [string] args,
                             int64 timeout_usecs,
                             ExternalProcess.Type t,
                             VoidFunc cleanup_func,
                             regex progressRE = ".*\(([0-9]+(\.[0-9]*)?)% done\).*",
                             regex messageRE  = "INFO:[ \t]+([^(]+)(\([0-9]+\.?[0-9]*% +done\))?")
    {
        print("INFO: ExternalIOProcess: %s %s %d\n" % (path, string(args), timeout_usecs));

        _proc       = io.process(path, args, timeout_usecs);

        case (t)
        {
            ReadOnly  -> { _proc.close_in();}
            ReadWrite -> {;}
        }
    }

    method: isFinished (bool;) { _proc eq nil; }

    method: cancel (void;)
    {
        if (_proc neq nil)
        {
            _proc.kill();
            if (_cleanup neq nil) _cleanup();
            _proc = nil;
        }
    }

    method: finish (void;)
    {
        if (_proc neq nil)
        {
            _proc.close();
            if (_cleanup neq nil) _cleanup();
            _proc = nil;
        }
    }

    method: processIO (void;)
    {
        if (_proc neq nil)
        {
            try
            {
                if (_proc.out().eof())
                {
                    finish();
                }
                else
                {
                    let line = io.read_line(_proc.out(), '\n');
                    //print("PROCESS: %s\n" % line);

                    if (_messageRE.match(line))
                    {
                        let m = _messageRE.smatch(line);
                        _lastMessage = m[1];
                    }
                
                    if (_progressRE.match(line))
                    {
                        let m = _progressRE.smatch(line);
                        if (m[1] != "") _progress = float(m[1]);
                    }
                }
            }
            catch (exception exc)
            {
                //  Probably timed out
                //  need to check for other error states here
                _proc.out().clear();
            }
        }
    }
}

}
