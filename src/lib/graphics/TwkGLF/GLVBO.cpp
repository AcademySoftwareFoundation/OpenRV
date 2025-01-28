//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <TwkGLF/GLVBO.h>

namespace TwkGLF
{
    using namespace std;

    //
    //  Returns a VBO for the renderer to use. if none available in
    //  m_vboList, then make a new one
    //
    //  NOTE: VBO caching mechanism is single threaded only.
    //

    GLVBO* GLVBO::availableVBO(GLVBOVector& vbos)
    {
        GLVBO* newvbo = NULL;

        for (size_t i = 0; i < vbos.size(); ++i)
        {
            if (vbos[i]->isAvailable())
            {
                newvbo = vbos[i];
                newvbo->makeUnavailable();
                return newvbo;
            }
        }

        // nothing in the list is available, make new one
        newvbo = new GLVBO();
        vbos.push_back(newvbo);
        newvbo->makeUnavailable();

        return newvbo;
    }

} // namespace TwkGLF
