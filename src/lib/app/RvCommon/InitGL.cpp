//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <TwkGLF/GL.h>
#include <iostream>

namespace Rv
{
    using namespace std;

    void initializeGLExtensions()
    {
#ifdef TWK_USE_GLEW
        GLenum err = glewInit(NULL);

        if (GLEW_OK != err)
        {
            cout << "ERROR: glew init failed in RvCommon" << endl;
            abort();
        }
#endif
    }

} // namespace Rv
