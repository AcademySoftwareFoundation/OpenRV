//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
module: file_sequence_mode {
use rvtypes;
use commands;
require qt;

class: FileSequenceMinorMode : MinorMode
{ 
    method: fixpath (void; Event event)
    {
        event.reject();

        //
        //  The contents of the "incoming-source-path" looks like
        //  "filename;;tag". If the tag exists and is "explicit" that
        //  means we shouldn't mess with it. Otherwise infer the
        //  sequence.
        //

        let parts = string.split(event.contents(), ";;");

        if (parts.size() < 2 || (parts[1] != "explicit" && parts[1] != "session"))
        {
            let previous    = event.returnContents(),
                inpath      = if (previous != "") then previous else parts[0],
                info        = qt.QFileInfo(inpath);

            if (info.exists() && info.isFile())
            {
                let (seq,frame) = sequenceOfFile(inpath);
            if (seq != "") event.setReturnContent(seq);
        }
    }
    }

    method: FileSequenceMinorMode(FileSequenceMinorMode;)
    {
        this.init("file-to-sequence",
                  [("incoming-source-path", fixpath, "Convert image paths to sequence paths")],
                  nil,
                  nil);
    }
}

\: createMode (Mode;)
{
    return FileSequenceMinorMode();
}


}
