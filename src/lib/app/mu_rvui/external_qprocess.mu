//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

module: external_qprocess {
use rvtypes;
use commands;
use encoding;
use qt;

documentation: """
ExternalQProcess is derived from ExternalProcess declared in
rvtypes. This implements an actual interface with a process using Qt's
QProcess class. See also external_ioprocess.mu. """

class: ExternalQProcess : ExternalProcess
{
    QProcess _proc;
    string _errors;

    method: readyRead (void;)
    {   
        while (_proc.canReadLine())
        {
            let line = utf8_to_string(_proc.readLine(0).constData());
            if (regex("ERROR:").match(line)) _errors = _errors + line;

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

    method: finish (void; int exitcode = -1)
    {
        if (_proc neq nil)
        {
            print("INFO: ExternalProcess finished with exit code: %d\n" % (exitcode));
            if (exitcode != 0) 
            {
                alertPanel(
                        true, // associated panel (sheet on OSX)
                        ErrorAlert,
                        "ERROR", "Errors:\n%s" % _errors,
                        "OK", nil, nil);
            }
            _proc.close();
            _exitcode = exitcode;
            if (_cleanup neq nil) _cleanup();
            _proc = nil;
        }
    }

    method: isFinished (bool;) { _proc eq nil; }

    method: cancel (void;)
    {
        if (_proc neq nil)
        {
            _proc.close();
            if (_cleanup neq nil) _cleanup();
            _proc = nil;
        }
    }

    method: processIO (void;) {;} // deprecated

    method: ExternalQProcess (ExternalProcess; 
                             string name,
                             string path, 
                             [string] args,
                             int64 timeout_usecs,
                             ExternalProcess.Type t,
                             VoidFunc cleanup_func,
                             regex progressRE = ".*\(([0-9]+(\.[0-9]*)?)% done\).*",
                             regex messageRE  = "INFO:[ \t]+([^(]+)(\([0-9]+\.?[0-9]*% +done\))?")
    {
        print("INFO: ExternalProcess: %s %s %d\n" % (path, string(args), timeout_usecs));

        init(name, path, t, cleanup_func, progressRE, messageRE);

        _proc = QProcess(mainWindowWidget());

        try
        {
            connect(_proc, QIODevice.readyRead, readyRead);
            connect(_proc, QProcess.finished, finish);
        }
        catch (exception exc)
        {
            print("EXCEPTION during connect: %s\n" % string(exc));
        }

        string[] newArgs;
        for_each (a; args) newArgs.push_back(a);
        _errors = "";

        _proc.start(path, newArgs, QIODevice.ReadOnly | QIODevice.Text);
    }

}

}
