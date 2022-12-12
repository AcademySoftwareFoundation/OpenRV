//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

\: doit  (void;)
{
    use io;
    bool windows = runtime.build_os() == "WINDOWS";
    let cmdstring = if windows then "C:/cygwin/bin/ls" else "/bin/ls";
    
    process cmd = process(cmdstring, ["-al", "test"], int64.max);
    istream lsout = cmd.out();
    cmd.close_in();
    
    while (true)
    {
        let line = read_line(lsout, '\n');
        if (line == "") break;
        print("%s\n" % line);
    }

    cmd.close();
}

doit();
