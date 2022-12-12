//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

module: rvnuke_process {
use rvtypes;
use commands;
use encoding;
use qt;
require io;
require runtime;

documentation: """
ExternalNukeProcess is derived from ExternalProcess declared in
rvtypes. This implements an actual interface with a process using Qt's
QProcess class. See also external_ioprocess.mu. """

class: ExternalNukeProcess : ExternalProcess
{
    QProcess _proc;
    string _errors;
    string _output;
    int _writeCount;
    int _totalWrites;

    regex _frameTimeRE;

    (void; string) _progressCallback;

    io.ofstream _debOut;

    method: deb(void; string s) 
    { 
        if (_debOut neq nil) 
        {
            io.print (_debOut, "nuke_process: " + s + "\n");
            io.flush (_debOut);
        }
    }

    method: readyRead (void;)
    {   
        deb ("readyRead");
        while (_proc.canReadLine())
        {
            let line = utf8_to_string(_proc.readLine(0).constData());
            if (regex("Error:").match(line)) _errors += line;
            if (_debOut neq nil) _output += line;

            if (_frameTimeRE.match(line))
            {
                let m = _frameTimeRE.smatch(line);
                _lastMessage = "Frame %s, %s seconds" % (m[1], m[2]);
            }
            
            if (_progressRE.match(line))
            {
                let m = _progressRE.smatch(line);

                ++_writeCount;
                _progress = 100.0 * float(_writeCount) / float(_totalWrites);
                deb ("    ************************* progress %s" % _progress);
                _progressCallback (m[1]);
            }
        }
    }

    method: finish (void; int exitcode = -1)
    {
        deb ("FINISHED");
        if (_proc neq nil)
        {

            if (exitcode != 0) 
            {
                if (_errors == "") _errors = " No errors in output";
                alertPanel(
                        true, // associated panel (sheet on OSX)
                        ErrorAlert,
                        "ERROR", "Errors:\n%s" % _errors,
                        "OK", nil, nil);
            }
            deb("ERRORs:\n%s" % _errors);
            deb("ALL OUTPUT:\n%s" % _output);
            _proc.close();
            _exitcode = exitcode;
            if (_cleanup neq nil) _cleanup();
            _proc = nil;
        }
    }

    method: isFinished (bool;) { _proc eq nil; }

    method: cancel (void;)
    {
        deb ("CANCELED");
        if (_proc neq nil)
        {
            _proc.close();
            if (_cleanup neq nil) _cleanup();
            _proc = nil;
        }
    }

    method: processIO (void;) {;} // deprecated

    method: ExternalNukeProcess (ExternalProcess; 
                             io.ofstream debOut,
                             string name,
                             string path,
                             string[] args,
                             int64 timeout_usecs,
                             ExternalProcess.Type t,
                             VoidFunc cleanup_func,
                             int totalWrites,
                             (void; string) progressCallback,
                             //regex progressRE = ".*\(([0-9]+(\.[0-9]*)?)% done\).*",
                             regex progressRE = "^Writing (.*) took.*$",
                             regex messageRE  = "INFO:[ \t]+([^(]+)(\([0-9]+\.?[0-9]*% +done\))?")
    {
        _frameTimeRE = "^Writing .*%s\\.([0-9]+)\\.[^ ]* took (.*) sec.*$" % name;
        _debOut = debOut;
        deb("ExternalNukeProcess: %s %s %s\n" % (path, string(args), totalWrites));

        init (name, path, t, cleanup_func, progressRE, messageRE);

        _writeCount = 0;
        _totalWrites = totalWrites;
        _progressCallback = progressCallback;
        _errors = "";

        _proc = QProcess(mainWindowWidget());

        try
        {
            //connect(_proc, QIODevice.readyRead, readyRead);
            //connect(_proc, QIODevice.readyReadStandardOutput, readyRead);
            connect(_proc, QProcess.readyReadStandardOutput, readyRead);
            connect(_proc, QProcess.finished, finish);
        }
        catch (exception exc)
        {
            print("EXCEPTION during connect: %s\n" % string(exc));
        }

        //
        //  This does not work, don't understand why!
        //
        /*
        if (runtime.build_os() == "LINUX")
        {
            let pe = qt.QProcessEnvironment.systemEnvironment();
            pe.remove("LD_LIBRARY_PATH");
            _proc.setProcessEnvironment(pe);
        }
        */
        _proc.setProcessChannelMode (QProcess.MergedChannels);
        _lastMessage = "";
        _progress = 0.0;
        _cancelDetails = "\nCancel rendering of Nuke node '%s' ?   Frames rendered up to this point will still be available for playback.\n" % name;

        _proc.start (path, args, QIODevice.Unbuffered | QIODevice.ReadOnly | QIODevice.Text);
    }
}

}
