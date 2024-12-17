//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#ifndef __runtime__GLTextModule__h__
#define __runtime__GLTextModule__h__

#include <Mu/Module.h>
#include <Mu/Node.h>

namespace Mu
{

    class GLTextModule : public Module
    {
    public:
        GLTextModule(Context* c, const char* name);
        virtual ~GLTextModule();

        virtual void load();

        static void init();

        static NODE_DECLARATION(TwkGLText_init, void);
        static NODE_DECLARATION(writeAt, void);
        static NODE_DECLARATION(writeAtNLf, int);
        static NODE_DECLARATION(writeAtNLfv, int);
        static NODE_DECLARATION(setSize, void);
        static NODE_DECLARATION(getSize, int);
        static NODE_DECLARATION(bounds, Pointer);
        static NODE_DECLARATION(boundsNL, Pointer);
        static NODE_DECLARATION(color4f, void);
        static NODE_DECLARATION(color3fv, void);
        static NODE_DECLARATION(color4fv, void);
        static NODE_DECLARATION(width, float);
        static NODE_DECLARATION(height, float);
        static NODE_DECLARATION(widthNL, float);
        static NODE_DECLARATION(heightNL, float);
        static NODE_DECLARATION(ascenderHeight, float);
        static NODE_DECLARATION(descenderDepth, float);
    };

} // namespace Mu

#endif // __runtime__GLTextModule__h__
