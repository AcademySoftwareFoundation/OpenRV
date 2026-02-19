TODO: Generate Patch files for new OpenEXR Versions

To help you resolve the conflicts, here is the high-level logic of your patch. You are adding a new compile flag that, when enabled, changes OpenEXR's error handling.

Define Flag: In cmake/OpenEXRConfig.h.in, it adds #define OPENEXR_ALLOW_PARTIAL_EXR_READ.

Catch Read Errors: In all the ...InputFile.cpp files, it finds the try...catch blocks inside the newLineBufferTask or newTileBufferTask functions (where data is actually read from the file).

Flag Bad Buffers: Instead of throwing the exception, it catches it, sets a new hasException = true boolean on the lineBuffer or tileBuffer, and reports a custom error string.

Fill with Black: In the ...Task::execute methods, it checks for that hasException flag before copying data to the framebuffer. If the flag is true, it overrides the slice's settings and forces sliceFill = true and sliceFillValue = 0.0 (black).

Exception (ImfInputFile.cpp): This file is slightly different. It defers throwing an exception on a bad tile read until the end of the function, allowing it to fill bad tiles with black first.

When you're resolving conflicts, you'll be looking for the readTileData calls and the copyIntoFrameBuffer calls and inserting this logic around them, using the new version's code as the HEAD.

a/cmake/OpenEXRConfig.h.in
a/src/lib/OpenEXR/ImfDeepScanLineInputFile.cpp
a/src/lib/OpenEXR/ImfDeepTiledInputFile.cpp
a/src/lib/OpenEXR/ImfInputFile.cpp
a/src/lib/OpenEXR/ImfScanLineInputFile.cpp
a/src/lib/OpenEXR/ImfTiledInputFile.cpp
